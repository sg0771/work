#ifndef __clang__
#include "SoftWire.hpp"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#ifdef WIN32
#include <windows.h>
#elif __unix__
#include <sys/mman.h>
#endif

namespace SoftWire
{
	Linker::External* Linker::externals;

	Linker::Linker()
	{
	}

	Linker::~Linker()
	{
		clearExternals();
	}

	void* Linker::resolveExternal(const char* name)
	{
		const External* external = externals;

		while (external)
		{
			if (external->name && strcmp(external->name, name) == 0)
			{
				return external->pointer;
			}

			external = external->next();
		}

		return 0;
	}

	void Linker::defineExternal(void* pointer, const char* name)
	{
		if (!externals)
		{
			externals = new External();
		}

		External* external = externals;

		do
		{
			if (external->name && strcmp(external->name, name) == 0)
			{
				external->pointer = pointer;
				return;
			}

			external = external->next();
		} while (external);

		externals->append(Identifier(pointer, name));
	}

	void Linker::clearExternals()
	{
		delete externals;
		externals = 0;
	}
}

namespace SoftWire
{
	Loader::Loader(const Linker& linker, bool x64) : linker(linker), x64(x64)
	{
		machineCode = 0;
		instructions = 0;
		listing = 0;
		possession = true;

		reset();
	}

	Loader::~Loader()
	{
		if (possession)
		{
			delete[] machineCode;
			machineCode = 0;
		}

		delete instructions;
		instructions = 0;

		delete[] listing;
		listing = 0;
	}

	void (*Loader::callable(const char* entryLabel))()
	{
		if (finalized) throw Error("Cannot retrieve callable from finalized code");

		if (!machineCode)
		{
			loadCode();
		}

		if (!entryLabel)
		{
			return (void(*)())machineCode;
		}

		const unsigned char* entryPoint = resolveLocal(entryLabel, 0);

		if (!entryPoint)
		{
			throw Error("Entry point '%s' not found", entryLabel);
		}

		return (void(*)())entryPoint;
	}

	void (*Loader::finalize(const char* entryLabel))()
	{
		if (!machineCode)
		{
			loadCode();
		}

		finalized = true;

		delete instructions;
		instructions = 0;

		delete[] listing;
		listing = 0;

		if (!entryLabel)
		{
			return (void(*)())machineCode;
		}

		const unsigned char* entryPoint = resolveLocal(entryLabel, 0);

		if (!entryPoint)
		{
			throw Error("Entry point '%s' not found", entryLabel);
		}

		return (void(*)())entryPoint;
	}

	void* Loader::acquire()
	{
		possession = false;

		return machineCode;
	}

	Encoding* Loader::appendEncoding(const Encoding& encoding)
	{
		if (!instructions) instructions = new Instruction();

		instructions->append(encoding);

		return instructions->tail();
	}

	void Loader::loadCode(const char* entryLabel)
	{
		int length = codeLength() + 64;   // NOTE: Code length is not accurate due to alignment issues

		machineCode = new unsigned char[length];
		unsigned char* currentCode = machineCode;

#ifdef WIN32
		unsigned long oldProtection;
		VirtualProtect(machineCode, length, PAGE_EXECUTE_READWRITE, &oldProtection);
#elif __unix__
		mprotect(machineCode, length, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif

		Instruction* instruction = instructions;

		while (instruction)
		{
			Encoding& encoding = *instruction;
			encoding.setAddress(currentCode);
			const char* reference = encoding.getReference();
			const char* label = encoding.getLabel();

			if (reference)
			{
				if (encoding.relativeReference())
				{
					int offset = resolveReference(reference, instruction) - currentCode - encoding.length(currentCode);
					encoding.setJumpOffset(offset);
				}
				else
				{
					intptr_t address = (intptr_t)resolveReference(reference, instruction);

					// Encoded as memory reference or immediate?
					if (encoding.hasDisplacement())
					{
						encoding.addDisplacement(address);
					}
					else if (encoding.hasImmediate())
					{
						encoding.setImmediate(address);
					}
					else
					{
						INTERNAL_ERROR;
					}
				}
			}
			else if (encoding.hasImmediate() && encoding.relativeReference())
			{
				__int64 offset = encoding.getImmediate() - (__int64)currentCode - encoding.length(currentCode);
				encoding.setCallOffset((int)offset);
			}

			if (x64 && encoding.isRipRelative())
			{
				__int64 displacement = encoding.getDisplacement() - (__int64)currentCode - encoding.length(currentCode);
				encoding.setDisplacement(displacement);
			}

			currentCode += encoding.writeCode(currentCode);

			instruction = instruction->next();
		}
	}

	const unsigned char* Loader::resolveReference(const char* name, const Instruction* position) const
	{
		const unsigned char* reference = resolveLocal(name, position);

		if (reference)
		{
			return reference;
		}
		else
		{
			reference = resolveExternal(name);
		}

		if (reference)
		{
			return reference;
		}
		else
		{
			throw Error("Unresolved identifier '%s'", name);
		}
	}

	const unsigned char* Loader::resolveLocal(const char* name, const Instruction* position) const
	{
		const Instruction* instruction = instructions;

		unsigned char* target = machineCode;
		unsigned char* local = 0;   // Closest match before, or after position

		while (instruction)
		{
			if (local && (instruction == position || position == 0))
			{
				return local;
			}
			else if (instruction == position)
			{
				position = 0;   // Return first match following position
			}

			if (instruction->getLabel() && strcmp(instruction->getLabel(), name) == 0)
			{
				local = target;
			}
			else
			{
				target += instruction->length(target);
			}

			instruction = instruction->next();
		}

		if (local)
		{
			return local;
		}

		return 0;
	}

	const unsigned char* Loader::resolveExternal(const char* name) const
	{
		return (unsigned char*)linker.resolveExternal(name);
	}

	int Loader::codeLength() const
	{
		const Instruction* instruction = instructions;
		unsigned char* end = machineCode;

		while (instruction)
		{
			end += instruction->length(end);

			instruction = instruction->next();
		}

		return end - machineCode;
	}

	const char* Loader::getListing()
	{
		if (!machineCode)
		{
			return 0;
		}

		if (listing)
		{
			return listing;
		}

		listing = new char[codeLength() * 4];
		char* buffer = listing;

		for (Instruction* instruction = instructions; instruction; instruction = instruction->next())
		{
			buffer += instruction->printCode(buffer);
		}

		*(buffer - 1) = '\0';

		return listing;
	}

	void Loader::clearListing()
	{
		delete[] listing;
		listing = 0;
	}

	void Loader::reset()
	{
		if (possession)
		{
			delete[] machineCode;
			machineCode = 0;
		}

		machineCode = 0;

		delete instructions;
		instructions = 0;

		delete[] listing;
		listing = 0;

		possession = true;
		finalized = false;
	}

	int Loader::instructionCount()
	{
		Instruction* instruction = instructions;

		int count = 0;

		while (instruction)
		{
			if (instruction->isEmitting())
			{
				count++;
			}

			instruction = instruction->next();
		}

		return count;
	}
}

namespace SoftWire
{
	const Specifier Specifier::specifierSet[] =
	{
		{TYPE_UNKNOWN,	""},

		{TYPE_NEAR,		"NEAR"},
		{TYPE_SHORT,	"SHORT"},
		//	{TYPE_FAR,		"FAR"},

			{TYPE_BYTE,		"BYTE"},
			{TYPE_WORD,		"WORD"},
			{TYPE_DWORD,	"DWORD"},
			//	{TYPE_TWORD,	"TWORD"},
				{TYPE_QWORD,	"QWORD"},
				{TYPE_MMWORD,	"MMWORD"},
				{TYPE_XMMWORD,	"XMMWORD"},
				{TYPE_XWORD,	"XWORD"},
				{TYPE_OWORD,	"OWORD"},

				{TYPE_PTR,		"PTR"}
	};

	Specifier::Type Specifier::scan(const char* string)
	{
		if (string)
		{
			for (int i = 0; i < sizeof(specifierSet) / sizeof(Specifier); i++)
			{
				if (_stricmp(string, specifierSet[i].notation) == 0)
				{
					return specifierSet[i].type;
				}
			}
		}

		return TYPE_UNKNOWN;
	}

	bool Operand::operator==(Operand& op)
	{
		return type == op.type &&
			baseReg == op.baseReg &&
			indexReg == op.indexReg &&
			scale == op.scale &&
			displacement == op.displacement;
	}

	bool Operand::operator!=(Operand& op)
	{
		return type != op.type ||
			baseReg != op.baseReg ||
			indexReg != op.indexReg ||
			scale != op.scale ||
			displacement != op.displacement;
	}

	bool Operand::isSubtypeOf(Type type, Type baseType)
	{
		return (type & baseType) == type;
	}

	bool Operand::isSubtypeOf(Type baseType) const
	{
		return isSubtypeOf(type, baseType);
	}

	const char* Operand::string() const
	{
		static char string[256];

		if (isVoid(type))
		{
			return 0;
		}
		else if (isImm(type))
		{
			if (reference)
			{
				return reference;
			}
			else
			{
				if (value <= 127 && value >= -128)
				{
					snprintf(string, 255, "0x%0.2X", value & 0xFF);
				}
				else if (value <= 32767 && value - 32768)
				{
					snprintf(string, 255, "0x%0.4X", value & 0xFFFF);
				}
				else
				{
					snprintf(string, 255, "0x%0.8X", value);
				}
			}
		}
		else if (isReg(type))
		{
			return regName();
		}
		else if (isMem(type))
		{
			switch (type)
			{
			case OPERAND_MEM8:
				snprintf(string, 255, "byte ptr [");
				break;
			case OPERAND_MEM16:
				snprintf(string, 255, "word ptr [");
				break;
			case OPERAND_MEM32:
				snprintf(string, 255, "dword ptr [");
				break;
			case OPERAND_MEM64:
				snprintf(string, 255, "qword ptr [");
				break;
			case OPERAND_MEM128:
				snprintf(string, 255, "xmmword ptr [");
				break;
			case OPERAND_MEM:
			default:
				snprintf(string, 255, "byte ptr [");
			}

			if (baseReg != Encoding::REG_UNKNOWN)
			{
				snprintf(string, 255, "%s%s", string, regName());

				if (indexReg != Encoding::REG_UNKNOWN)
				{
					snprintf(string, 255, "%s+", string);
				}
			}

			if (indexReg != Encoding::REG_UNKNOWN)
			{
				snprintf(string, 255, "%s%s", string, indexName());
			}

			switch (scale)
			{
			case 0:
			case 1:
				break;
			case 2:
				snprintf(string, 255, "%s*2", string);
				break;
			case 4:
				snprintf(string, 255, "%s*4", string);
				break;
			case 8:
				snprintf(string, 255, "%s*8", string);
				break;
			default:
				throw INTERNAL_ERROR;
			}

			if (displacement)
			{
				if (baseReg != Encoding::REG_UNKNOWN ||
					indexReg != Encoding::REG_UNKNOWN)
				{
					snprintf(string, 255, "%s+", string);
				}

				if (reference)
				{
					snprintf(string, 255, "%s%s", string, reference);
				}
				else
				{
					if (displacement <= 32767 && displacement >= -32768)
					{
#ifdef _M_IX86
						snprintf(string, 255, "%s%d", string, displacement);
#else
						snprintf(string, 255, "%s%lld", string, displacement);
#endif
					}
					else
					{
#ifdef _M_IX86
						snprintf(string, 255, "%s0x%0.8X", string, displacement);
#else
						snprintf(string, 255, "%s0x%0.8llx", string, displacement);
#endif
					}
				}
			}

			snprintf(string, 255, "%s]", string);
		}
		else
		{
			throw INTERNAL_ERROR;
		}

		return _strlwr(string);
	}

	bool Operand::isVoid(Type type)
	{
		return type == OPERAND_VOID;
	}

	bool Operand::isImm(Type type)
	{
		return (type & OPERAND_IMM) == type;
	}

	bool Operand::isReg(Type type)
	{
		return (type & OPERAND_REG) == type;
	}

	bool Operand::isMem(Type type)
	{
		return (type & OPERAND_MEM) == type;
	}

	bool Operand::isR_M(Type type)
	{
		return (type & OPERAND_R_M) == type;
	}

	bool Operand::isVoid(const Operand& operand)
	{
		return isVoid(operand.type);
	}

	bool Operand::isImm(const Operand& operand)
	{
		return isImm(operand.type);
	}

	bool Operand::isReg(const Operand& operand)
	{
		return isReg(operand.type);
	}

	bool Operand::isMem(const Operand& operand)
	{
		return isMem(operand.type);
	}

	bool Operand::isR_M(const Operand& operand)
	{
		return isR_M(operand.type);
	}

	const Operand::Register Operand::registerSet[] =
	{
		{OPERAND_VOID,		""},

		{OPERAND_AL,		"al", Encoding::AL},
		{OPERAND_CL,		"cl", Encoding::CL},
		{OPERAND_REG8,		"dl", Encoding::DL},
		{OPERAND_REG8,		"bl", Encoding::BL},
		{OPERAND_REG8,		"ah", Encoding::AH},
		{OPERAND_REG8,		"ch", Encoding::CH},
		{OPERAND_REG8,		"dh", Encoding::DH},
		{OPERAND_REG8,		"bh", Encoding::BH},

		{OPERAND_AX,		"ax", Encoding::AX},
		{OPERAND_CX,		"cx", Encoding::CX},
		{OPERAND_DX,		"dx", Encoding::DX},
		{OPERAND_REG16,		"bx", Encoding::BX},
		{OPERAND_REG16,		"sp", Encoding::SP},
		{OPERAND_REG16,		"bp", Encoding::BP},
		{OPERAND_REG16,		"si", Encoding::SI},
		{OPERAND_REG16,		"di", Encoding::DI},

		{OPERAND_EAX,		"eax", Encoding::EAX},
		{OPERAND_ECX,		"ecx", Encoding::ECX},
		{OPERAND_REG32,		"edx", Encoding::EDX},
		{OPERAND_REG32,		"ebx", Encoding::EBX},
		{OPERAND_REG32,		"esp", Encoding::ESP},
		{OPERAND_REG32,		"ebp", Encoding::EBP},
		{OPERAND_REG32,		"esi", Encoding::ESI},
		{OPERAND_REG32,		"edi", Encoding::EDI},

		{OPERAND_ST0,		"st",  Encoding::ST0},
		{OPERAND_ST0,		"st0", Encoding::ST0},
		{OPERAND_FPUREG,	"st1", Encoding::ST1},
		{OPERAND_FPUREG,	"st2", Encoding::ST2},
		{OPERAND_FPUREG,	"st3", Encoding::ST3},
		{OPERAND_FPUREG,	"st4", Encoding::ST4},
		{OPERAND_FPUREG,	"st5", Encoding::ST5},
		{OPERAND_FPUREG,	"st6", Encoding::ST6},
		{OPERAND_FPUREG,	"st7", Encoding::ST7},

		{OPERAND_MMREG,		"mm0", Encoding::MM0},
		{OPERAND_MMREG,		"mm1", Encoding::MM1},
		{OPERAND_MMREG,		"mm2", Encoding::MM2},
		{OPERAND_MMREG,		"mm3", Encoding::MM3},
		{OPERAND_MMREG,		"mm4", Encoding::MM4},
		{OPERAND_MMREG,		"mm5", Encoding::MM5},
		{OPERAND_MMREG,		"mm6", Encoding::MM6},
		{OPERAND_MMREG,		"mm7", Encoding::MM7},

		{OPERAND_XMMREG,	"xmm0", Encoding::XMM0},
		{OPERAND_XMMREG,	"xmm1", Encoding::XMM1},
		{OPERAND_XMMREG,	"xmm2", Encoding::XMM2},
		{OPERAND_XMMREG,	"xmm3", Encoding::XMM3},
		{OPERAND_XMMREG,	"xmm4", Encoding::XMM4},
		{OPERAND_XMMREG,	"xmm5", Encoding::XMM5},
		{OPERAND_XMMREG,	"xmm6", Encoding::XMM6},
		{OPERAND_XMMREG,	"xmm7", Encoding::XMM7}
	};

	const char* Operand::regName() const
	{
		for (int i = 0; i < sizeof(registerSet) / sizeof(Operand::Register); i++)
		{
			if (reg == registerSet[i].reg)
			{
				if (isSubtypeOf(OPERAND_MEM) && Operand::isSubtypeOf(registerSet[i].type, OPERAND_REG32) ||
					Operand::isSubtypeOf(registerSet[i].type, type) && reg == registerSet[i].reg)
				{
					return registerSet[i].notation;
				}
			}
		}

		throw INTERNAL_ERROR;
	}

	const char* Operand::indexName() const
	{
		for (int i = 0; i < sizeof(registerSet) / sizeof(Operand::Register); i++)
		{
			if (indexReg == registerSet[i].reg && Operand::isSubtypeOf(registerSet[i].type, OPERAND_REG32))
			{
				return registerSet[i].notation;
			}
		}

		throw INTERNAL_ERROR;
	}

	const Operand::Register Operand::syntaxSet[] =
	{
		{OPERAND_VOID,		""},

		{OPERAND_ONE,		"1"},
		{OPERAND_IMM,		"imm"},
		{OPERAND_IMM8,		"imm8"},
		{OPERAND_IMM16,		"imm16"},
		{OPERAND_IMM32,		"imm32"},
		//	{OPERAND_IMM64,		"imm64"},

			{OPERAND_AL,		"AL"},
			{OPERAND_AX,		"AX"},
			{OPERAND_EAX,		"EAX"},
			{OPERAND_RAX,		"RAX"},
			{OPERAND_DX,		"DX"},
			{OPERAND_CL,		"CL"},
			{OPERAND_CX,		"CX"},
			{OPERAND_ECX,		"ECX"},
			{OPERAND_ST0,		"ST0"},

			{OPERAND_REG8,		"reg8"},
			{OPERAND_REG16,		"reg16"},
			{OPERAND_REG32,		"reg32"},
			{OPERAND_REG64,		"reg64"},
			{OPERAND_FPUREG,	"fpureg"},
			{OPERAND_MMREG,		"mmreg"},
			{OPERAND_XMMREG,	"xmmreg"},

			{OPERAND_MEM,		"mem"},
			{OPERAND_MEM8,		"mem8"},
			{OPERAND_MEM16,		"mem16"},
			{OPERAND_MEM32,		"mem32"},
			{OPERAND_MEM64,		"mem64"},
			{OPERAND_MEM128,	"mem128"},

			{OPERAND_R_M8,		"r/m8"},
			{OPERAND_R_M16,		"r/m16"},
			{OPERAND_R_M32,		"r/m32"},
			{OPERAND_R_M64,		"r/m64"},
			{OPERAND_R_M128,	"r/m128"},

			{OPERAND_XMM32,		"xmm32"},
			{OPERAND_XMM64,		"xmm64"},
			{OPERAND_MM64,		"mm64"}
	};

	Operand::Type Operand::scanSyntax(const char* string)
	{
		if (string)
		{
			for (int i = 0; i < sizeof(syntaxSet) / sizeof(Operand::Register); i++)
			{
				if (_stricmp(string, syntaxSet[i].notation) == 0)
				{
					return syntaxSet[i].type;
				}
			}
		}

		return OPERAND_UNKNOWN;
	}
}

namespace SoftWire
{
	Optimizer::Optimizer(bool x64) : RegisterAllocator(x64)
	{
	}

	Optimizer::~Optimizer()
	{
	}
}

namespace SoftWire
{
	const Instruction::Syntax InstructionSet::instructionSet[] =
	{
		/*
			Encoding syntax:
			----------------
			+r Add register value to opcode
			/# Value for Mod R/M register field encoding
			/r Effective address encoding
			ib Byte immediate
			iw Word immediate
			id Dword immediate
			-b Byte relative address
			-i Word or dword relative address
			p0 LOCK instruction prefix (F0h)
			p2 REPNE/REPNZ instruction prefix (F2h)
			p3 REP/REPE/REPZ instruction prefix (F3h) (also SSE prefix)
			po Offset override prefix (66h)
			pa Address override prefix (67h)

			Read Keywords.cpp for operands syntax
		*/

		// x86 instruction set
		{"AAA",				"",							"37",					Instruction::CPU_8086 | Instruction::CPU_INVALID64},
		{"AAD",				"",							"D5 0A",				Instruction::CPU_8086 | Instruction::CPU_INVALID64},
		{"AAD",				"imm",						"D5 ib",				Instruction::CPU_8086 | Instruction::CPU_INVALID64},
		{"AAM",				"",							"D4 0A",				Instruction::CPU_8086 | Instruction::CPU_INVALID64},
		{"AAM",				"imm",						"D4 ib",				Instruction::CPU_8086 | Instruction::CPU_INVALID64},
		{"AAS",				"",							"3F",					Instruction::CPU_8086 | Instruction::CPU_INVALID64},
		{"ADC",				"r/m8,reg8",				"10 /r",				Instruction::CPU_8086},
		{"ADC",				"r/m16,reg16",				"po 11 /r",				Instruction::CPU_8086},
		{"ADC",				"r/m32,reg32",				"po 11 /r",				Instruction::CPU_386},
		{"ADC",				"r/m64,reg64",				"po 11 /r",				Instruction::CPU_X64},
		{"ADC",				"reg8,r/m8",				"12 /r",				Instruction::CPU_8086},
		{"ADC",				"reg16,r/m16",				"po 13 /r",				Instruction::CPU_8086},
		{"ADC",				"reg32,r/m32",				"po 13 /r",				Instruction::CPU_386},
		{"ADC",				"reg64,r/m64",				"po 13 /r",				Instruction::CPU_X64},
		{"ADC",				"BYTE r/m8,imm8",			"80 /2 ib",				Instruction::CPU_8086},
		{"ADC",				"WORD r/m16,imm16",			"po 81 /2 iw",			Instruction::CPU_8086},
		{"ADC",				"DWORD r/m32,imm32",		"po 81 /2 id",			Instruction::CPU_386},
		{"ADC",				"QWORD r/m64,imm32",		"po 81 /2 id",			Instruction::CPU_X64},
		{"ADC",				"WORD r/m16,imm8",			"po 83 /2 ib",			Instruction::CPU_8086},
		{"ADC",				"DWORD r/m32,imm8",			"po 83 /2 ib",			Instruction::CPU_386},
		{"ADC",				"QWORD r/m64,imm8",			"po 83 /2 ib",			Instruction::CPU_X64},
		{"ADC",				"AL,imm8",					"14 ib",				Instruction::CPU_8086},
		{"ADC",				"AX,imm16",					"po 15 iw",				Instruction::CPU_8086},
		{"ADC",				"EAX,imm32",				"po 15 id",				Instruction::CPU_386},
		{"ADC",				"RAX,imm32",				"po 15 id",				Instruction::CPU_X64},
		{"ADD",				"r/m8,reg8",				"00 /r",				Instruction::CPU_8086},
		{"ADD",				"r/m16,reg16",				"po 01 /r",				Instruction::CPU_8086},
		{"ADD",				"r/m32,reg32",				"po 01 /r",				Instruction::CPU_386},
		{"ADD",				"r/m64,reg64",				"po 01 /r",				Instruction::CPU_X64},
		{"ADD",				"reg8,r/m8",				"02 /r",				Instruction::CPU_8086},
		{"ADD",				"reg16,r/m16",				"po 03 /r",				Instruction::CPU_8086},
		{"ADD",				"reg32,r/m32",				"po 03 /r",				Instruction::CPU_386},
		{"ADD",				"reg64,r/m64",				"po 03 /r",				Instruction::CPU_X64},
		{"ADD",				"BYTE r/m8,imm8",			"80 /0 ib",				Instruction::CPU_8086},
		{"ADD",				"WORD r/m16,imm16",			"po 81 /0 iw",			Instruction::CPU_8086},
		{"ADD",				"DWORD r/m32,imm32",		"po 81 /0 id",			Instruction::CPU_386},
		{"ADD",				"QWORD r/m64,imm32",		"po 81 /0 id",			Instruction::CPU_X64},
		{"ADD",				"WORD r/m16,imm8",			"po 83 /0 ib",			Instruction::CPU_8086},
		{"ADD",				"DWORD r/m32,imm8",			"po 83 /0 ib",			Instruction::CPU_386},
		{"ADD",				"QWORD r/m64,imm8",			"po 83 /0 ib",			Instruction::CPU_X64},
		{"ADD",				"AL,imm8",					"04 ib",				Instruction::CPU_8086},
		{"ADD",				"AX,imm16",					"po 05 iw",				Instruction::CPU_8086},
		{"ADD",				"EAX,imm32",				"po 05 id",				Instruction::CPU_386},
		{"ADD",				"RAX,imm32",				"po 05 id",				Instruction::CPU_X64},
		{"ADDPD",			"xmmreg,r/m128",			"66 0F 58 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"ADDPS",			"xmmreg,r/m128",			"0F 58 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"ADDSD",			"xmmreg,xmm64",				"p2 0F 58 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"ADDSS",			"xmmreg,xmm32",				"p3 0F 58 /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"ADDSUBPD",		"xmmreg,r/m128",			"66 0F D0 /r",			Instruction::CPU_PNI},
		{"ADDSUBPS",		"xmmreg,r/m128",			"p2 0F D0 /r",			Instruction::CPU_PNI},
		{"ALIGN",			"imm",						"p1 90"},				// Special 'instruction', indicated by the 'p1' prefix
		{"AND",				"r/m8,reg8",				"20 /r",				Instruction::CPU_8086},
		{"AND",				"r/m16,reg16",				"po 21 /r",				Instruction::CPU_8086},
		{"AND",				"r/m32,reg32",				"po 21 /r",				Instruction::CPU_386},
		{"AND",				"r/m64,reg64",				"po 21 /r",				Instruction::CPU_X64},
		{"AND",				"reg8,r/m8",				"22 /r",				Instruction::CPU_8086},
		{"AND",				"reg16,r/m16",				"po 23 /r",				Instruction::CPU_8086},
		{"AND",				"reg32,r/m32",				"po 23 /r",				Instruction::CPU_386},
		{"AND",				"reg64,r/m64",				"po 23 /r",				Instruction::CPU_X64},
		{"AND",				"BYTE r/m8,imm8",			"80 /4 ib",				Instruction::CPU_8086},
		{"AND",				"WORD r/m16,imm16",			"po 81 /4 iw",			Instruction::CPU_8086},
		{"AND",				"DWORD r/m32,imm32",		"po 81 /4 id",			Instruction::CPU_386},
		{"AND",				"QWORD r/m64,imm32",		"po 81 /4 id",			Instruction::CPU_X64},
		{"AND",				"WORD r/m16,imm8",			"po 83 /4 ib",			Instruction::CPU_8086},
		{"AND",				"DWORD r/m32,imm8",			"po 83 /4 ib",			Instruction::CPU_386},
		{"AND",				"QWORD r/m64,imm8",			"po 83 /4 ib",			Instruction::CPU_X64},
		{"AND",				"AL,imm8",					"24 ib",				Instruction::CPU_8086},
		{"AND",				"AX,imm16",					"po 25 iw",				Instruction::CPU_8086},
		{"AND",				"EAX,imm32",				"po 25 id",				Instruction::CPU_386},
		{"AND",				"RAX,imm32",				"po 25 id",				Instruction::CPU_X64},
		{"ANDNPD",			"xmmreg,r/m128",			"66 0F 55 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"ANDNPS",			"xmmreg,r/m128",			"0F 55 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"ANDPD",			"xmmreg,r/m128",			"66 0F 54 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"ANDPS",			"xmmreg,r/m128",			"0F 54 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		//	{"ARPL",			"r/m16,reg16",				"63 /r",				Instruction::CPU_286 | Instruction::CPU_PRIV},
			{"BLENDPD",			"xmmreg,r/m128,imm8",		"66 0F 3A 0D /r ib",	Instruction::CPU_SSE4_1},
			{"BLENDPS",			"xmmreg,r/m128,imm8",		"66 0F 3A 0C /r ib",	Instruction::CPU_SSE4_1},
			{"BLENDVPS",		"xmmreg,r/m128",			"66 0F 38 14 /r",		Instruction::CPU_SSE4_1},
			{"BOUND",			"reg16,mem",				"po 62 /r",				Instruction::CPU_186 | Instruction::CPU_INVALID64},
			{"BOUND",			"reg32,mem",				"po 62 /r",				Instruction::CPU_386 | Instruction::CPU_INVALID64},
			{"BSF",				"reg16,r/m16",				"po 0F BC /r",			Instruction::CPU_386},
			{"BSF",				"reg32,r/m32",				"po 0F BC /r",			Instruction::CPU_386},
			{"BSF",				"reg64,r/m64",				"po 0F BC /r",			Instruction::CPU_X64},
			{"BSR",				"reg16,r/m16",				"po 0F BD /r",			Instruction::CPU_386},
			{"BSR",				"reg32,r/m32",				"po 0F BD /r",			Instruction::CPU_386},
			{"BSR",				"reg64,r/m64",				"po 0F BD /r",			Instruction::CPU_X64},
			{"BSWAP",			"reg32",					"po 0F C8 +r",			Instruction::CPU_486},
			{"BSWAP",			"reg64",					"po 0F C8 +r",			Instruction::CPU_X64},
			{"BT",				"r/m16,reg16",				"po 0F A3 /r",			Instruction::CPU_386},
			{"BT",				"r/m32,reg32",				"po 0F A3 /r",			Instruction::CPU_386},
			{"BT",				"r/m64,reg64",				"po 0F A3 /r",			Instruction::CPU_X64},
			{"BT",				"WORD r/m16,imm8",			"po 0F BA /4 ib",		Instruction::CPU_386},
			{"BT",				"DWORD r/m32,imm8",			"po 0F BA /4 ib",		Instruction::CPU_386},
			{"BT",				"QWORD r/m64,imm8",			"po 0F BA /4 ib",		Instruction::CPU_X64},
			{"BTC",				"r/m16,reg16",				"po 0F BB /r",			Instruction::CPU_386},
			{"BTC",				"r/m32,reg32",				"po 0F BB /r",			Instruction::CPU_386},
			{"BTC",				"r/m64,reg64",				"po 0F BB /r",			Instruction::CPU_X64},
			{"BTC",				"WORD r/m16,imm8",			"po 0F BA /7 ib",		Instruction::CPU_386},
			{"BTC",				"DWORD r/m32,imm8",			"po 0F BA /7 ib",		Instruction::CPU_386},
			{"BTC",				"QWORD r/m64,imm8",			"po 0F BA /7 ib",		Instruction::CPU_X64},
			{"BTR",				"r/m16,reg16",				"po 0F B3 /r",			Instruction::CPU_386},
			{"BTR",				"r/m32,reg32",				"po 0F B3 /r",			Instruction::CPU_386},
			{"BTR",				"r/m64,reg64",				"po 0F B3 /r",			Instruction::CPU_X64},
			{"BTR",				"WORD r/m16,imm8",			"po 0F BA /6 ib",		Instruction::CPU_386},
			{"BTR",				"DWORD r/m32,imm8",			"po 0F BA /6 ib",		Instruction::CPU_386},
			{"BTR",				"QWORD r/m64,imm8",			"po 0F BA /6 ib",		Instruction::CPU_X64},
			{"BTS",				"r/m16,reg16",				"po 0F AB /r",			Instruction::CPU_386},
			{"BTS",				"r/m32,reg32",				"po 0F AB /r",			Instruction::CPU_386},
			{"BTS",				"r/m64,reg64",				"po 0F AB /r",			Instruction::CPU_X64},
			{"BTS",				"WORD r/m16,imm8",			"po 0F BA /5 ib",		Instruction::CPU_386},
			{"BTS",				"DWORD r/m32,imm8",			"po 0F BA /5 ib",		Instruction::CPU_386},
			{"BTS",				"QWORD r/m64,imm8",			"po 0F BA /5 ib",		Instruction::CPU_X64},
			{"CALL",			"imm",						"E8 -i",				Instruction::CPU_8086},
			//	{"CALL",			"imm:imm16",				"po 9A iw iw",			Instruction::CPU_8086 | Instruction::CPU_INVALID64},
			//	{"CALL",			"imm:imm32",				"po 9A id iw",			Instruction::CPU_386 | Instruction::CPU_INVALID64},
			//	{"CALL",			"FAR mem16",				"po FF /3",				Instruction::CPU_8086},
			//	{"CALL",			"FAR mem32",				"po FF /3",				Instruction::CPU_386},
				{"CALL",			"WORD r/m16",				"po FF /2",				Instruction::CPU_8086},
				{"CALL",			"DWORD r/m32",				"po FF /2",				Instruction::CPU_386},
				{"CALL",			"QWORD r/m64",				"po FF /2",				Instruction::CPU_X64},
				{"CBW",				"",							"po 98",				Instruction::CPU_8086},
				{"CDQ",				"",							"po 99",				Instruction::CPU_386},
				{"CDQE",			"",							"po 98",				Instruction::CPU_X64},
				{"CLC",				"",							"F8",					Instruction::CPU_8086},
				{"CLD",				"",							"FC",					Instruction::CPU_8086},
				{"CLFLUSH",			"mem",						"0F AE /7",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
				{"CLI",				"",							"FA",					Instruction::CPU_8086},
				//	{"CLTS",			"",							"0F 06",				Instruction::CPU_286 | Instruction::CPU_PRIV},
					{"CMC",				"",							"F5",					Instruction::CPU_8086},
					{"CMOVA",			"reg16,r/m16",				"po 0F 47 /r",			Instruction::CPU_P6},
					{"CMOVA",			"reg32,r/m32",				"po 0F 47 /r",			Instruction::CPU_P6},
					{"CMOVA",			"reg64,r/m64",				"po 0F 47 /r",			Instruction::CPU_X64},
					{"CMOVAE",			"reg16,r/m16",				"po 0F 43 /r",			Instruction::CPU_P6},
					{"CMOVAE",			"reg32,r/m32",				"po 0F 43 /r",			Instruction::CPU_P6},
					{"CMOVAE",			"reg64,r/m64",				"po 0F 43 /r",			Instruction::CPU_X64},
					{"CMOVB",			"reg16,r/m16",				"po 0F 42 /r",			Instruction::CPU_P6},
					{"CMOVB",			"reg32,r/m32",				"po 0F 42 /r",			Instruction::CPU_P6},
					{"CMOVB",			"reg64,r/m64",				"po 0F 42 /r",			Instruction::CPU_X64},
					{"CMOVBE",			"reg16,r/m16",				"po 0F 46 /r",			Instruction::CPU_P6},
					{"CMOVBE",			"reg32,r/m32",				"po 0F 46 /r",			Instruction::CPU_P6},
					{"CMOVBE",			"reg64,r/m64",				"po 0F 46 /r",			Instruction::CPU_X64},
					{"CMOVC",			"reg16,r/m16",				"po 0F 42 /r",			Instruction::CPU_P6},
					{"CMOVC",			"reg32,r/m32",				"po 0F 42 /r",			Instruction::CPU_P6},
					{"CMOVC",			"reg64,r/m64",				"po 0F 42 /r",			Instruction::CPU_X64},
					{"CMOVE",			"reg16,r/m16",				"po 0F 44 /r",			Instruction::CPU_P6},
					{"CMOVE",			"reg32,r/m32",				"po 0F 44 /r",			Instruction::CPU_P6},
					{"CMOVE",			"reg64,r/m64",				"po 0F 44 /r",			Instruction::CPU_X64},
					{"CMOVG",			"reg16,r/m16",				"po 0F 4F /r",			Instruction::CPU_P6},
					{"CMOVG",			"reg32,r/m32",				"po 0F 4F /r",			Instruction::CPU_P6},
					{"CMOVG",			"reg64,r/m64",				"po 0F 4F /r",			Instruction::CPU_X64},
					{"CMOVGE",			"reg16,r/m16",				"po 0F 4D /r",			Instruction::CPU_P6},
					{"CMOVGE",			"reg32,r/m32",				"po 0F 4D /r",			Instruction::CPU_P6},
					{"CMOVGE",			"reg64,r/m64",				"po 0F 4D /r",			Instruction::CPU_X64},
					{"CMOVL",			"reg16,r/m16",				"po 0F 4C /r",			Instruction::CPU_P6},
					{"CMOVL",			"reg32,r/m32",				"po 0F 4C /r",			Instruction::CPU_P6},
					{"CMOVL",			"reg64,r/m64",				"po 0F 4C /r",			Instruction::CPU_X64},
					{"CMOVLE",			"reg16,r/m16",				"po 0F 4E /r",			Instruction::CPU_P6},
					{"CMOVLE",			"reg32,r/m32",				"po 0F 4E /r",			Instruction::CPU_P6},
					{"CMOVLE",			"reg64,r/m64",				"po 0F 4E /r",			Instruction::CPU_X64},
					{"CMOVNA",			"reg16,r/m16",				"po 0F 46 /r",			Instruction::CPU_P6},
					{"CMOVNA",			"reg32,r/m32",				"po 0F 46 /r",			Instruction::CPU_P6},
					{"CMOVNA",			"reg64,r/m64",				"po 0F 46 /r",			Instruction::CPU_X64},
					{"CMOVNB",			"reg16,r/m16",				"po 0F 43 /r",			Instruction::CPU_P6},
					{"CMOVNB",			"reg32,r/m32",				"po 0F 43 /r",			Instruction::CPU_P6},
					{"CMOVNB",			"reg64,r/m64",				"po 0F 43 /r",			Instruction::CPU_X64},
					{"CMOVNBE",			"reg16,r/m16",				"po 0F 47 /r",			Instruction::CPU_P6},
					{"CMOVNBE",			"reg32,r/m32",				"po 0F 47 /r",			Instruction::CPU_P6},
					{"CMOVNBE",			"reg64,r/m64",				"po 0F 47 /r",			Instruction::CPU_X64},
					{"CMOVNC",			"reg16,r/m16",				"po 0F 43 /r",			Instruction::CPU_P6},
					{"CMOVNC",			"reg32,r/m32",				"po 0F 43 /r",			Instruction::CPU_P6},
					{"CMOVNC",			"reg64,r/m64",				"po 0F 43 /r",			Instruction::CPU_X64},
					{"CMOVNE",			"reg16,r/m16",				"po 0F 45 /r",			Instruction::CPU_P6},
					{"CMOVNE",			"reg32,r/m32",				"po 0F 45 /r",			Instruction::CPU_P6},
					{"CMOVNE",			"reg64,r/m64",				"po 0F 45 /r",			Instruction::CPU_X64},
					{"CMOVNEA",			"reg16,r/m16",				"po 0F 42 /r",			Instruction::CPU_P6},
					{"CMOVNEA",			"reg32,r/m32",				"po 0F 42 /r",			Instruction::CPU_P6},
					{"CMOVNEA",			"reg64,r/m64",				"po 0F 42 /r",			Instruction::CPU_X64},
					{"CMOVNG",			"reg16,r/m16",				"po 0F 4E /r",			Instruction::CPU_P6},
					{"CMOVNG",			"reg32,r/m32",				"po 0F 4E /r",			Instruction::CPU_P6},
					{"CMOVNG",			"reg64,r/m64",				"po 0F 4E /r",			Instruction::CPU_X64},
					{"CMOVNGE",			"reg16,r/m16",				"po 0F 4C /r",			Instruction::CPU_P6},
					{"CMOVNGE",			"reg32,r/m32",				"po 0F 4C /r",			Instruction::CPU_P6},
					{"CMOVNGE",			"reg64,r/m64",				"po 0F 4C /r",			Instruction::CPU_X64},
					{"CMOVNL",			"reg16,r/m16",				"po 0F 4D /r",			Instruction::CPU_P6},
					{"CMOVNL",			"reg32,r/m32",				"po 0F 4D /r",			Instruction::CPU_P6},
					{"CMOVNL",			"reg64,r/m64",				"po 0F 4D /r",			Instruction::CPU_X64},
					{"CMOVNLE",			"reg16,r/m16",				"po 0F 4F /r",			Instruction::CPU_P6},
					{"CMOVNLE",			"reg32,r/m32",				"po 0F 4F /r",			Instruction::CPU_P6},
					{"CMOVNLE",			"reg64,r/m64",				"po 0F 4F /r",			Instruction::CPU_X64},
					{"CMOVNO",			"reg16,r/m16",				"po 0F 41 /r",			Instruction::CPU_P6},
					{"CMOVNO",			"reg32,r/m32",				"po 0F 41 /r",			Instruction::CPU_P6},
					{"CMOVNO",			"reg64,r/m64",				"po 0F 41 /r",			Instruction::CPU_X64},
					{"CMOVNP",			"reg16,r/m16",				"po 0F 4B /r",			Instruction::CPU_P6},
					{"CMOVNP",			"reg32,r/m32",				"po 0F 4B /r",			Instruction::CPU_P6},
					{"CMOVNP",			"reg64,r/m64",				"po 0F 4B /r",			Instruction::CPU_X64},
					{"CMOVNS",			"reg16,r/m16",				"po 0F 49 /r",			Instruction::CPU_P6},
					{"CMOVNS",			"reg32,r/m32",				"po 0F 49 /r",			Instruction::CPU_P6},
					{"CMOVNS",			"reg64,r/m64",				"po 0F 49 /r",			Instruction::CPU_X64},
					{"CMOVNZ",			"reg16,r/m16",				"po 0F 45 /r",			Instruction::CPU_P6},
					{"CMOVNZ",			"reg32,r/m32",				"po 0F 45 /r",			Instruction::CPU_P6},
					{"CMOVNZ",			"reg64,r/m64",				"po 0F 45 /r",			Instruction::CPU_X64},
					{"CMOVO",			"reg16,r/m16",				"po 0F 40 /r",			Instruction::CPU_P6},
					{"CMOVO",			"reg32,r/m32",				"po 0F 40 /r",			Instruction::CPU_P6},
					{"CMOVO",			"reg64,r/m64",				"po 0F 40 /r",			Instruction::CPU_X64},
					{"CMOVP",			"reg16,r/m16",				"po 0F 4A /r",			Instruction::CPU_P6},
					{"CMOVP",			"reg32,r/m32",				"po 0F 4A /r",			Instruction::CPU_P6},
					{"CMOVP",			"reg64,r/m64",				"po 0F 4A /r",			Instruction::CPU_X64},
					{"CMOVPE",			"reg16,r/m16",				"po 0F 4A /r",			Instruction::CPU_P6},
					{"CMOVPE",			"reg32,r/m32",				"po 0F 4A /r",			Instruction::CPU_P6},
					{"CMOVPE",			"reg64,r/m64",				"po 0F 4A /r",			Instruction::CPU_X64},
					{"CMOVPO",			"reg16,r/m16",				"po 0F 4B /r",			Instruction::CPU_P6},
					{"CMOVPO",			"reg32,r/m32",				"po 0F 4B /r",			Instruction::CPU_P6},
					{"CMOVPO",			"reg64,r/m64",				"po 0F 4B /r",			Instruction::CPU_X64},
					{"CMOVS",			"reg16,r/m16",				"po 0F 48 /r",			Instruction::CPU_P6},
					{"CMOVS",			"reg32,r/m32",				"po 0F 48 /r",			Instruction::CPU_P6},
					{"CMOVS",			"reg32,r/m64",				"po 0F 48 /r",			Instruction::CPU_X64},
					{"CMOVZ",			"reg16,r/m16",				"po 0F 44 /r",			Instruction::CPU_P6},
					{"CMOVZ",			"reg32,r/m32",				"po 0F 44 /r",			Instruction::CPU_P6},
					{"CMOVZ",			"reg64,r/m64",				"po 0F 44 /r",			Instruction::CPU_X64},
					{"CMP",				"r/m8,reg8",				"38 /r",				Instruction::CPU_8086},
					{"CMP",				"r/m16,reg16",				"po 39 /r",				Instruction::CPU_8086},
					{"CMP",				"r/m32,reg32",				"po 39 /r",				Instruction::CPU_386},
					{"CMP",				"r/m64,reg64",				"po 39 /r",				Instruction::CPU_X64},
					{"CMP",				"reg8,r/m8",				"3A /r",				Instruction::CPU_8086},
					{"CMP",				"reg16,r/m16",				"po 3B /r",				Instruction::CPU_8086},
					{"CMP",				"reg32,r/m32",				"po 3B /r",				Instruction::CPU_386},
					{"CMP",				"reg64,r/m64",				"po 3B /r",				Instruction::CPU_X64},
					{"CMP",				"BYTE r/m8,imm8",			"80 /7 ib",				Instruction::CPU_8086},
					{"CMP",				"WORD r/m16,imm16",			"po 81 /7 iw",			Instruction::CPU_8086},
					{"CMP",				"DWORD r/m32,imm32",		"po 81 /7 id",			Instruction::CPU_386},
					{"CMP",				"QWORD r/m64,imm32",		"po 81 /7 id",			Instruction::CPU_X64},
					{"CMP",				"WORD r/m16,imm8",			"po 83 /7 ib",			Instruction::CPU_8086},
					{"CMP",				"DWORD r/m32,imm8",			"po 83 /7 ib",			Instruction::CPU_386},
					{"CMP",				"QWORD r/m64,imm8",			"po 83 /7 ib",			Instruction::CPU_X64},
					{"CMP",				"AL,imm8",					"3C ib",				Instruction::CPU_8086},
					{"CMP",				"AX,imm16",					"po 3D iw",				Instruction::CPU_8086},
					{"CMP",				"EAX,imm32",				"po 3D id",				Instruction::CPU_386},
					{"CMP",				"RAX,imm32",				"po 3D id",				Instruction::CPU_X64},
					{"CMPEQPD",			"xmmreg,r/m128",			"66 0F C2 /r 00",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPEQPS",			"xmmreg,r/m128",			"0F C2 /r 00",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPEQSD",			"xmmreg,xmm64",				"p2 0F C2 /r 00",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPEQSS",			"xmmreg,xmm32",				"p3 0F C2 /r 00",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPLEPD",			"xmmreg,r/m128",			"66 0F C2 /r 02",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPLEPS",			"xmmreg,r/m128",			"0F C2 /r 02",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPLESD",			"xmmreg,xmm64",				"p2 0F C2 /r 02",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPLESS",			"xmmreg,xmm32",				"p3 0F C2 /r 02",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPLTPD",			"xmmreg,r/m128",			"66 0F C2 /r 01",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPLTPS",			"xmmreg,r/m128",			"0F C2 /r 01",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPLTSD",			"xmmreg,xmm64",				"p2 0F C2 /r 01",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPLTSS",			"xmmreg,xmm32",				"p3 0F C2 /r 01",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPNEQPD",		"xmmreg,r/m128",			"66 0F C2 /r 04",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPNEQPS",		"xmmreg,r/m128",			"0F C2 /r 04",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPNEQSD",		"xmmreg,xmm64",				"p2 0F C2 /r 04",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPNEQSS",		"xmmreg,xmm32",				"p3 0F C2 /r 04",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPNLEPD",		"xmmreg,r/m128",			"66 0F C2 /r 06",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPNLEPS",		"xmmreg,r/m128",			"0F C2 /r 06",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPNLESD",		"xmmreg,xmm64",				"p2 0F C2 /r 06",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPNLESS",		"xmmreg,xmm32",				"p3 0F C2 /r 06",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPNLTPD",		"xmmreg,r/m128",			"66 0F C2 /r 05",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPNLTPS",		"xmmreg,r/m128",			"0F C2 /r 05",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPNLTSD",		"xmmreg,xmm64",				"p2 0F C2 /r 05",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPNLTSS",		"xmmreg,xmm32",				"p3 0F C2 /r 05",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPORDPD",		"xmmreg,r/m128",			"66 0F C2 /r 07",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPORDPS",		"xmmreg,r/m128",			"0F C2 /r 07",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPORDSD",		"xmmreg,xmm64",				"p2 0F C2 /r 07",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPORDSS",		"xmmreg,xmm32",				"p3 0F C2 /r 07",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPPD",			"xmmreg,r/m128,imm8",		"66 0F C2 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPPS",			"xmmreg,r/m128,imm8",		"0F C2 /r ib",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPSB",			"",							"A6",					Instruction::CPU_8086},
					{"CMPSD",			"",							"po A7",				Instruction::CPU_386},
					{"CMPSD",			"xmmreg,xmm64,imm8",		"p2 0F C2 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPSQ",			"",							"po A7",				Instruction::CPU_X64},
					{"CMPSS",			"xmmreg,xmm32,imm8",		"p3 0F C2 /r ib",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPSW",			"",							"po A7",				Instruction::CPU_8086},
					{"CMPUNORDPD",		"xmmreg,r/m128",			"66 0F C2 /r 03",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPUNORDPS",		"xmmreg,r/m128",			"0F C2 /r 03",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPUNORDSD",		"xmmreg,xmm64",				"p2 0F C2 /r 03",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
					{"CMPUNORDSS",		"xmmreg,xmm32",				"p3 0F C2 /r 03",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
					{"CMPXCHG",			"r/m8,reg8",				"0F B0 /r",				Instruction::CPU_PENTIUM},
					{"CMPXCHG",			"r/m16,reg16",				"po 0F B1 /r",			Instruction::CPU_PENTIUM},
					{"CMPXCHG",			"r/m32,reg32",				"po 0F B1 /r",			Instruction::CPU_PENTIUM},
					{"CMPXCHG",			"r/m64,reg64",				"po 0F B1 /r",			Instruction::CPU_X64},
					//	{"CMPXCHG486",		"r/m8,reg8",				"0F A6 /r",				Instruction::CPU_486 | Instruction::CPU_UNDOC},
					//	{"CMPXCHG486",		"r/m16,reg16",				"po 0F A7 /r",			Instruction::CPU_486 | Instruction::CPU_UNDOC},
					//	{"CMPXCHG486",		"r/m32,reg32",				"po 0F A7 /r",			Instruction::CPU_486 | Instruction::CPU_UNDOC},
						{"CMPXCHG16B",		"mem",						"0F C7 /1",				Instruction::CPU_X64},
						{"CMPXCHG8B",		"mem",						"0F C7 /1",				Instruction::CPU_PENTIUM},
						{"COMISD",			"xmmreg,xmm64",				"66 0F 2F /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"COMISS",			"xmmreg,xmm32",				"0F 2F /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
						{"CPUID",			"",							"0F A2",				Instruction::CPU_PENTIUM},
						{"CQO",				"",							"po 99",				Instruction::CPU_X64},
						{"CVTDQ2PD",		"xmmreg,xmm64",				"p3 0F E6 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTDQ2PS",		"xmmreg,r/m128",			"0F 5B /r",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTPD2DQ",		"xmmreg,r/m128",			"p2 0F E6 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTPD2PI",		"mmreg,r/m128",				"66 0F 2D /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTPD2PS",		"xmmreg,r/m128",			"66 0F 5A /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTPI2PD",		"xmmreg,mm64",				"66 0F 2A /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTPI2PS",		"xmmreg,mm64",				"0F 2A /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
						{"CVTPS2DQ",		"xmmreg,r/m128",			"66 0F 5B /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTPS2PD",		"xmmreg,xmm64",				"0F 5A /r",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTPS2PI",		"mmreg,xmm64",				"0F 2D /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
						{"CVTSD2SI",		"reg32,xmm64",				"p2 0F 2D /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTSI2SD",		"xmmreg,r/m32",				"p2 0F 2A /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTSI2SS",		"xmmreg,r/m32",				"p3 0F 2A /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
						{"CVTSS2SD",		"xmmreg,xmm32",				"p3 0F 5A /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTSS2SI",		"reg32,xmm32",				"p3 0F 2D /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
						{"CVTTPD2DQ",		"xmmreg,r/m128",			"66 0F E6 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTTPD2PI",		"mmreg,r/m128",				"66 0F 2C /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTTPS2DQ",		"xmmreg,r/m128",			"p3 0F 5B /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTTPS2PI",		"mmreg,xmm64",				"0F 2C /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
						{"CVTTSD2SI",		"reg32,xmm64",				"p2 0F 2C /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
						{"CVTTSS2SI",		"reg32,xmm32",				"p3 0F 2C /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
						{"CWD",				"",							"po 99",				Instruction::CPU_8086},
						{"CWDE",			"",							"po 98",				Instruction::CPU_386},
						{"DAA",				"",							"27",					Instruction::CPU_8086 | Instruction::CPU_INVALID64},
						{"DAS",				"",							"2F",					Instruction::CPU_8086 | Instruction::CPU_INVALID64},
						{"DB",				"",							"p1 ib"},				// Special 'instruction', indicated by the 'p1' prefix
						{"DB",				"imm8",						"p1 ib"},				// Special 'instruction', indicated by the 'p1' prefix
						{"DB",				"mem",						"p1 01"},				// Special 'instruction', indicated by the 'p1' prefix
						{"DD",				"",							"p1 id"},				// Special 'instruction', indicated by the 'p1' prefix
						{"DD",				"imm32",					"p1 id"},				// Special 'instruction', indicated by the 'p1' prefix
						{"DD",				"mem",						"p1 04"},				// Special 'instruction', indicated by the 'p1' prefix
						//	{"DEC",				"reg16",					"po 48 +r",				Instruction::CPU_8086 | Instruction::CPU_INVALID64},
						//	{"DEC",				"reg32",					"po 48 +r",				Instruction::CPU_386 | Instruction::CPU_INVALID64},
							{"DEC",				"BYTE r/m8",				"FE /1",				Instruction::CPU_8086},
							{"DEC",				"WORD r/m16",				"po FF /1",				Instruction::CPU_8086},
							{"DEC",				"DWORD r/m32",				"po FF /1",				Instruction::CPU_386},
							{"DEC",				"QWORD r/m64",				"po FF /1",				Instruction::CPU_X64},
							{"DIV",				"BYTE r/m8",				"F6 /6",				Instruction::CPU_8086},
							{"DIV",				"WORD r/m16",				"po F7 /6",				Instruction::CPU_8086},
							{"DIV",				"DWORD r/m32",				"po F7 /6",				Instruction::CPU_386},
							{"DIV",				"QWORD r/m64",				"po F7 /6",				Instruction::CPU_X64},
							{"DIVPD",			"xmmreg,r/m128",			"66 0F 5E /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
							{"DIVPS",			"xmmreg,r/m128",			"0F 5E /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
							{"DIVSD",			"xmmreg,xmm64",				"p2 0F 5E /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
							{"DIVSS",			"xmmreg,xmm32",				"p3 0F 5E /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
							{"DPPD",			"xmmreg,r/m128,imm8",		"66 0F 3A 41 /r ib",	Instruction::CPU_SSE4_1},
							{"DPPS",			"xmmreg,r/m128,imm8",		"66 0F 3A 40 /r ib",	Instruction::CPU_SSE4_1},
							{"DW",				"",							"p1 iw"},				// Special 'instruction', indicated by the 'p1' prefix
							{"DW",				"imm16",					"p1 iw"},				// Special 'instruction', indicated by the 'p1' prefix
							{"DW",				"mem",						"p1 02"},				// Special 'instruction', indicated by the 'p1' prefix
							{"EMMS",			"",							"0F 77",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
							{"EXTRACTPS",		"r/m32,xmmreg,imm8",		"66 0F 3A 17 /r ib",	Instruction::CPU_SSE4_1},
							{"EXTRACTPS",		"reg64,xmmreg,imm8",		"66 0F 3A 17 /r ib",	Instruction::CPU_SSE4_1 | Instruction::CPU_X64},
							//	{"ENTER",			"imm,imm",					"C8 iw ib",				Instruction::CPU_186},
								{"F2XM1",			"",							"D9 F0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
								{"FABS",			"",							"D9 E1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
								{"FADD",			"DWORD mem32",				"D8 /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
								{"FADD",			"QWORD mem64",				"DC /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
								{"FADD",			"fpureg",					"D8 C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
								{"FADD",			"ST0,fpureg",				"D8 C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
								//	{"FADD",			"TO fpureg",				"DC C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
									{"FADD",			"fpureg,ST0",				"DC C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
									{"FADDP",			"",							"DE C1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
									{"FADDP",			"fpureg",					"DE C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
									{"FADDP",			"fpureg,ST0",				"DE C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
									//	{"FBLD",			"mem80",					"DF /4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
									//	{"FBSTP",			"mem80",					"DF /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FCHS",			"",							"D9 E0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FCLEX",			"",							"9B DB E2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FCMOVB",			"fpureg",					"DA C0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVB",			"ST0,fpureg",				"DA C0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVBE",			"fpureg",					"DA D0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVBE",			"ST0,fpureg",				"DA D0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVE",			"fpureg",					"DA C8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVE",			"ST0,fpureg",				"DA C8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVNB",			"fpureg",					"DB C0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVNB",			"ST0,fpureg",				"DB C0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVNBE",		"fpureg",					"DB D0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVNBE",		"ST0,fpureg",				"DB D0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVNE",			"fpureg",					"DB C8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVNE",			"ST0,fpureg",				"DB C8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVNU",			"fpureg",					"DB D8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVNU",			"ST0,fpureg",				"DB D8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVU",			"fpureg",					"DA D8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCMOVU",			"ST0,fpureg",				"DA D8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCOM",			"DWORD mem32",				"D8 /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FCOM",			"QWORD mem64",				"DC /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FCOM",			"fpureg",					"D8 D0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FCOM",			"ST0,fpureg",				"D8 D0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FCOMI",			"fpureg",					"DB F0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCOMI",			"ST0,fpureg",				"DB F0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCOMIP",			"fpureg",					"DF F0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCOMIP",			"ST0,fpureg",				"DF F0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
										{"FCOMP",			"DWORD mem32",				"D8 /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FCOMP",			"QWORD mem64",				"DC /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FCOMP",			"fpureg",					"D8 D8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FCOMP",			"ST0,fpureg",				"D8 D8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FCOMPP",			"",							"DE D9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FCOS",			"",							"D9 FF",				Instruction::CPU_386 | Instruction::CPU_FPU},
										{"FDECSTP",			"",							"D9 F6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FDISI",			"",							"9B DB E1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FDIV",			"DWORD mem32",				"D8 /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FDIV",			"QWORD mem64",				"DC /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FDIV",			"fpureg",					"D8 F0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										{"FDIV",			"ST0,fpureg",				"D8 F0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
										//	{"FDIV",			"TO fpureg",				"DC F8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
											{"FDIV",			"fpureg,ST0",				"DC F8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
											{"FDIVP",			"",							"DE F9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
											{"FDIVP",			"fpureg",					"DE F8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
											{"FDIVP",			"fpureg,ST0",				"DE F8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
											{"FDIVR",			"DWORD mem32",				"D8 /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
											{"FDIVR",			"QWORD mem64",				"DC /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
											{"FDIVR",			"fpureg",					"D8 F8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
											{"FDIVR",			"ST0,fpureg",				"D8 F8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
											//	{"FDIVR",			"TO fpureg",				"DC F0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
												{"FDIVR",			"fpureg,ST0",				"DC F0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
												{"FDIVRP",			"",							"DE F1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
												{"FDIVRP",			"fpureg",					"DE F0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
												{"FDIVRP",			"fpureg,ST0",				"DE F0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
												{"FEMMS",			"",							"0F 0E",				Instruction::CPU_3DNOW},
												{"FENI",			"",							"9B DB E0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
												{"FFREE",			"fpureg",					"DD C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
												//	{"FFREEP",			"fpureg",					"DF C0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU | Instruction::CPU_UNDOC},
													{"FIADD",			"WORD mem16",				"DE /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FIADD",			"DWORD mem32",				"DA /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FICOM",			"WORD mem16",				"DE /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FICOM",			"DWORD mem32",				"DA /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FICOMP",			"WORD mem16",				"DE /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FICOMP",			"DWORD mem32",				"DA /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FIDIV",			"WORD mem16",				"DE /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FIDIV",			"DWORD mem32",				"DA /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FIDIVR",			"WORD mem16",				"DE /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FIDIVR",			"DWORD mem32",				"DA /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FILD",			"WORD mem16",				"DF /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FILD",			"DWORD mem32",				"DB /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FILD",			"QWORD mem64",				"DF /5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FIMUL",			"WORD mem16",				"DE /1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FIMUL",			"DWORD mem32",				"DA /1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FINCSTP",			"",							"D9 F7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FINIT",			"",							"9B DB E3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FIST",			"WORD mem16",				"DF /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FIST",			"DWORD mem32",				"DB /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FISTP",			"WORD mem16",				"DF /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FISTP",			"DWORD mem32",				"DB /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FISTP",			"QWORD mem64",				"DF /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FISTTP",			"WORD mem16",				"DF /1",				Instruction::CPU_PNI},
													{"FISTTP",			"DWORD mem32",				"DB /1",				Instruction::CPU_PNI},
													{"FISTTP",			"QWORD mem64",				"DD /1",				Instruction::CPU_PNI},
													{"FISUB",			"WORD mem16",				"DE /4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FISUB",			"DWORD mem32",				"DA /4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FISUBR",			"WORD mem16",				"DE /5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FISUBR",			"DWORD mem32",				"DA /5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FLD",				"DWORD mem32",				"D9 /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													{"FLD",				"QWORD mem64",				"DD /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
													//	{"FLD",				"mem80",					"DB /5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FLD",				"fpureg",					"D9 C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FLD1",			"",							"D9 E8",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FLDCW",			"mem16",					"D9 /5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FLDENV",			"mem",						"D9 /4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FLDL2E",			"",							"D9 EA",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FLDL2T",			"",							"D9 E9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FLDLG2",			"",							"D9 EC",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FLDLN2",			"",							"D9 ED",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FLDPI",			"",							"D9 EB",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FLDZ",			"",							"D9 EE",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FMUL",			"DWORD mem32",				"D8 /1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FMUL",			"QWORD mem64",				"DC /1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FMUL",			"",							"D8 C9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FMUL",			"fpureg",					"D8 C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														{"FMUL",			"ST0,fpureg",				"D8 C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
														//	{"FMUL",			"TO fpureg",				"DC C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FMUL",			"fpureg,ST0",				"DC C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FMULP",			"fpureg",					"DE C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FMULP",			"fpureg,ST0",				"DE C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FMULP",			"",							"DE C9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FNCLEX",			"",							"DB E2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FNDISI",			"",							"DB E1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FNENI",			"",							"DB E0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FNINIT",			"",							"DB E3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FNOP",			"",							"D9 D0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FNSAVE",			"mem",						"DD /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FNSTCW",			"mem16",					"D9 /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FNSTENV",			"mem",						"D9 /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FNSTSW",			"mem16",					"DD /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FNSTSW",			"AX",						"DF E0",				Instruction::CPU_286 | Instruction::CPU_FPU},
															{"FPATAN",			"",							"D9 F3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FPREM",			"",							"D9 F8",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FPREM1",			"",							"D9 F5",				Instruction::CPU_386 | Instruction::CPU_FPU},
															{"FPTAN",			"",							"D9 F2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FRNDINT",			"",							"D9 FC",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FRSTOR",			"mem",						"DD /4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FSAVE",			"mem",						"9B DD /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FSCALE",			"",							"D9 FD",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FSETPM",			"",							"DB E4",				Instruction::CPU_286 | Instruction::CPU_FPU},
															{"FSIN",			"",							"D9 FE",				Instruction::CPU_386 | Instruction::CPU_FPU},
															{"FSINCOS",			"",							"D9 FB",				Instruction::CPU_386 | Instruction::CPU_FPU},
															{"FSQRT",			"",							"D9 FA",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FST",				"DWORD mem32",				"D9 /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FST",				"QWORD mem64",				"DD /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FST",				"fpureg",					"DD D0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FSTCW",			"mem16",					"9B D9 /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FSTENV",			"mem",						"9B D9 /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FSTP",			"DWORD mem32",				"D9 /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															{"FSTP",			"QWORD mem64",				"DD /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
															//	{"FSTP",			"mem80",					"DB /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																{"FSTP",			"fpureg",					"DD D8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																{"FSTSW",			"mem16",					"9B DD /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																{"FSTSW",			"AX",						"9B DF E0",				Instruction::CPU_286 | Instruction::CPU_FPU},
																{"FSUB",			"DWORD mem32",				"D8 /4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																{"FSUB",			"QWORD mem64",				"DC /4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																{"FSUB",			"fpureg",					"D8 E0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																{"FSUB",			"ST0,fpureg",				"D8 E0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																//	{"FSUB",			"TO fpureg",				"DC E8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																	{"FSUB",			"fpureg,ST0",				"DC E8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																	{"FSUBP",			"",							"DE E9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																	{"FSUBP",			"fpureg",					"DE E8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																	{"FSUBP",			"fpureg,ST0",				"DE E8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																	{"FSUBR",			"DWORD mem32",				"D8 /5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																	{"FSUBR",			"QWORD mem64",				"DC /5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																	{"FSUBR",			"fpureg",					"D8 E8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																	{"FSUBR",			"ST0,fpureg",				"D8 E8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																	//	{"FSUBR",			"TO fpureg",				"DC E0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																		{"FSUBR",			"fpureg,ST0",				"DC E0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																		{"FSUBRP",			"",							"DE E1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																		{"FSUBRP",			"fpureg",					"DE E0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																		{"FSUBRP",			"fpureg,ST0",				"DE E0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																		{"FTST",			"",							"D9 E4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																		{"FUCOM",			"fpureg",					"DD E0 +r",				Instruction::CPU_386 | Instruction::CPU_FPU},
																		{"FUCOM",			"ST0,fpureg",				"DD E0 +r",				Instruction::CPU_386 | Instruction::CPU_FPU},
																		{"FUCOMI",			"fpureg",					"DB E8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
																		{"FUCOMI",			"ST0,fpureg",				"DB E8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
																		{"FUCOMIP",			"fpureg",					"DF E8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
																		{"FUCOMIP",			"ST0,fpureg",				"DF E8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
																		{"FUCOMP",			"fpureg",					"DD E8 +r",				Instruction::CPU_386 | Instruction::CPU_FPU},
																		{"FUCOMP",			"ST0,fpureg",				"DD E8 +r",				Instruction::CPU_386 | Instruction::CPU_FPU},
																		{"FUCOMPP",			"",							"DA E9",				Instruction::CPU_386 | Instruction::CPU_FPU},
																		{"FWAIT",			"",							"9B",					Instruction::CPU_8086},
																		{"FXAM",			"",							"D9 E5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																		{"FXCH",			"",							"D9 C9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																		{"FXCH",			"fpureg",					"D9 C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																		{"FXCH",			"fpureg,ST0",				"D9 C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																		{"FXCH",			"ST0,fpureg",				"D9 C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																		//	{"FXRSTOR",			"mem",						"0F AE /1",				Instruction::CPU_P6 | Instruction::CPU_SSE | Instruction::CPU_FPU},
																		//	{"FXSAVE",			"mem",						"0F AE /0",				Instruction::CPU_P6 | Instruction::CPU_SSE | Instruction::CPU_FPU},
																			{"FXTRACT",			"",							"D9 F4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																			{"FYL2X",			"",							"D9 F1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																			{"FYL2XP1",			"",							"D9 F9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
																			{"HADDPD",			"xmmreg,r/m128",			"66 0F 7C /r",			Instruction::CPU_PNI},
																			{"HADDPS",			"xmmreg,r/m128",			"p2 0F 7C /r",			Instruction::CPU_PNI},
																			{"HLT",				"",							"F4",					Instruction::CPU_8086},
																			{"HSUBPD",			"xmmreg,r/m128",			"66 0F 7D /r",			Instruction::CPU_PNI},
																			{"HSUBPS",			"xmmreg,r/m128",			"p2 0F 7D /r",			Instruction::CPU_PNI},
																			//	{"IBTS",			"r/m16,reg16",				"po 0F A7 /r",			Instruction::CPU_386 | Instruction::CPU_UNDOC},
																			//	{"IBTS",			"r/m32,reg32",				"po 0F A7 /r",			Instruction::CPU_386 | Instruction::CPU_UNDOC},
																				{"IDIV",			"BYTE r/m8",				"F6 /7",				Instruction::CPU_8086},
																				{"IDIV",			"WORD r/m16",				"po F7 /7",				Instruction::CPU_8086},
																				{"IDIV",			"DWORD r/m32",				"po F7 /7",				Instruction::CPU_386},
																				{"IDIV",			"QWORD r/m64",				"po F7 /7",				Instruction::CPU_X64},
																				{"IMUL",			"BYTE r/m8",				"F6 /5",				Instruction::CPU_8086},
																				{"IMUL",			"WORD r/m16",				"po F7 /5",				Instruction::CPU_8086},
																				{"IMUL",			"DWORD r/m32",				"po F7 /5",				Instruction::CPU_386},
																				{"IMUL",			"QWORD r/m64",				"po F7 /5",				Instruction::CPU_X64},
																				{"IMUL",			"reg16,r/m16",				"po 0F AF /r",			Instruction::CPU_386},
																				{"IMUL",			"reg32,r/m32",				"po 0F AF /r",			Instruction::CPU_386},
																				{"IMUL",			"reg64,r/m64",				"po 0F AF /r",			Instruction::CPU_X64},
																				{"IMUL",			"reg16,imm8",				"po 6B /r ib",			Instruction::CPU_286},
																				{"IMUL",			"reg32,imm8",				"po 6B /r ib",			Instruction::CPU_386},
																				{"IMUL",			"reg64,imm8",				"po 6B /r ib",			Instruction::CPU_X64},
																				{"IMUL",			"reg16,imm16",				"po 69 /r iw",			Instruction::CPU_286},
																				{"IMUL",			"reg32,imm32",				"po 69 /r id",			Instruction::CPU_386},
																				{"IMUL",			"reg64,imm32",				"po 69 /r id",			Instruction::CPU_X64},
																				{"IMUL",			"reg16,r/m16,imm8",			"po 6B /r ib",			Instruction::CPU_286},
																				{"IMUL",			"reg32,r/m32,imm8",			"po 6B /r ib",			Instruction::CPU_386},
																				{"IMUL",			"reg64,r/m64,imm8",			"po 6B /r ib",			Instruction::CPU_X64},
																				{"IMUL",			"reg16,r/m16,imm16",		"po 69 /r iw",			Instruction::CPU_286},
																				{"IMUL",			"reg32,r/m32,imm32",		"po 69 /r id",			Instruction::CPU_386},
																				{"IMUL",			"reg64,r/m64,imm32",		"po 69 /r id",			Instruction::CPU_X64},
																				{"IN",				"AL,imm8",					"E4 ib",				Instruction::CPU_8086},
																				{"IN",				"AX,imm8",					"po E5 ib",				Instruction::CPU_8086},
																				{"IN",				"EAX,imm8",					"po E5 ib",				Instruction::CPU_386},
																				{"IN",				"AL,DX",					"EC",					Instruction::CPU_8086},
																				{"IN",				"AX,DX",					"po ED",				Instruction::CPU_8086},
																				{"IN",				"EAX,DX",					"po ED",				Instruction::CPU_386},
																				//	{"INC",				"reg16",					"po 40 +r",				Instruction::CPU_8086 | Instruction::CPU_INVALID64},
																				//	{"INC",				"reg32",					"po 40 +r",				Instruction::CPU_386 | Instruction::CPU_INVALID64},
																					{"INC",				"BYTE r/m8",				"FE /0",				Instruction::CPU_8086},
																					{"INC",				"WORD r/m16",				"po FF /0",				Instruction::CPU_8086},
																					{"INC",				"DWORD r/m32",				"po FF /0",				Instruction::CPU_386},
																					{"INC",				"QWORD r/m64",				"po FF /0",				Instruction::CPU_X64},
																					{"INSB",			"",							"6C",					Instruction::CPU_186},
																					{"INSD",			"",							"po 6D",				Instruction::CPU_386},
																					{"INSERTPS",		"xmmreg,r/m32,imm8",		"66 0F 3A 21 /r ib",	Instruction::CPU_SSE4_1},
																					{"INSW",			"",							"po 6D",				Instruction::CPU_186},
																					//	{"INT",				"imm8",						"CD ib",				Instruction::CPU_8086},
																					//	{"INT1",			"",							"F1",					Instruction::CPU_P6 | Instruction::CPU_UNDOC},
																					//	{"ICEBP",			"",							"F1",					Instruction::CPU_P6 | Instruction::CPU_UNDOC},
																					//	{"INT01",			"",							"F1",					Instruction::CPU_P6 | Instruction::CPU_UNDOC},
																						{"INT03",			"",							"CC",					Instruction::CPU_8086},
																						{"INT3",			"",							"CC",					Instruction::CPU_8086},
																						{"INTO",			"",							"CE",					Instruction::CPU_8086 | Instruction::CPU_INVALID64},
																						//	{"INVD",			"",							"0F 08",				Instruction::CPU_486},
																						//	{"INVLPG",			"mem",						"0F 01 /7",				Instruction::CPU_486},
																						//	{"IRET",			"",							"CF",					Instruction::CPU_8086},
																						//	{"IRETW",			"",							"po CF",				Instruction::CPU_8086},
																						//	{"IRETD",			"",							"po CF",				Instruction::CPU_386},
																							{"JA",				"NEAR imm8",				"77 -b",				Instruction::CPU_8086},
																							{"JA",				"imm",						"0F 87 -i",				Instruction::CPU_386},
																							{"JAE",				"NEAR imm8",				"73 -b",				Instruction::CPU_8086},
																							{"JAE",				"imm",						"0F 83 -i",				Instruction::CPU_386},
																							{"JB",				"NEAR imm8",				"72 -b",				Instruction::CPU_8086},
																							{"JB",				"imm",						"0F 82 -i",				Instruction::CPU_386},
																							{"JBE",				"NEAR imm8",				"76 -b",				Instruction::CPU_8086},
																							{"JBE",				"imm",						"0F 86 -i",				Instruction::CPU_386},
																							{"JC",				"NEAR imm8",				"72 -b",				Instruction::CPU_8086},
																							{"JC",				"imm",						"0F 82 -i",				Instruction::CPU_386},
																							{"JCXZ",			"NEAR imm8",				"po E3 -b",				Instruction::CPU_8086},
																							{"JE",				"NEAR imm8",				"74 -b",				Instruction::CPU_8086},
																							{"JE",				"imm",						"0F 84 -i",				Instruction::CPU_386},
																							{"JECXZ",			"NEAR imm8",				"po E3 -b",				Instruction::CPU_386},
																							{"JG",				"NEAR imm8",				"7F -b",				Instruction::CPU_8086},
																							{"JG",				"imm",						"0F 8F -i",				Instruction::CPU_386},
																							{"JGE",				"NEAR imm8",				"7D -b",				Instruction::CPU_8086},
																							{"JGE",				"imm",						"0F 8D -i",				Instruction::CPU_386},
																							{"JL",				"NEAR imm8",				"7C -b",				Instruction::CPU_8086},
																							{"JL",				"imm",						"0F 8C -i",				Instruction::CPU_386},
																							{"JLE",				"NEAR imm8",				"7E -b",				Instruction::CPU_8086},
																							{"JLE",				"imm",						"0F 8E -i",				Instruction::CPU_386},
																							{"JMP",				"imm",						"E9 -i",				Instruction::CPU_8086},
																							{"JMP",				"NEAR imm8",				"EB -b",				Instruction::CPU_8086},
																							//	{"JMP",				"imm:imm16",				"po EA iw iw",			Instruction::CPU_8086 | Instruction::CPU_INVALID64},
																							//	{"JMP",				"imm:imm32",				"po EA id iw",			Instruction::CPU_386 | Instruction::CPU_INVALID64},
																								{"JMP",				"mem",						"po FF /5",				Instruction::CPU_8086},
																								//	{"JMP",				"FAR mem",					"po FF /5",				Instruction::CPU_386},
																									{"JMP",				"WORD r/m16",				"po FF /4",				Instruction::CPU_8086},
																									{"JMP",				"DWORD r/m32",				"po FF /4",				Instruction::CPU_386},
																									{"JMP",				"QWORD r/m64",				"po FF /4",				Instruction::CPU_X64},
																									{"JNA",				"NEAR imm8",				"76 -b",				Instruction::CPU_8086},
																									{"JNA",				"imm",						"0F 86 -i",				Instruction::CPU_386},
																									{"JNAE",			"NEAR imm8",				"72 -b",				Instruction::CPU_8086},
																									{"JNAE",			"imm",						"0F 82 -i",				Instruction::CPU_386},
																									{"JNB",				"NEAR imm8",				"73 -b",				Instruction::CPU_8086},
																									{"JNB",				"imm",						"0F 83 -i",				Instruction::CPU_386},
																									{"JNBE",			"NEAR imm8",				"77 -b",				Instruction::CPU_8086},
																									{"JNBE",			"imm",						"0F 87 -i",				Instruction::CPU_386},
																									{"JNC",				"NEAR imm8",				"73 -b",				Instruction::CPU_8086},
																									{"JNC",				"imm",						"0F 83 -i",				Instruction::CPU_386},
																									{"JNE",				"NEAR imm8",				"75 -b",				Instruction::CPU_8086},
																									{"JNE",				"imm",						"0F 85 -i",				Instruction::CPU_386},
																									{"JNG",				"NEAR imm8",				"7E -b",				Instruction::CPU_8086},
																									{"JNG",				"imm",						"0F 8E -i",				Instruction::CPU_386},
																									{"JNGE",			"NEAR imm8",				"7C -b",				Instruction::CPU_8086},
																									{"JNGE",			"imm",						"0F 8C -i",				Instruction::CPU_386},
																									{"JNL",				"NEAR imm8",				"7D -b",				Instruction::CPU_8086},
																									{"JNL",				"imm",						"0F 8D -i",				Instruction::CPU_386},
																									{"JNLE",			"NEAR imm8",				"7F -b",				Instruction::CPU_8086},
																									{"JNLE",			"imm",						"0F 8F -i",				Instruction::CPU_386},
																									{"JNO",				"NEAR imm8",				"71 -b",				Instruction::CPU_8086},
																									{"JNO",				"imm",						"0F 81 -i",				Instruction::CPU_386},
																									{"JNP",				"NEAR imm8",				"7B -b",				Instruction::CPU_8086},
																									{"JNP",				"imm",						"0F 8B -i",				Instruction::CPU_386},
																									{"JNS",				"NEAR imm8",				"79 -b",				Instruction::CPU_8086},
																									{"JNS",				"imm",						"0F 89 -i",				Instruction::CPU_386},
																									{"JNZ",				"NEAR imm8",				"75 -b",				Instruction::CPU_8086},
																									{"JNZ",				"imm",						"0F 85 -i",				Instruction::CPU_386},
																									{"JO",				"NEAR imm8",				"70 -b",				Instruction::CPU_8086},
																									{"JO",				"imm",						"0F 80 -i",				Instruction::CPU_386},
																									{"JP",				"NEAR imm8",				"7A -b",				Instruction::CPU_8086},
																									{"JP",				"imm",						"0F 8A -i",				Instruction::CPU_386},
																									{"JPE",				"NEAR imm8",				"7A -b",				Instruction::CPU_8086},
																									{"JPE",				"imm",						"0F 8A -i",				Instruction::CPU_386},
																									{"JPO",				"NEAR imm8",				"7B -b",				Instruction::CPU_8086},
																									{"JPO",				"imm",						"0F 8B -i",				Instruction::CPU_386},
																									{"JRCXZ",			"NEAR imm8",				"po E3 -b",				Instruction::CPU_X64},
																									{"JS",				"NEAR imm8",				"78 -b",				Instruction::CPU_8086},
																									{"JS",				"imm",						"0F 88 -i",				Instruction::CPU_386},
																									{"JZ",				"NEAR imm8",				"74 -b",				Instruction::CPU_8086},
																									{"JZ",				"imm",						"0F 84 -i",				Instruction::CPU_386},
																									{"LAHF",			"",							"9F",					Instruction::CPU_8086},
																									//	{"LAR",				"reg16,r/m16",				"po 0F 02 /r",			Instruction::CPU_286 | Instruction::CPU_PRIV},
																									//	{"LAR",				"reg32,r/m32",				"po 0F 02 /r",			Instruction::CPU_286 | Instruction::CPU_PRIV},
																										{"LDDQU",			"xmmreg,mem",				"p2 0F F0 /r",			Instruction::CPU_PNI},
																										{"LDMXCSR",			"mem32",					"0F AE /2",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																										{"LDS",				"reg16,mem",				"po C5 /r",				Instruction::CPU_8086 | Instruction::CPU_INVALID64},
																										{"LDS",				"reg32,mem",				"po C5 /r",				Instruction::CPU_8086 | Instruction::CPU_INVALID64},
																										{"LEA",				"reg16,mem",				"po 8D /r",				Instruction::CPU_8086},
																										{"LEA",				"reg32,mem",				"po 8D /r",				Instruction::CPU_386},
																										{"LEA",				"reg64,mem",				"po 8D /r",				Instruction::CPU_X64},
																										{"LEAVE",			"",							"C9",					Instruction::CPU_186},
																										{"LES",				"reg16,mem",				"po C4 /r",				Instruction::CPU_8086 | Instruction::CPU_INVALID64},
																										{"LES",				"reg32,mem",				"po C4 /r",				Instruction::CPU_8086 | Instruction::CPU_INVALID64},
																										{"LFENCE",			"",							"0F AE E8",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																										{"LFS",				"reg16,mem",				"po 0F B4 /r",			Instruction::CPU_386},
																										{"LFS",				"reg32,mem",				"po 0F B4 /r",			Instruction::CPU_386},
																										{"LGS",				"reg16,mem",				"po 0F B5 /r",			Instruction::CPU_386},
																										{"LGS",				"reg32,mem",				"po 0F B5 /r",			Instruction::CPU_386},
																										//	{"LGDT",			"mem",						"0F 01 /2",				Instruction::CPU_286 | Instruction::CPU_PRIV},
																										//	{"LIDT",			"mem",						"0F 01 /3",				Instruction::CPU_286 | Instruction::CPU_PRIV},
																										//	{"LLDT",			"r/m16",					"0F 00 /2",				Instruction::CPU_286 | Instruction::CPU_PRIV},
																										//	{"LMSW",			"r/m16",					"0F 01 /6",				Instruction::CPU_286 | Instruction::CPU_PRIV},
																										//	{"LOADALL",			"",							"0F 07",				Instruction::CPU_386 | Instruction::CPU_UNDOC},
																										//	{"LOADALL286",		"",							"0F 05",				Instruction::CPU_286 | Instruction::CPU_UNDOC},
																											{"LOCK ADC",		"mem8,reg8",				"p0 10 /r",				Instruction::CPU_8086},
																											{"LOCK ADC",		"mem16,reg16",				"p0 po 11 /r",			Instruction::CPU_8086},
																											{"LOCK ADC",		"mem32,reg32",				"p0 po 11 /r",			Instruction::CPU_386},
																											{"LOCK ADC",		"BYTE mem8,imm8",			"p0 80 /2 ib",			Instruction::CPU_8086},
																											{"LOCK ADC",		"WORD mem16,imm16",			"p0 po 81 /2 iw",		Instruction::CPU_8086},
																											{"LOCK ADC",		"DWORD mem32,imm32",		"p0 po 81 /2 id",		Instruction::CPU_386},
																											{"LOCK ADC",		"WORD mem16,imm8",			"p0 po 83 /2 ib",		Instruction::CPU_8086},
																											{"LOCK ADC",		"DWORD mem32,imm8",			"p0 po 83 /2 ib",		Instruction::CPU_386},
																											{"LOCK ADD",		"mem8,reg8",				"p0 00 /r",				Instruction::CPU_8086},
																											{"LOCK ADD",		"mem16,reg16",				"p0 po 01 /r",			Instruction::CPU_8086},
																											{"LOCK ADD",		"mem32,reg32",				"p0 po 01 /r",			Instruction::CPU_386},
																											{"LOCK ADD",		"BYTE mem8,imm8",			"p0 80 /0 ib",			Instruction::CPU_8086},
																											{"LOCK ADD",		"WORD mem16,imm16",			"p0 po 81 /0 iw",		Instruction::CPU_8086},
																											{"LOCK ADD",		"DWORD mem32,imm32",		"p0 po 81 /0 id",		Instruction::CPU_386},
																											{"LOCK ADD",		"WORD mem16,imm8",			"p0 po 83 /0 ib",		Instruction::CPU_8086},
																											{"LOCK ADD",		"DWORD mem32,imm8",			"p0 po 83 /0 ib",		Instruction::CPU_386},
																											{"LOCK AND",		"mem8,reg8",				"p0 20 /r",				Instruction::CPU_8086},
																											{"LOCK AND",		"mem16,reg16",				"p0 po 21 /r",			Instruction::CPU_8086},
																											{"LOCK AND",		"mem32,reg32",				"p0 po 21 /r",			Instruction::CPU_386},
																											{"LOCK AND",		"BYTE mem8,imm8",			"80 /4 ib",				Instruction::CPU_8086},
																											{"LOCK AND",		"WORD mem16,imm16",			"p0 po 81 /4 iw",		Instruction::CPU_8086},
																											{"LOCK AND",		"DWORD mem32,imm32",		"p0 po 81 /4 id",		Instruction::CPU_386},
																											{"LOCK AND",		"WORD mem16,imm8",			"p0 po 83 /4 ib",		Instruction::CPU_8086},
																											{"LOCK AND",		"DWORD mem32,imm8",			"p0 po 83 /4 ib",		Instruction::CPU_386},
																											{"LOCK BTC",		"mem16,reg16",				"p0 po 0F BB /r",		Instruction::CPU_386},
																											{"LOCK BTC",		"mem32,reg32",				"p0 po 0F BB /r",		Instruction::CPU_386},
																											{"LOCK BTC",		"WORD mem16,imm8",			"p0 po 0F BA /7 ib",	Instruction::CPU_386},
																											{"LOCK BTC",		"DWORD mem32,imm8",			"p0 po 0F BA /7 ib",	Instruction::CPU_386},
																											{"LOCK BTR",		"mem16,reg16",				"p0 po 0F B3 /r",		Instruction::CPU_386},
																											{"LOCK BTR",		"mem32,reg32",				"p0 po 0F B3 /r",		Instruction::CPU_386},
																											{"LOCK BTR",		"WORD mem16,imm8",			"p0 po 0F BA /6 ib",	Instruction::CPU_386},
																											{"LOCK BTR",		"DWORD mem32,imm8",			"p0 po 0F BA /6 ib",	Instruction::CPU_386},
																											{"LOCK BTS",		"mem16,reg16",				"p0 po 0F AB /r",		Instruction::CPU_386},
																											{"LOCK BTS",		"mem32,reg32",				"p0 po 0F AB /r",		Instruction::CPU_386},
																											{"LOCK BTS",		"WORD mem16,imm8",			"p0 po 0F BA /5 ib",	Instruction::CPU_386},
																											{"LOCK BTS",		"DWORD mem32,imm8",			"p0 po 0F BA /5 ib",	Instruction::CPU_386},
																											{"LOCK CMPXCHG",	"mem8,reg8",				"p0 0F B0 /r",			Instruction::CPU_PENTIUM},
																											{"LOCK CMPXCHG",	"mem16,reg16",				"p0 po 0F B1 /r",		Instruction::CPU_PENTIUM},
																											{"LOCK CMPXCHG",	"mem32,reg32",				"p0 po 0F B1 /r",		Instruction::CPU_PENTIUM},
																											{"LOCK CMPXCHG8B",	"mem",						"p0 0F C7 /1",			Instruction::CPU_PENTIUM},
																											{"LOCK DEC",		"BYTE mem8",				"p0 FE /1",				Instruction::CPU_8086},
																											{"LOCK DEC",		"WORD mem16",				"p0 po FF /1",			Instruction::CPU_8086},
																											{"LOCK DEC",		"DWORD mem32",				"p0 po FF /1",			Instruction::CPU_386},
																											{"LOCK INC",		"BYTE mem8",				"p0 FE /0",				Instruction::CPU_8086},
																											{"LOCK INC",		"WORD mem16",				"p0 po FF /0",			Instruction::CPU_8086},
																											{"LOCK INC",		"DWORD mem32",				"p0 po FF /0",			Instruction::CPU_386},
																											{"LOCK NEG",		"BYTE mem8",				"p0 F6 /3",				Instruction::CPU_8086},
																											{"LOCK NEG",		"WORD mem16",				"p0 po F7 /3",			Instruction::CPU_8086},
																											{"LOCK NEG",		"DWORD mem32",				"p0 po F7 /3",			Instruction::CPU_386},
																											{"LOCK NOT",		"BYTE mem8",				"p0 F6 /2",				Instruction::CPU_8086},
																											{"LOCK NOT",		"WORD mem16",				"p0 po F7 /2",			Instruction::CPU_8086},
																											{"LOCK NOT",		"DWORD mem32",				"p0 po F7 /2",			Instruction::CPU_386},
																											{"LOCK OR",			"mem8,reg8",				"p0 08 /r",				Instruction::CPU_8086},
																											{"LOCK OR",			"mem16,reg16",				"p0 po 09 /r",			Instruction::CPU_8086},
																											{"LOCK OR",			"mem32,reg32",				"p0 po 09 /r",			Instruction::CPU_386},
																											{"LOCK OR",			"BYTE mem8,imm8",			"p0 80 /1 ib",			Instruction::CPU_8086},
																											{"LOCK OR",			"WORD mem16,imm16",			"p0 po 81 /1 iw",		Instruction::CPU_8086},
																											{"LOCK OR",			"DWORD mem32,imm32",		"p0 po 81 /1 id",		Instruction::CPU_386},
																											{"LOCK OR",			"WORD mem16,imm8",			"p0 po 83 /1 ib",		Instruction::CPU_8086},
																											{"LOCK OR",			"DWORD mem32,imm8",			"p0 po 83 /1 ib",		Instruction::CPU_386},
																											{"LOCK SBB",		"mem8,reg8",				"p0 18 /r",				Instruction::CPU_8086},
																											{"LOCK SBB",		"mem16,reg16",				"p0 po 19 /r",			Instruction::CPU_8086},
																											{"LOCK SBB",		"mem32,reg32",				"p0 po 19 /r",			Instruction::CPU_386},
																											{"LOCK SBB",		"BYTE mem8,imm8",			"p0 80 /3 ib",			Instruction::CPU_8086},
																											{"LOCK SBB",		"WORD mem16,imm16",			"p0 po 81 /3 iw",		Instruction::CPU_8086},
																											{"LOCK SBB",		"DWORD mem32,imm32",		"p0 po 81 /3 id",		Instruction::CPU_386},
																											{"LOCK SBB",		"WORD mem16,imm8",			"p0 po 83 /3 ib",		Instruction::CPU_8086},
																											{"LOCK SBB",		"DWORD mem32,imm8",			"p0 po 83 /3 ib",		Instruction::CPU_8086},
																											{"LOCK SUB",		"BYTE mem8,imm8",			"p0 80 /5 ib",			Instruction::CPU_8086},
																											{"LOCK SUB",		"WORD mem16,imm16",			"p0 po 81 /5 iw",		Instruction::CPU_8086},
																											{"LOCK SUB",		"DWORD mem32,imm32",		"p0 po 81 /5 id",		Instruction::CPU_386},
																											{"LOCK SUB",		"WORD mem16,imm8",			"p0 po 83 /5 ib",		Instruction::CPU_8086},
																											{"LOCK SUB",		"DWORD mem32,imm8",			"p0 po 83 /5 ib",		Instruction::CPU_386},
																											{"LOCK SUB",		"mem8,reg8",				"p0 28 /r",				Instruction::CPU_8086},
																											{"LOCK SUB",		"mem16,reg16",				"p0 po 29 /r",			Instruction::CPU_8086},
																											{"LOCK SUB",		"mem32,reg32",				"p0 po 29 /r",			Instruction::CPU_386},
																											{"LOCK XADD",		"mem8,reg8",				"p0 0F C0 /r",			Instruction::CPU_486},
																											{"LOCK XADD",		"mem16,reg16",				"p0 po 0F C1 /r",		Instruction::CPU_486},
																											{"LOCK XADD",		"mem32,reg32",				"p0 po 0F C1 /r",		Instruction::CPU_486},
																											{"LOCK XCHG",		"mem8,reg8",				"p0 86 /r",				Instruction::CPU_8086},
																											{"LOCK XCHG",		"mem16,reg16",				"p0 po 87 /r",			Instruction::CPU_8086},
																											{"LOCK XCHG",		"mem32,reg32",				"p0 po 87 /r",			Instruction::CPU_386},
																											{"LOCK XOR",		"mem8,reg8",				"p0 30 /r",				Instruction::CPU_8086},
																											{"LOCK XOR",		"mem16,reg16",				"p0 po 31 /r",			Instruction::CPU_8086},
																											{"LOCK XOR",		"mem32,reg32",				"p0 po 31 /r",			Instruction::CPU_386},
																											{"LOCK XOR",		"BYTE mem8,imm8",			"p0 80 /6 ib",			Instruction::CPU_8086},
																											{"LOCK XOR",		"WORD mem16,imm16",			"p0 po 81 /6 iw",		Instruction::CPU_8086},
																											{"LOCK XOR",		"DWORD mem32,imm32",		"p0 po 81 /6 id",		Instruction::CPU_386},
																											{"LOCK XOR",		"WORD mem16,imm8",			"p0 po 83 /6 ib",		Instruction::CPU_8086},
																											{"LOCK XOR",		"DWORD mem32,imm8",			"p0 po 83 /6 ib",		Instruction::CPU_386},
																											{"LODSB",			"",							"AC",					Instruction::CPU_8086},
																											{"LODSD",			"",							"po AD",				Instruction::CPU_386},
																											{"LODSQ",			"",							"po AD",				Instruction::CPU_X64},
																											{"LODSW",			"",							"po AD",				Instruction::CPU_8086},
																											{"LOOP",			"imm",						"E2 -b",				Instruction::CPU_8086},
																											{"LOOP",			"imm,CX",					"pa E2 -b",				Instruction::CPU_8086},
																											{"LOOP",			"imm,ECX",					"pa E2 -b",				Instruction::CPU_386},
																											{"LOOPE",			"imm",						"E1 -b",				Instruction::CPU_8086},
																											{"LOOPE",			"imm,CX",					"pa E1 -b",				Instruction::CPU_8086},
																											{"LOOPE",			"imm,ECX",					"pa E1 -b",				Instruction::CPU_386},
																											{"LOOPNE",			"imm",						"E0 -b",				Instruction::CPU_8086},
																											{"LOOPNE",			"imm,CX",					"pa E0 -b",				Instruction::CPU_8086},
																											{"LOOPNE",			"imm,ECX",					"pa E0 -b",				Instruction::CPU_386},
																											{"LOOPNZ",			"imm",						"E0 -b",				Instruction::CPU_8086},
																											{"LOOPNZ",			"imm,CX",					"pa E0 -b",				Instruction::CPU_8086},
																											{"LOOPNZ",			"imm,ECX",					"pa E0 -b",				Instruction::CPU_386},
																											{"LOOPZ",			"imm",						"E1 -b",				Instruction::CPU_8086},
																											{"LOOPZ",			"imm,CX",					"pa E1 -b",				Instruction::CPU_8086},
																											{"LOOPZ",			"imm,ECX",					"pa E1 -b",				Instruction::CPU_386},
																											//	{"LSL",				"reg16,r/m16",				"po 0F 03 /r",			Instruction::CPU_286 | Instruction::CPU_PRIV},
																											//	{"LSL",				"reg32,r/m32",				"po 0F 03 /r",			Instruction::CPU_286 | Instruction::CPU_PRIV},
																											//	{"LTR",				"r/m16",					"0F 00 /3",				Instruction::CPU_286 | Instruction::CPU_PRIV},
																												{"LSS",				"reg16,mem",				"po 0F B2 /r",			Instruction::CPU_386},
																												{"LSS",				"reg32,mem",				"po 0F B2 /r",			Instruction::CPU_386},
																												{"MASKMOVDQU",		"xmmreg,xmmreg",			"66 0F F7 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																												{"MASKMOVQ",		"mmreg,mmreg",				"0F F7 /r",				Instruction::CPU_KATMAI},
																												{"MAXPD",			"xmmreg,r/m128",			"66 0F 5F /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																												{"MAXPS",			"xmmreg,r/m128",			"0F 5F /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																												{"MAXSD",			"xmmreg,xmm64",				"p2 0F 5F /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																												{"MAXSS",			"xmmreg,xmm32",				"p3 0F 5F /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																												{"MFENCE",			"",							"0F AE F0",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																												{"MINPD",			"xmmreg,r/m128",			"66 0F 5D /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																												{"MINPS",			"xmmreg,r/m128",			"0F 5D /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																												{"MINSD",			"xmmreg,xmm64",				"p2 0F 5D /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																												{"MINSS",			"xmmreg,xmm32",				"p3 0F 5D /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																												{"MONITOR",			"",							"0F 01 C8",				Instruction::CPU_PNI},
																												{"MOV",				"r/m8,reg8",				"88 /r",				Instruction::CPU_8086},
																												{"MOV",				"r/m16,reg16",				"po 89 /r",				Instruction::CPU_8086},
																												{"MOV",				"r/m32,reg32",				"po 89 /r",				Instruction::CPU_386},
																												{"MOV",				"r/m64,reg64",				"po 89 /r",				Instruction::CPU_X64},
																												{"MOV",				"reg8,r/m8",				"8A /r",				Instruction::CPU_8086},
																												{"MOV",				"reg16,r/m16",				"po 8B /r",				Instruction::CPU_8086},
																												{"MOV",				"reg32,r/m32",				"po 8B /r",				Instruction::CPU_386},
																												{"MOV",				"reg64,r/m64",				"po 8B /r",				Instruction::CPU_X64},
																												{"MOV",				"reg8,imm8",				"B0 +r ib",				Instruction::CPU_8086},
																												{"MOV",				"reg16,imm16",				"po B8 +r iw",			Instruction::CPU_8086},
																												{"MOV",				"reg32,imm32",				"po B8 +r id",			Instruction::CPU_386},
																												//	{"MOV",				"reg64,imm64",				"po B8 +r iq",			Instruction::CPU_X64},   // FIXME: imm64 unimplemented
																													{"MOV",				"BYTE r/m8,imm8",			"C6 /0 ib",				Instruction::CPU_8086},
																													{"MOV",				"WORD r/m16,imm16",			"po C7 /0 iw",			Instruction::CPU_8086},
																													{"MOV",				"DWORD r/m32,imm32",		"po C7 /0 id",			Instruction::CPU_386},
																													{"MOV",				"QWORD r/m64,imm32",		"po C7 /0 id",			Instruction::CPU_X64},
																													//	{"MOV",				"AL,memoffs8",				"A0 id",				Instruction::CPU_8086},
																													//	{"MOV",				"AX,memoffs16",				"po A1 id",				Instruction::CPU_8086},
																													//	{"MOV",				"EAX,memoffs32",			"po A1 id",				Instruction::CPU_386},
																													//	{"MOV",				"memoffs8,AL",				"A2 id",				Instruction::CPU_8086},
																													//	{"MOV",				"memoffs16,AX",				"po A3 id",				Instruction::CPU_8086},
																													//	{"MOV",				"memoffs32,EAX",			"po A3 id",				Instruction::CPU_386},
																													//	{"MOV",				"r/m16,segreg",				"po 8C /r",				Instruction::CPU_8086},
																													//	{"MOV",				"r/m32,segreg",				"po 8C /r",				Instruction::CPU_386},
																													//	{"MOV",				"segreg,r/m16",				"po 8E /r",				Instruction::CPU_8086},
																													//	{"MOV",				"segreg,r/m32",				"po 8E /r",				Instruction::CPU_386},
																													//	{"MOV",				"reg32,CR",					"0F 20 /r",				Instruction::CPU_386},
																													//	{"MOV",				"reg32,DR",					"0F 21 /r",				Instruction::CPU_386},
																													//	{"MOV",				"reg32,TR",					"0F 24 /r",				Instruction::CPU_386},
																													//	{"MOV",				"CR,reg32",					"0F 22 /r",				Instruction::CPU_386},
																													//	{"MOV",				"DR,reg32",					"0F 23 /r",				Instruction::CPU_386},
																													//	{"MOV",				"TR,reg32",					"0F 26 /r",				Instruction::CPU_386},
																														{"MOVAPD",			"xmmreg,r/m128",			"66 0F 28 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVAPD",			"r/m128,xmmreg",			"66 0F 29 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVAPS",			"xmmreg,r/m128",			"0F 28 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVAPS",			"r/m128,xmmreg",			"0F 29 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVD",			"mmreg,r/m32",				"0F 6E /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"MOVD",			"mmreg,r/m64",				"0F 6E /r",				Instruction::CPU_X64},
																														{"MOVD",			"r/m32,mmreg",				"0F 7E /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"MOVD",			"r/m64,mmreg",				"0F 7E /r",				Instruction::CPU_X64},
																														{"MOVD",			"xmmreg,r/m32",				"66 0F 6E /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVD",			"xmmreg,r/m64",				"66 0F 6E /r",			Instruction::CPU_X64},
																														{"MOVD",			"r/m32,xmmreg",				"66 0F 7E /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVD",			"r/m64,xmmreg",				"66 0F 7E /r",			Instruction::CPU_X64},
																														{"MOVDDUP",			"xmmreg,r/m128",			"p2 0F 12 /r",			Instruction::CPU_PNI},
																														{"MOVDQ2Q",			"mmreg,xmmreg",				"p2 0F D6 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVDQA",			"xmmreg,r/m128",			"66 0F 6F /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVDQA",			"r/m128,xmmreg",			"66 0F 7F /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVDQU",			"xmmreg,r/m128",			"p3 0F 6F /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVDQU",			"r/m128,xmmreg",			"p3 0F 7F /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVHLPS",			"xmmreg,xmmreg",			"0F 12 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVHPD",			"xmmreg,mem64",				"66 0F 16 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVHPD",			"mem64,xmmreg",				"66 0F 17 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVHPS",			"xmmreg,mem64",				"0F 16 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVHPS",			"mem64,xmmreg",				"0F 17 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVHPS",			"xmmreg,xmmreg",			"0F 16 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVLHPS",			"xmmreg,xmmreg",			"0F 16 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVLPD",			"xmmreg,mem64",				"66 0F 12 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVLPD",			"mem64,xmmreg",				"66 0F 13 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVLPS",			"xmmreg,mem64",				"0F 12 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVLPS",			"mem64,xmmreg",				"0F 13 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVMSKPD",		"reg32,xmmreg",				"66 0F 50 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVMSKPS",		"reg32,xmmreg",				"0F 50 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVNTDQ",			"mem128,xmmreg",			"66 0F E7 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVNTDQA",		"xmmreg,mem128",			"66 0F 38 2A /r",		Instruction::CPU_SSE4_1},
																														{"MOVNTI",			"mem32,reg32",				"0F C3 /r",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVNTI",			"mem64,reg64",				"0F C3 /r",				Instruction::CPU_X64},
																														{"MOVNTPD",			"mem128,xmmreg",			"66 0F 2B /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVNTPS",			"mem128,xmmreg",			"0F 2B /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVNTQ",			"mem64,mmreg",				"0F E7 /r",				Instruction::CPU_KATMAI},
																														{"MOVQ",			"mmreg,mm64",				"0F 6F /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"MOVQ",			"mm64,mmreg",				"0F 7F /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"MOVQ",			"xmmreg,xmm64",				"p3 0F 7E /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVQ",			"xmm64,xmmreg",				"66 0F D6 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVQ2DQ",			"xmmreg,mmreg",				"p3 0F D6 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVSB",			"",							"A4",					Instruction::CPU_8086},
																														{"MOVSD",			"",							"po A5",				Instruction::CPU_386},
																														{"MOVSD",			"xmmreg,xmm64",				"p2 0F 10 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVSD",			"xmm64,xmmreg",				"p2 0F 11 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVSHDUP",		"xmmreg,r/m128",			"p3 0F 16 /r",			Instruction::CPU_PNI},
																														{"MOVSLDUP",		"xmmreg,r/m128",			"p3 0F 12 /r",			Instruction::CPU_PNI},
																														{"MOVSQ",			"",							"po A5",				Instruction::CPU_X64},
																														{"MOVSS",			"xmmreg,xmm32",				"p3 0F 10 /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVSS",			"xmm32,xmmreg",				"p3 0F 11 /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVSW",			"",							"po A5",				Instruction::CPU_8086},
																														{"MOVSX",			"reg16,r/m8",				"po 0F BE /r",			Instruction::CPU_386},
																														{"MOVSX",			"reg32,r/m8",				"po 0F BE /r",			Instruction::CPU_386},
																														{"MOVSX",			"reg64,r/m8",				"po 0F BE /r",			Instruction::CPU_X64},
																														{"MOVSX",			"reg32,r/m16",				"po 0F BF /r",			Instruction::CPU_386},
																														{"MOVSX",			"reg64,r/m16",				"po 0F BF /r",			Instruction::CPU_X64},
																														{"MOVSXD",			"reg64,r/m32",				"po 63 /r",				Instruction::CPU_X64},
																														{"MOVUPD",			"xmmreg,r/m128",			"66 0F 10 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVUPD",			"r/m128,xmmreg",			"66 0F 11 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MOVUPS",			"xmmreg,r/m128",			"0F 10 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVUPS",			"r/m128,xmmreg",			"0F 11 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MOVZX",			"reg16,r/m8",				"po 0F B6 /r",			Instruction::CPU_386},
																														{"MOVZX",			"reg32,r/m8",				"po 0F B6 /r",			Instruction::CPU_386},
																														{"MOVZX",			"reg64,r/m8",				"po 0F B6 /r",			Instruction::CPU_X64},
																														{"MOVZX",			"reg32,r/m16",				"po 0F B7 /r",			Instruction::CPU_386},
																														{"MOVZX",			"reg64,r/m16",				"po 0F B7 /r",			Instruction::CPU_X64},
																														{"MPSADBW",			"xmmreg,r/m128,imm8",		"66 0F 3A 42 /r ib",	Instruction::CPU_SSE4_1},
																														{"MUL",				"BYTE r/m8",				"F6 /4",				Instruction::CPU_8086},
																														{"MUL",				"WORD r/m16",				"po F7 /4",				Instruction::CPU_8086},
																														{"MUL",				"DWORD r/m32",				"po F7 /4",				Instruction::CPU_386},
																														{"MUL",				"QWORD r/m64",				"po F7 /4",				Instruction::CPU_X64},
																														{"MULPD",			"xmmreg,r/m128",			"66 0F 59 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MULPS",			"xmmreg,r/m128",			"0F 59 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MULSD",			"xmmreg,xmm64",				"p2 0F 59 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"MULSS",			"xmmreg,xmm32",				"p3 0F 59 /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"MWAIT",			"",							"0F 01 C9",				Instruction::CPU_PNI},
																														{"NEG",				"BYTE r/m8",				"F6 /3",				Instruction::CPU_8086},
																														{"NEG",				"WORD r/m16",				"po F7 /3",				Instruction::CPU_8086},
																														{"NEG",				"DWORD r/m32",				"po F7 /3",				Instruction::CPU_386},
																														{"NEG",				"QWORD r/m64",				"po F7 /3",				Instruction::CPU_X64},
																														{"NOP",				"",							"90",					Instruction::CPU_8086},
																														{"NOT",				"BYTE r/m8",				"F6 /2",				Instruction::CPU_8086},
																														{"NOT",				"WORD r/m16",				"po F7 /2",				Instruction::CPU_8086},
																														{"NOT",				"DWORD r/m32",				"po F7 /2",				Instruction::CPU_386},
																														{"NOT",				"QWORD r/m64",				"po F7 /2",				Instruction::CPU_X64},
																														{"NULL",			"",							""},					// Empty 'instruction', placeholder for annotations
																														{"OR",				"r/m8,reg8",				"08 /r",				Instruction::CPU_8086},
																														{"OR",				"r/m16,reg16",				"po 09 /r",				Instruction::CPU_8086},
																														{"OR",				"r/m32,reg32",				"po 09 /r",				Instruction::CPU_386},
																														{"OR",				"r/m64,reg64",				"po 09 /r",				Instruction::CPU_X64},
																														{"OR",				"reg8,r/m8",				"0A /r",				Instruction::CPU_8086},
																														{"OR",				"reg16,r/m16",				"po 0B /r",				Instruction::CPU_8086},
																														{"OR",				"reg32,r/m32",				"po 0B /r",				Instruction::CPU_386},
																														{"OR",				"reg64,r/m64",				"po 0B /r",				Instruction::CPU_X64},
																														{"OR",				"BYTE r/m8,imm8",			"80 /1 ib",				Instruction::CPU_8086},
																														{"OR",				"WORD r/m16,imm16",			"po 81 /1 iw",			Instruction::CPU_8086},
																														{"OR",				"DWORD r/m32,imm32",		"po 81 /1 id",			Instruction::CPU_386},
																														{"OR",				"QWORD r/m64,imm32",		"po 81 /1 id",			Instruction::CPU_X64},
																														{"OR",				"WORD r/m16,imm8",			"po 83 /1 ib",			Instruction::CPU_8086},
																														{"OR",				"DWORD r/m32,imm8",			"po 83 /1 ib",			Instruction::CPU_386},
																														{"OR",				"QWORD r/m64,imm8",			"po 83 /1 ib",			Instruction::CPU_X64},
																														{"OR",				"AL,imm8",					"0C ib",				Instruction::CPU_8086},
																														{"OR",				"AX,imm16",					"po 0D iw",				Instruction::CPU_8086},
																														{"OR",				"EAX,imm32",				"po 0D id",				Instruction::CPU_386},
																														{"OR",				"RAX,imm32",				"po 0D id",				Instruction::CPU_X64},
																														{"ORPD",			"xmmreg,r/m128",			"66 0F 56 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"ORPS",			"xmmreg,r/m128",			"0F 56 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																														{"OUT",				"imm8,AL",					"E6 ib",				Instruction::CPU_8086},
																														{"OUT",				"imm8,AX",					"po E7 ib",				Instruction::CPU_8086},
																														{"OUT",				"imm8,EAX",					"po E7 ib",				Instruction::CPU_386},
																														{"OUT",				"DX,AL",					"EE",					Instruction::CPU_8086},
																														{"OUT",				"DX,AX",					"po EF",				Instruction::CPU_8086},
																														{"OUT",				"DX,EAX",					"po EF",				Instruction::CPU_386},
																														{"OUTSB",			"",							"6E",					Instruction::CPU_186},
																														{"OUTSD",			"",							"po 6F",				Instruction::CPU_386},
																														{"OUTSW",			"",							"po 6F",				Instruction::CPU_186},
																														{"PABSB",			"mmreg,mm64",				"0F 38 1c /r",			Instruction::CPU_SSSE3},
																														{"PABSB",			"xmmreg,r/m128",			"66 0f 38 1c /r",		Instruction::CPU_SSSE3},
																														{"PABSD",			"mmreg,mm64",				"0F 38 0a /r",			Instruction::CPU_SSSE3},
																														{"PABSD",			"xmmreg,r/m128",			"66 0f 38 1e /r",		Instruction::CPU_SSSE3},
																														{"PABSW",			"mmreg,mm64",				"0F 38 1e /r",			Instruction::CPU_SSSE3},
																														{"PABSW",			"xmmreg,r/m128",			"66 0f 38 1d /r",		Instruction::CPU_SSSE3},
																														{"PACKSSDW",		"mmreg,mm64",				"0F 6B /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PACKSSDW",		"xmmreg,r/m128",			"66 0F 6B /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PACKSSWB",		"mmreg,mm64",				"0F 63 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PACKSSWB",		"xmmreg,r/m128",			"66 0F 63 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PACKUSDW",		"xmmreg,r/m128",			"66 0F 38 2B /r",		Instruction::CPU_SSE4_1},
																														{"PACKUSWB",		"mmreg,mm64",				"0F 67 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PACKUSWB",		"xmmreg,r/m128",			"66 0F 67 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PADDB",			"mmreg,mm64",				"0F FC /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PADDB",			"xmmreg,r/m128",			"66 0F FC /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PADDD",			"mmreg,mm64",				"0F FE /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PADDD",			"xmmreg,r/m128",			"66 0F FE /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PADDQ",			"mmreg,mm64",				"0F D4 /r",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PADDQ",			"xmmreg,r/m128",			"66 0F D4 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PADDSB",			"mmreg,mm64",				"0F EC /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PADDSB",			"xmmreg,r/m128",			"66 0F EC /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PADDSIW",			"mmreg,mm64",				"0F 51 /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
																														{"PADDSW",			"mmreg,mm64",				"0F ED /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PADDSW",			"xmmreg,r/m128",			"66 0F ED /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PADDUSB",			"mmreg,mm64",				"0F DC /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PADDUSB",			"xmmreg,r/m128",			"66 0F DC /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PADDUSW",			"mmreg,mm64",				"0F DD /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PADDUSW",			"xmmreg,r/m128",			"66 0F DD /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PADDW",			"mmreg,mm64",				"0F FD /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PADDW",			"xmmreg,r/m128",			"66 0F FD /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PALIGNR",			"mmreg,mm64,imm8",			"0F 3a 0f ib",			Instruction::CPU_SSSE3},
																														{"PALIGNR",			"xmmreg,r/m128,imm8",		"66 0f 38 1c ib",		Instruction::CPU_SSSE3},
																														{"PAND",			"mmreg,mm64",				"0F DB /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PAND",			"xmmreg,r/m128",			"66 0F DB /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PANDN",			"mmreg,mm64",				"0F DF /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PANDN",			"xmmreg,r/m128",			"66 0F DF /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PAUSE",			"",							"p3 90",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PAVEB",			"mmreg,mm64",				"0F 50 /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
																														{"PAVGB",			"mmreg,mm64",				"0F E0 /r",				Instruction::CPU_KATMAI},
																														{"PAVGB",			"xmmreg,r/m128",			"66 0F E0 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PAVGUSB",			"mmreg,mm64",				"0F 0F /r BF",			Instruction::CPU_3DNOW},
																														{"PAVGW",			"mmreg,mm64",				"0F E3 /r",				Instruction::CPU_KATMAI},
																														{"PAVGW",			"xmmreg,r/m128",			"66 0F E3 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PBLENDVB",		"xmmreg,r/m128",			"66 0F 38 10 /r",		Instruction::CPU_SSE4_1},
																														{"PBLENDW",			"xmmreg,r/m128,imm8",		"66 0F 3A 0E /r ib",	Instruction::CPU_SSE4_1},
																														{"PCMPEQB",			"mmreg,mm64",				"0F 74 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PCMPEQB",			"xmmreg,r/m128",			"66 0F 74 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PCMPEQD",			"mmreg,mm64",				"0F 76 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PCMPEQD",			"xmmreg,r/m128",			"66 0F 76 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PCMPEQQ",			"xmmreg,r/m128",			"66 0F 38 29 /r",		Instruction::CPU_SSE4_1},
																														{"PCMPEQW",			"mmreg,mm64",				"0F 75 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PCMPEQW",			"xmmreg,r/m128",			"66 0F 75 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PCMPESTRI",		"xmmreg,r/m128,imm8",		"66 0F 3A 61 /r ib",	Instruction::CPU_SSE4_2},
																														{"PCMPESTRM",		"xmmreg,r/m128,imm8",		"66 0F 3A 60 /r ib",	Instruction::CPU_SSE4_2},
																														{"PCMPGTB",			"mmreg,mm64",				"0F 64 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PCMPGTB",			"xmmreg,r/m128",			"66 0F 64 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PCMPGTD",			"mmreg,mm64",				"0F 66 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PCMPGTD",			"xmmreg,r/m128",			"66 0F 66 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PCMPGTQ",			"xmmreg,r/m128",			"66 0F 38 37 /r",		Instruction::CPU_SSE4_1},
																														{"PCMPGTW",			"mmreg,mm64",				"0F 65 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PCMPGTW",			"xmmreg,r/m128",			"66 0F 65 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PCMPISTRI",		"xmmreg,r/m128,imm8",		"66 0F 3A 63 /r ib",	Instruction::CPU_SSE4_2},
																														{"PCMPISTRM",		"xmmreg,r/m128,imm8",		"66 0F 3A 62 /r ib",	Instruction::CPU_SSE4_2},
																														{"PDISTIB",			"mmreg,mem64",				"0F 54 /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
																														{"PEXTRB",			"reg32,xmmreg,imm8",		"66 0F 3A 14 /r ib",	Instruction::CPU_SSE4_1},
																														{"PEXTRB",			"mem8,xmmreg,imm8",			"66 0F 3A 14 /r ib",	Instruction::CPU_SSE4_1},
																														{"PEXTRB",			"reg64,xmmreg,imm8",		"66 0F 3A 14 /r ib",	Instruction::CPU_SSE4_1 | Instruction::CPU_X64},
																														{"PEXTRD",			"r/m32,xmmreg,imm8",		"66 0F 3A 16 /r ib",	Instruction::CPU_SSE4_1},
																														{"PEXTRD",			"r/m64,xmmreg,imm8",		"66 0F 3A 16 /r ib",	Instruction::CPU_SSE4_1 | Instruction::CPU_X64},
																														{"PEXTRW",			"mem16,xmmreg,imm8",		"66 0F 3A 15 /r ib",	Instruction::CPU_SSE4_1},
																														{"PEXTRW",			"reg64,xmmreg,imm8",		"66 0F 3A 15 /r ib",	Instruction::CPU_SSE4_1 | Instruction::CPU_X64},
																														{"PEXTRW",			"reg32,mmreg,imm8",			"0F C5 /r ib",			Instruction::CPU_KATMAI},
																														{"PEXTRW",			"reg32,xmmreg,imm8",		"66 0F C5 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PF2ID",			"mmreg,mm64",				"0F 0F /r 1D",			Instruction::CPU_3DNOW},
																														{"PF2IW",			"mmreg,mm64",				"0F 0F /r 1C",			Instruction::CPU_ATHLON},
																														{"PFACC",			"mmreg,mm64",				"0F 0F /r AE",			Instruction::CPU_3DNOW},
																														{"PFADD",			"mmreg,mm64",				"0F 0F /r 9E",			Instruction::CPU_3DNOW},
																														{"PFCMPEQ",			"mmreg,mm64",				"0F 0F /r B0",			Instruction::CPU_3DNOW},
																														{"PFCMPGE",			"mmreg,mm64",				"0F 0F /r 90",			Instruction::CPU_3DNOW},
																														{"PFCMPGT",			"mmreg,mm64",				"0F 0F /r A0",			Instruction::CPU_3DNOW},
																														{"PFMAX",			"mmreg,mm64",				"0F 0F /r A4",			Instruction::CPU_3DNOW},
																														{"PFMIN",			"mmreg,mm64",				"0F 0F /r 94",			Instruction::CPU_3DNOW},
																														{"PFMUL",			"mmreg,mm64",				"0F 0F /r B4",			Instruction::CPU_3DNOW},
																														{"PFNACC",			"mmreg,mm64",				"0F 0F /r 8A",			Instruction::CPU_ATHLON},
																														{"PFPNACC",			"mmreg,mm64",				"0F 0F /r 8E",			Instruction::CPU_ATHLON},
																														{"PFRCP",			"mmreg,mm64",				"0F 0F /r 96",			Instruction::CPU_3DNOW},
																														{"PFRCPIT1",		"mmreg,mm64",				"0F 0F /r A6",			Instruction::CPU_3DNOW},
																														{"PFRCPIT2",		"mmreg,mm64",				"0F 0F /r B6",			Instruction::CPU_3DNOW},
																														{"PFRSQIT1",		"mmreg,mm64",				"0F 0F /r A7",			Instruction::CPU_3DNOW},
																														{"PFRSQRT",			"mmreg,mm64",				"0F 0F /r 97",			Instruction::CPU_3DNOW},
																														{"PFSUB",			"mmreg,mm64",				"0F 0F /r 9A",			Instruction::CPU_3DNOW},
																														{"PFSUBR",			"mmreg,mm64",				"0F 0F /r AA",			Instruction::CPU_3DNOW},
																														{"PHADDD",			"mmreg,mm64",				"0F 38 02 /r",			Instruction::CPU_SSSE3},
																														{"PHADDD",			"xmmreg,r/m128",			"66 0F 38 02 /r",		Instruction::CPU_SSSE3},
																														{"PHADDSW",			"mmreg,mm64",				"0F 38 03 /r",			Instruction::CPU_SSSE3},
																														{"PHADDSW",			"xmmreg,r/m128",			"66 0F 38 03 /r",		Instruction::CPU_SSSE3},
																														{"PHADDW",			"mmreg,mm64",				"0F 38 01 /r",			Instruction::CPU_SSSE3},
																														{"PHADDW",			"xmmreg,r/m128",			"66 0F 38 01 /r",		Instruction::CPU_SSSE3},
																														{"PHMINPOSUW",		"xmmreg,r/m128",			"66 0F 38 41 /r",		Instruction::CPU_SSE4_1},
																														{"PHSUBD",			"mmreg,mm64",				"0F 38 06 /r",			Instruction::CPU_SSSE3},
																														{"PHSUBD",			"xmmreg,r/m128",			"66 0F 38 06 /r",		Instruction::CPU_SSSE3},
																														{"PHSUBSW",			"mmreg,mm64",				"0F 38 07 /r",			Instruction::CPU_SSSE3},
																														{"PHSUBSW",			"xmmreg,r/m128",			"66 0F 38 07 /r",		Instruction::CPU_SSSE3},
																														{"PHSUBW",			"mmreg,mm64",				"0F 38 05 /r",			Instruction::CPU_SSSE3},
																														{"PHSUBW",			"xmmreg,r/m128",			"66 0F 38 05 /r",		Instruction::CPU_SSSE3},
																														{"PI2FD",			"mmreg,mm64",				"0F 0F /r 0D",			Instruction::CPU_3DNOW},
																														{"PI2FW",			"mmreg,mm64",				"0F 0F /r 0C",			Instruction::CPU_ATHLON},
																														{"PINSRB",			"xmmreg,reg32,imm8",		"66 0F 3A 20 /r ib",	Instruction::CPU_SSE4_1},
																														{"PINSRB",			"xmmreg,mem8,imm8",			"66 0F 3A 20 /r ib",	Instruction::CPU_SSE4_1},
																														{"PINSRD",			"xmmreg,r/m32,imm8",		"66 0F 3A 22 /r ib",	Instruction::CPU_SSE4_1},
																														{"PINSRQ",			"xmmreg,r/m64,imm8",		"66 0F 3A 22 /r ib",	Instruction::CPU_SSE4_1 | Instruction::CPU_X64},
																														{"PINSRW",			"mmreg,r/m16,imm8",			"0F C4 /r ib",			Instruction::CPU_KATMAI},
																														{"PINSRW",			"xmmreg,r/m16,imm8",		"66 0F C4 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PMACHRIW",		"mmreg,mem64",				"0F 5E /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
																														{"PMADDUBSW",		"mmreg,mm64",				"0F 38 04 /r",			Instruction::CPU_SSSE3},
																														{"PMADDUBSW",		"xmmreg,r/m128",			"66 0F 38 04 /r",		Instruction::CPU_SSSE3},
																														{"PMADDWD",			"mmreg,mm64",				"0F F5 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PMADDWD",			"xmmreg,r/m128",			"66 0F F5 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PMAGW",			"mmreg,mm64",				"0F 52 /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
																														{"PMAXSB",			"xmmreg,r/m128",			"66 0F 38 3C /r",		Instruction::CPU_SSE4_1},
																														{"PMAXSD",			"xmmreg,r/m128",			"66 0F 38 3D /r",		Instruction::CPU_SSE4_1},
																														{"PMAXSW",			"xmmreg,r/m128",			"66 0F EE /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PMAXSW",			"mmreg,mm64",				"0F EE /r",				Instruction::CPU_KATMAI},
																														{"PMAXUB",			"mmreg,mm64",				"0F DE /r",				Instruction::CPU_KATMAI},
																														{"PMAXUB",			"xmmreg,r/m128",			"66 0F DE /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PMINSB",			"xmmreg,r/m128",			"66 0F 38 38 /r",		Instruction::CPU_SSE4_1},
																														{"PMINSD",			"xmmreg,r/m128",			"66 0F 38 39 /r",		Instruction::CPU_SSE4_1},
																														{"PMINSW",			"mmreg,mm64",				"0F EA /r",				Instruction::CPU_KATMAI},
																														{"PMINSW",			"xmmreg,r/m128",			"66 0F EA /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PMINUB",			"mmreg,mm64",				"0F DA /r",				Instruction::CPU_KATMAI},
																														{"PMINUB",			"xmmreg,r/m128",			"66 0F DA /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PMINUD",			"xmmreg,r/m128",			"66 0F 38 3B /r",		Instruction::CPU_SSE4_1},
																														{"PMOVMSKB",		"reg32,mmreg",				"0F D7 /r",				Instruction::CPU_KATMAI},
																														{"PMOVMSKB",		"reg32,xmmreg",				"66 0F D7 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PMOVSXBD",		"xmmreg,xmmreg",			"66 0F 38 21 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVSXBD",		"xmmreg,mem32",				"66 0F 38 21 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVSXBQ",		"xmmreg,xmmreg",			"66 0F 38 22 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVSXBQ",		"xmmreg,mem16",				"66 0F 38 22 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVSXBW",		"xmmreg,xmmreg",			"66 0F 38 20 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVSXBW",		"xmmreg,mem64",				"66 0F 38 20 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVSXDQ",		"xmmreg,xmmreg",			"66 0F 38 25 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVSXDQ",		"xmmreg,mem64",				"66 0F 38 25 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVSXWD",		"xmmreg,xmmreg",			"66 0F 38 23 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVSXWD",		"xmmreg,mem64",				"66 0F 38 23 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVSXWQ",		"xmmreg,xmmreg",			"66 0F 38 24 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVSXWQ",		"xmmreg,mem32",				"66 0F 38 24 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVZXBD",		"xmmreg,xmmreg",			"66 0F 38 31 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVZXBD",		"xmmreg,mem32",				"66 0F 38 31 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVZXBQ",		"xmmreg,xmmreg",			"66 0F 38 32 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVZXBQ",		"xmmreg,mem16",				"66 0F 38 32 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVZXBW",		"xmmreg,xmmreg",			"66 0F 38 30 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVZXBW",		"xmmreg,mem64",				"66 0F 38 30 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVZXDQ",		"xmmreg,xmmreg",			"66 0F 38 35 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVZXDQ",		"xmmreg,mem64",				"66 0F 38 35 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVZXWD",		"xmmreg,xmmreg",			"66 0F 38 33 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVZXWD",		"xmmreg,mem64",				"66 0F 38 33 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVZXWQ",		"xmmreg,xmmreg",			"66 0F 38 34 /r",		Instruction::CPU_SSE4_1},
																														{"PMOVZXWQ",		"xmmreg,mem32",				"66 0F 38 34 /r",		Instruction::CPU_SSE4_1},
																														{"PMULDQ",			"xmmreg,r/m128",			"66 0F 38 28 /r",		Instruction::CPU_SSE4_1},
																														{"PMULHRIW",		"mmreg,mm64",				"0F 5D /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
																														{"PMULHRSW",		"mmreg,mm64",				"0F 38 0B /r",			Instruction::CPU_SSSE3 },
																														{"PMULHRSW",		"xmmreg,r/m128",			"66 0F 38 0B /r",		Instruction::CPU_SSSE3 },
																														{"PMULHRWA",		"mmreg,mm64",				"0F 0F /r B7",			Instruction::CPU_3DNOW},
																														{"PMULHRWC",		"mmreg,mm64",				"0F 59 /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
																														{"PMULHUW",			"mmreg,mm64",				"0F E4 /r",				Instruction::CPU_KATMAI},
																														{"PMULHUW",			"xmmreg,r/m128",			"66 0F E4 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PMULHW",			"mmreg,mm64",				"0F E5 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PMULHW",			"xmmreg,r/m128",			"66 0F E5 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PMULLD",			"xmmreg,r/m128",			"66 0F 38 40 /r",		Instruction::CPU_SSE4_1},
																														{"PMULLW",			"mmreg,mm64",				"0F D5 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																														{"PMULLW",			"xmmreg,r/m128",			"66 0F D5 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PMULUDQ",			"mmreg,mm64",				"0F F4 /r",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PMULUDQ",			"xmmreg,r/m128",			"66 0F F4 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																														{"PMVGEZB",			"mmreg,mem64",				"0F 5C /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
																														{"PMVLZB",			"mmreg,mem64",				"0F 5B /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
																														{"PMVNZB",			"mmreg,mem64",				"0F 5A /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
																														{"PMVZB",			"mmreg,mem64",				"0F 58 /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
																														{"POP",				"reg16",					"po 58 +r",				Instruction::CPU_8086},
																														{"POP",				"reg32",					"po 58 +r",				Instruction::CPU_386},
																														{"POP",				"reg64",					"po 58 +r",				Instruction::CPU_X64},
																														{"POP",				"WORD r/m16",				"po 8F /0",				Instruction::CPU_8086},
																														{"POP",				"DWORD r/m32",				"po 8F /0",				Instruction::CPU_386},
																														{"POP",				"QWORD r/m64",				"po 8F /0",				Instruction::CPU_X64},
																														//	{"POP",				"CS",						"0F",					Instruction::CPU_8086 | Instruction::CPU_UNDOC | Instruction::CPU_INVALID64},
																														//	{"POP",				"DS",						"1F",					Instruction::CPU_8086 | Instruction::CPU_INVALID64},
																														//	{"POP",				"ES",						"07",					Instruction::CPU_8086 | Instruction::CPU_INVALID64},
																														//	{"POP",				"SS",						"17",					Instruction::CPU_8086 | Instruction::CPU_INVALID64},
																														//	{"POP",				"FS",						"0F A1",				Instruction::CPU_386},
																														//	{"POP",				"GS",						"0F A9",				Instruction::CPU_386},
																															{"POPA",			"",							"61",					Instruction::CPU_186 | Instruction::CPU_INVALID64},
																															{"POPAD",			"",							"po 61",				Instruction::CPU_386 | Instruction::CPU_INVALID64},
																															{"POPAW",			"",							"po 61",				Instruction::CPU_186 | Instruction::CPU_INVALID64},
																															{"POPCNT",			"reg16,r/m16",				"F3 0F B8 /r",			Instruction::CPU_SSE4_2},
																															{"POPCNT",			"reg32,r/m32",				"F3 0F B8 /r",			Instruction::CPU_SSE4_2},
																															{"POPCNT",			"reg64,r/m64",				"F3 0F B8 /r",			Instruction::CPU_SSE4_2 | Instruction::CPU_X64},
																															{"POPF",			"",							"9D",					Instruction::CPU_186},
																															{"POPFD",			"",							"po 9D",				Instruction::CPU_386},
																															{"POPFQ",			"",							"po 9D",				Instruction::CPU_X64},
																															{"POPFW",			"",							"po 9D",				Instruction::CPU_186},
																															{"POR",				"mmreg,mm64",				"0F EB /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"POR",				"xmmreg,r/m128",			"66 0F EB /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PREFETCH",		"mem",						"0F 0D /0",				Instruction::CPU_3DNOW},
																															{"PREFETCHNTA",		"mem",						"0F 18 /0",				Instruction::CPU_KATMAI},
																															{"PREFETCHT0",		"mem",						"0F 18 /1",				Instruction::CPU_KATMAI},
																															{"PREFETCHT1",		"mem",						"0F 18 /2",				Instruction::CPU_KATMAI},
																															{"PREFETCHT2",		"mem",						"0F 18 /3",				Instruction::CPU_KATMAI},
																															{"PREFETCHW",		"mem",						"0F 0D /1",				Instruction::CPU_3DNOW},
																															{"PSADBW",			"mmreg,mm64",				"0F F6 /r",				Instruction::CPU_KATMAI},
																															{"PSADBW",			"xmmreg,r/m128",			"66 0F F6 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSHUFB",			"mmreg,mm64",				"0F 38 00 /r",			Instruction::CPU_SSSE3},
																															{"PSHUFB",			"xmmreg,r/m128",			"66 0F 38 00 /r",		Instruction::CPU_SSSE3},
																															{"PSHUFD",			"xmmreg,r/m128,imm8",		"66 0F 70 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSHUFHW",			"xmmreg,r/m128,imm8",		"p3 0F 70 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSHUFLW",			"xmmreg,r/m128,imm8",		"p2 0F 70 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSHUFW",			"mmreg,mm64,imm8",			"0F 70 /r ib",			Instruction::CPU_KATMAI},
																															{"PSIGNB",			"mmreg,mm64",				"0F 38 08 /r",			Instruction::CPU_SSSE3},
																															{"PSIGNB",			"xmmreg,r/m128",			"66 0f 38 08 /r",		Instruction::CPU_SSSE3},
																															{"PSIGND",			"mmreg,mm64",				"0F 38 0a /r",			Instruction::CPU_SSSE3},
																															{"PSIGND",			"xmmreg,r/m128",			"66 0f 38 0a /r",		Instruction::CPU_SSSE3},
																															{"PSIGNW",			"mmreg,mm64",				"0F 38 09 /r",			Instruction::CPU_SSSE3},
																															{"PSIGNW",			"xmmreg,r/m128",			"66 0f 38 09 /r",		Instruction::CPU_SSSE3},
																															{"PSLLD",			"mmreg,mm64",				"0F F2 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSLLD",			"mmreg,imm8",				"0F 72 /6 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSLLD",			"xmmreg,r/m128",			"66 0F F2 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSLLD",			"xmmreg,imm8",				"66 0F 72 /6 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSLLQ",			"mmreg,mm64",				"0F F3 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSLLQ",			"mmreg,imm8",				"0F 73 /6 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSLLQ",			"xmmreg,r/m128",			"66 0F F3 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSLLQ",			"xmmreg,imm8",				"66 0F 73 /6 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSLLW",			"mmreg,mm64",				"0F F1 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSLLW",			"mmreg,imm8",				"0F 71 /6 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSLLW",			"xmmreg,r/m128",			"66 0F F1 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSLLW",			"xmmreg,imm8",				"66 0F 71 /6 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSRAD",			"mmreg,mm64",				"0F E2 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSRAD",			"mmreg,imm8",				"0F 72 /4 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSRAD",			"xmmreg,r/m128",			"66 0F E2 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSRAD",			"xmmreg,imm8",				"66 0F 72 /4 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSRAW",			"mmreg,mm64",				"0F E1 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSRAW",			"mmreg,imm8",				"0F 71 /4 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSRAW",			"xmmreg,r/m128",			"66 0F E1 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSRAW",			"xmmreg,imm8",				"66 0F 71 /4 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSRLD",			"mmreg,mm64",				"0F D2 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSRLD",			"mmreg,imm8",				"0F 72 /2 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSRLD",			"xmmreg,r/m128",			"66 0F D2 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSRLD",			"xmmreg,imm8",				"66 0F 72 /2 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSRLDQ",			"xmmreg,imm8",				"66 0F 73 /3 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSRLQ",			"mmreg,mm64",				"0F D3 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSRLQ",			"mmreg,imm8",				"0F 73 /2 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSRLQ",			"xmmreg,r/m128",			"66 0F D3 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSRLQ",			"xmmreg,imm8",				"66 0F 73 /2 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSRLW",			"mmreg,mm64",				"0F D1 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSRLW",			"mmreg,imm8",				"0F 71 /2 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSRLW",			"xmmreg,r/m128",			"66 0F D1 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSRLW",			"xmmreg,imm8",				"66 0F 71 /2 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSUBB",			"mmreg,mm64",				"0F F8 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSUBB",			"xmmreg,r/m128",			"66 0F F8 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSUBD",			"mmreg,mm64",				"0F FA /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSUBD",			"xmmreg,r/m128",			"66 0F FA /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSUBQ",			"mmreg,mm64",				"0F FB /r",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSUBQ",			"xmmreg,r/m128",			"66 0F FB /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSUBSB",			"mmreg,mm64",				"0F E8 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSUBSB",			"xmmreg,r/m128",			"66 0F E8 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSUBSIW",			"mmreg,mm64",				"0F 55 /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
																															{"PSUBSW",			"mmreg,mm64",				"0F E9 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSUBSW",			"xmmreg,r/m128",			"66 0F E9 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSUBUSB",			"mmreg,mm64",				"0F D8 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSUBUSB",			"xmmreg,r/m128",			"66 0F D8 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSUBUSW",			"mmreg,mm64",				"0F D9 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSUBUSW",			"xmmreg,r/m128",			"66 0F D9 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSUBW",			"mmreg,mm64",				"0F F9 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PSUBW",			"xmmreg,r/m128",			"66 0F F9 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PSWAPD",			"mmreg,mm64",				"0F 0F /r BB",			Instruction::CPU_ATHLON},
																															{"PTEST",			"xmmreg,r/m128",			"66 0F 38 17 /r",		Instruction::CPU_SSE4_1},
																															{"PUNPCKHBW",		"mmreg,mm64",				"0F 68 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PUNPCKHBW",		"xmmreg,r/m128",			"66 0F 68 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PUNPCKHDQ",		"mmreg,mm64",				"0F 6A /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PUNPCKHDQ",		"xmmreg,r/m128",			"66 0F 6A /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PUNPCKHQDQ",		"xmmreg,r/m128",			"66 0F 6D /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PUNPCKHWD",		"mmreg,mm64",				"0F 69 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PUNPCKHWD",		"xmmreg,r/m128",			"66 0F 69 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PUNPCKLBW",		"mmreg,mm64",				"0F 60 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PUNPCKLBW",		"xmmreg,r/m128",			"66 0F 60 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PUNPCKLDQ",		"mmreg,mm64",				"0F 62 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PUNPCKLDQ",		"xmmreg,r/m128",			"66 0F 62 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PUNPCKLQDQ",		"xmmreg,r/m128",			"66 0F 6C /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PUNPCKLWD",		"mmreg,mm64",				"0F 61 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																															{"PUNPCKLWD",		"xmmreg,r/m128",			"66 0F 61 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																															{"PUSH",			"reg16",					"po 50 +r",				Instruction::CPU_8086},
																															{"PUSH",			"reg32",					"po 50 +r",				Instruction::CPU_386},
																															{"PUSH",			"reg64",					"po 50 +r",				Instruction::CPU_X64},
																															{"PUSH",			"WORD r/m16",				"po FF /6",				Instruction::CPU_8086},
																															{"PUSH",			"DWORD r/m32",				"po FF /6",				Instruction::CPU_386},
																															{"PUSH",			"QWORD r/m64",				"po FF /6",				Instruction::CPU_X64},
																															//	{"PUSH",			"CS",						"0E",					Instruction::CPU_8086 | Instruction::CPU_INVALID64},
																															//	{"PUSH",			"DS",						"1E",					Instruction::CPU_8086 | Instruction::CPU_INVALID64},
																															//	{"PUSH",			"ES",						"06",					Instruction::CPU_8086 | Instruction::CPU_INVALID64},
																															//	{"PUSH",			"SS",						"16",					Instruction::CPU_8086 | Instruction::CPU_INVALID64},
																															//	{"PUSH",			"FS",						"0F A0",				Instruction::CPU_386},
																															//	{"PUSH",			"GS",						"0F A8",				Instruction::CPU_386},
																																{"PUSH",			"BYTE imm8",				"6A ib",				Instruction::CPU_286},
																																{"PUSH",			"WORD imm16",				"po 68 iw",				Instruction::CPU_286},
																																{"PUSH",			"DWORD imm32",				"po 68 id",				Instruction::CPU_386},
																																{"PUSH",			"QWORD imm32",				"po 68 id",				Instruction::CPU_X64},
																																{"PUSHA",			"",							"60",					Instruction::CPU_186 | Instruction::CPU_INVALID64},
																																{"PUSHAD",			"",							"po 60",				Instruction::CPU_386 | Instruction::CPU_INVALID64},
																																{"PUSHAW",			"",							"po 60",				Instruction::CPU_186 | Instruction::CPU_INVALID64},
																																{"PUSHF",			"",							"9C",					Instruction::CPU_186},
																																{"PUSHFD",			"",							"po 9C",				Instruction::CPU_386},
																																{"PUSHFW",			"",							"po 9C",				Instruction::CPU_186},
																																{"PXOR",			"mmreg,mm64",				"0F EF /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
																																{"PXOR",			"xmmreg,r/m128",			"66 0F EF /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																																{"RCL",				"BYTE r/m8,1",				"D0 /2",				Instruction::CPU_8086},
																																{"RCL",				"BYTE r/m8,CL",				"D2 /2",				Instruction::CPU_8086},
																																{"RCL",				"BYTE r/m8,imm8",			"C0 /2 ib",				Instruction::CPU_286},
																																{"RCL",				"WORD r/m16,1",				"po D1 /2",				Instruction::CPU_8086},
																																{"RCL",				"WORD r/m16,CL",			"po D3 /2",				Instruction::CPU_8086},
																																{"RCL",				"WORD r/m16,imm8",			"po C1 /2 ib",			Instruction::CPU_286},
																																{"RCL",				"DWORD r/m32,1",			"po D1 /2",				Instruction::CPU_386},
																																{"RCL",				"DWORD r/m32,CL",			"po D3 /2",				Instruction::CPU_386},
																																{"RCL",				"DWORD r/m32,imm8",			"po C1 /2 ib",			Instruction::CPU_386},
																																{"RCL",				"DWORD r/m32,1",			"po D1 /2",				Instruction::CPU_X64},
																																{"RCL",				"QWORD r/m64,CL",			"po D3 /2",				Instruction::CPU_X64},
																																{"RCL",				"QWORD r/m64,imm8",			"po C1 /2 ib",			Instruction::CPU_X64},
																																{"RCPPS",			"xmmreg,r/m128",			"0F 53 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																																{"RCPSS",			"xmmreg,xmm32",				"p3 0F 53 /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																																{"RCR",				"BYTE r/m8,1",				"D0 /3",				Instruction::CPU_8086},
																																{"RCR",				"BYTE r/m8,CL",				"D2 /3",				Instruction::CPU_8086},
																																{"RCR",				"BYTE r/m8,imm8",			"C0 /3 ib",				Instruction::CPU_286},
																																{"RCR",				"WORD r/m16,1",				"po D1 /3",				Instruction::CPU_8086},
																																{"RCR",				"WORD r/m16,CL",			"po D3 /3",				Instruction::CPU_8086},
																																{"RCR",				"WORD r/m16,imm8",			"po C1 /3 ib",			Instruction::CPU_286},
																																{"RCR",				"DWORD r/m32,1",			"po D1 /3",				Instruction::CPU_386},
																																{"RCR",				"DWORD r/m32,CL",			"po D3 /3",				Instruction::CPU_386},
																																{"RCR",				"DWORD r/m32,imm8",			"po C1 /3 ib",			Instruction::CPU_386},
																																{"RCR",				"QWORD r/m32,1",			"po D1 /3",				Instruction::CPU_X64},
																																{"RCR",				"QWORD r/m32,CL",			"po D3 /3",				Instruction::CPU_X64},
																																{"RCR",				"QWORD r/m32,imm8",			"po C1 /3 ib",			Instruction::CPU_X64},
																																{"RDMSR",			"",							"0F 32",				Instruction::CPU_PENTIUM},
																																{"RDPMC",			"",							"0F 33",				Instruction::CPU_P6},
																																//	{"RDSHR",			"",							"0F 36",				Instruction::CPU_P6 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
																																	{"RDTSC",			"",							"0F 31",				Instruction::CPU_PENTIUM},
																																	{"REP INSB",		"",							"p3 6C",				Instruction::CPU_186},
																																	{"REP INSD",		"",							"p3 po 6D",				Instruction::CPU_386},
																																	{"REP INSW",		"",							"p3 po 6D",				Instruction::CPU_186},
																																	{"REP LODSB",		"",							"p3 AC",				Instruction::CPU_8086},
																																	{"REP LODSD",		"",							"p3 po AD",				Instruction::CPU_386},
																																	{"REP LODSW",		"",							"p3 po AD",				Instruction::CPU_8086},
																																	{"REP MOVSB",		"",							"p3 A4",				Instruction::CPU_8086},
																																	{"REP MOVSD",		"",							"p3 po A5",				Instruction::CPU_386},
																																	{"REP MOVSW",		"",							"p3 po A5",				Instruction::CPU_8086},
																																	{"REP OUTSB",		"",							"p3 6E",				Instruction::CPU_186},
																																	{"REP OUTSD",		"",							"p3 po 6F",				Instruction::CPU_386},
																																	{"REP OUTSW",		"",							"p3 po 6F",				Instruction::CPU_186},
																																	{"REP SCASB",		"",							"p3 AE",				Instruction::CPU_8086},
																																	{"REP SCASD",		"",							"p3 po AF",				Instruction::CPU_386},
																																	{"REP SCASW",		"",							"p3 po AF",				Instruction::CPU_8086},
																																	{"REP STOSB",		"",							"p3 AA",				Instruction::CPU_8086},
																																	{"REP STOSD",		"",							"p3 po AB",				Instruction::CPU_386},
																																	{"REP STOSW",		"",							"p3 po AB",				Instruction::CPU_8086},
																																	{"REPE CMPSB",		"",							"p3 A6",				Instruction::CPU_8086},
																																	{"REPE CMPSD",		"",							"p3 po A7",				Instruction::CPU_386},
																																	{"REPE CMPSW",		"",							"p3 po A7",				Instruction::CPU_8086},
																																	{"REPE SCASB",		"",							"p3 AE",				Instruction::CPU_8086},
																																	{"REPE SCASD",		"",							"p3 po AF",				Instruction::CPU_386},
																																	{"REPE SCASW",		"",							"p3 po AF",				Instruction::CPU_8086},
																																	{"REPNE CMPSB",		"",							"p2 A6",				Instruction::CPU_8086},
																																	{"REPNE CMPSD",		"",							"p2 po A7",				Instruction::CPU_386},
																																	{"REPNE CMPSW",		"",							"p2 po A7",				Instruction::CPU_8086},
																																	{"REPNE SCASB",		"",							"p2 AE",				Instruction::CPU_8086},
																																	{"REPNE SCASD",		"",							"p2 po AF",				Instruction::CPU_386},
																																	{"REPNE SCASW",		"",							"p2 po AF",				Instruction::CPU_8086},
																																	{"REPNZ CMPSB",		"",							"p2 A6",				Instruction::CPU_8086},
																																	{"REPNZ CMPSD",		"",							"p2 po A7",				Instruction::CPU_386},
																																	{"REPNZ CMPSW",		"",							"p2 po A7",				Instruction::CPU_8086},
																																	{"REPNZ SCASB",		"",							"p2 AE",				Instruction::CPU_8086},
																																	{"REPNZ SCASD",		"",							"p2 po AF",				Instruction::CPU_386},
																																	{"REPNZ SCASW",		"",							"p2 po AF",				Instruction::CPU_8086},
																																	{"REPZ CMPSB",		"",							"p3 A6",				Instruction::CPU_8086},
																																	{"REPZ CMPSD",		"",							"p3 po A7",				Instruction::CPU_386},
																																	{"REPZ CMPSW",		"",							"p3 po A7",				Instruction::CPU_8086},
																																	{"REPZ SCASB",		"",							"p3 AE",				Instruction::CPU_8086},
																																	{"REPZ SCASD",		"",							"p3 po AF",				Instruction::CPU_386},
																																	{"REPZ SCASW",		"",							"p3 po AF",				Instruction::CPU_8086},
																																	{"RET",				"",							"C3",					Instruction::CPU_8086},
																																	{"RET",				"imm16",					"C2 iw",				Instruction::CPU_8086},
																																	//	{"RETF",			"",							"CB",					Instruction::CPU_8086},
																																	//	{"RETF",			"imm16",					"CA iw",				Instruction::CPU_8086},
																																	//	{"RETN",			"",							"C3",					Instruction::CPU_8086},
																																	//	{"RETN",			"imm16",					"C2 iw",				Instruction::CPU_8086},
																																		{"ROL",				"BYTE r/m8,1",				"D0 /0",				Instruction::CPU_8086},
																																		{"ROL",				"BYTE r/m8,CL",				"D2 /0",				Instruction::CPU_8086},
																																		{"ROL",				"BYTE r/m8,imm8",			"C0 /0 ib",				Instruction::CPU_286},
																																		{"ROL",				"WORD r/m16,1",				"po D1 /0",				Instruction::CPU_8086},
																																		{"ROL",				"WORD r/m16,CL",			"po D3 /0",				Instruction::CPU_8086},
																																		{"ROL",				"WORD r/m16,imm8",			"po C1 /0 ib",			Instruction::CPU_286},
																																		{"ROL",				"DWORD r/m32,1",			"po D1 /0",				Instruction::CPU_386},
																																		{"ROL",				"DWORD r/m32,CL",			"po D3 /0",				Instruction::CPU_386},
																																		{"ROL",				"DWORD r/m32,imm8",			"po C1 /0 ib",			Instruction::CPU_386},
																																		{"ROL",				"QWORD r/m32,1",			"po D1 /0",				Instruction::CPU_X64},
																																		{"ROL",				"QWORD r/m32,CL",			"po D3 /0",				Instruction::CPU_X64},
																																		{"ROL",				"QWORD r/m32,imm8",			"po C1 /0 ib",			Instruction::CPU_X64},
																																		{"ROR",				"BYTE r/m8,1",				"D0 /1",				Instruction::CPU_8086},
																																		{"ROR",				"BYTE r/m8,CL",				"D2 /1",				Instruction::CPU_8086},
																																		{"ROR",				"BYTE r/m8,imm8",			"C0 /1 ib",				Instruction::CPU_286},
																																		{"ROR",				"WORD r/m16,1",				"po D1 /1",				Instruction::CPU_8086},
																																		{"ROR",				"WORD r/m16,CL",			"po D3 /1",				Instruction::CPU_8086},
																																		{"ROR",				"WORD r/m16,imm8",			"po C1 /1 ib",			Instruction::CPU_286},
																																		{"ROR",				"DWORD r/m32,1",			"po D1 /1",				Instruction::CPU_386},
																																		{"ROR",				"DWORD r/m32,CL",			"po D3 /1",				Instruction::CPU_386},
																																		{"ROR",				"DWORD r/m32,imm8",			"po C1 /1 ib",			Instruction::CPU_386},
																																		{"ROR",				"QWORD r/m64,1",			"po D1 /1",				Instruction::CPU_X64},
																																		{"ROR",				"QWORD r/m64,CL",			"po D3 /1",				Instruction::CPU_X64},
																																		{"ROR",				"QWORD r/m64,imm8",			"po C1 /1 ib",			Instruction::CPU_X64},
																																		{"ROUNDPD",			"xmmreg,r/m128,imm8",		"66 0F 3A 09 /r ib",	Instruction::CPU_SSE4_1},
																																		{"ROUNDPS",			"xmmreg,r/m128,imm8",		"66 0F 3A 08 /r ib",	Instruction::CPU_SSE4_1},
																																		{"ROUNDSD",			"xmmreg,r/m128,imm8",		"66 0F 3A 0B /r ib",	Instruction::CPU_SSE4_1},
																																		{"ROUNDSS",			"xmmreg,r/m128,imm8",		"66 0F 3A 0A /r ib",	Instruction::CPU_SSE4_1},
																																		//	{"RSDC",			"segreg,mem80",				"0F 79 /r",				Instruction::CPU_486 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
																																		//	{"RSLDT",			"mem80",					"0F 7B /0",				Instruction::CPU_486 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
																																			{"RSM",				"",							"0F AA",				Instruction::CPU_PENTIUM},
																																			{"RSQRTPS",			"xmmreg,r/m128",			"0F 52 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																																			{"RSQRTSS",			"xmmreg,xmm32",				"p3 0F 52 /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																																			//	{"RSTS",			"mem80",					"0F 7D /0",				Instruction::CPU_486 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
																																				{"SAHF",			"",							"9E",					Instruction::CPU_8086},
																																				{"SAL",				"BYTE r/m8,1",				"D0 /4",				Instruction::CPU_8086},
																																				{"SAL",				"BYTE r/m8,CL",				"D2 /4",				Instruction::CPU_8086},
																																				{"SAL",				"BYTE r/m8,imm8",			"C0 /4 ib",				Instruction::CPU_286},
																																				{"SAL",				"WORD r/m16,1",				"po D1 /4",				Instruction::CPU_8086},
																																				{"SAL",				"WORD r/m16,CL",			"po D3 /4",				Instruction::CPU_8086},
																																				{"SAL",				"WORD r/m16,imm8",			"po C1 /4 ib",			Instruction::CPU_286},
																																				{"SAL",				"DWORD r/m32,1",			"po D1 /4",				Instruction::CPU_386},
																																				{"SAL",				"DWORD r/m32,CL",			"po D3 /4",				Instruction::CPU_386},
																																				{"SAL",				"DWORD r/m32,imm8",			"po C1 /4 ib",			Instruction::CPU_386},
																																				{"SAL",				"QWORD r/m64,1",			"po D1 /4",				Instruction::CPU_X64},
																																				{"SAL",				"QWORD r/m64,CL",			"po D3 /4",				Instruction::CPU_X64},
																																				{"SAL",				"QWORD r/m64,imm8",			"po C1 /4 ib",			Instruction::CPU_X64},
																																				{"SAR",				"BYTE r/m8,1",				"D0 /7",				Instruction::CPU_8086},
																																				{"SAR",				"BYTE r/m8,CL",				"D2 /7",				Instruction::CPU_8086},
																																				{"SAR",				"BYTE r/m8,imm8",			"C0 /7 ib",				Instruction::CPU_286},
																																				{"SAR",				"WORD r/m16,1",				"po D1 /7",				Instruction::CPU_8086},
																																				{"SAR",				"WORD r/m16,CL",			"po D3 /7",				Instruction::CPU_8086},
																																				{"SAR",				"WORD r/m16,imm8",			"po C1 /7 ib",			Instruction::CPU_286},
																																				{"SAR",				"DWORD r/m32,1",			"po D1 /7",				Instruction::CPU_386},
																																				{"SAR",				"DWORD r/m32,CL",			"po D3 /7",				Instruction::CPU_386},
																																				{"SAR",				"DWORD r/m32,imm8",			"po C1 /7 ib",			Instruction::CPU_386},
																																				{"SAR",				"QWORD r/m64,1",			"po D1 /7",				Instruction::CPU_X64},
																																				{"SAR",				"QWORD r/m64,CL",			"po D3 /7",				Instruction::CPU_X64},
																																				{"SAR",				"QWORD r/m64,imm8",			"po C1 /7 ib",			Instruction::CPU_X64},
																																				//	{"SALC",			"",							"D6",					Instruction::CPU_8086 | Instruction::CPU_UNDOC | Instruction::CPU_INVALID64},
																																					{"SBB",				"r/m8,reg8",				"18 /r",				Instruction::CPU_8086},
																																					{"SBB",				"r/m16,reg16",				"po 19 /r",				Instruction::CPU_8086},
																																					{"SBB",				"r/m32,reg32",				"po 19 /r",				Instruction::CPU_386},
																																					{"SBB",				"r/m64,reg64",				"po 19 /r",				Instruction::CPU_X64},
																																					{"SBB",				"reg8,r/m8",				"1A /r",				Instruction::CPU_8086},
																																					{"SBB",				"reg16,r/m16",				"po 1B /r",				Instruction::CPU_8086},
																																					{"SBB",				"reg32,r/m32",				"po 1B /r",				Instruction::CPU_386},
																																					{"SBB",				"reg64,r/m64",				"po 1B /r",				Instruction::CPU_X64},
																																					{"SBB",				"BYTE r/m8,imm8",			"80 /3 ib",				Instruction::CPU_8086},
																																					{"SBB",				"WORD r/m16,imm16",			"po 81 /3 iw",			Instruction::CPU_8086},
																																					{"SBB",				"DWORD r/m32,imm32",		"po 81 /3 id",			Instruction::CPU_386},
																																					{"SBB",				"QWORD r/m64,imm32",		"po 81 /3 id",			Instruction::CPU_X64},
																																					{"SBB",				"WORD r/m16,imm8",			"po 83 /3 ib",			Instruction::CPU_8086},
																																					{"SBB",				"DWORD r/m32,imm8",			"po 83 /3 ib",			Instruction::CPU_386},
																																					{"SBB",				"QWORD r/m64,imm8",			"po 83 /3 ib",			Instruction::CPU_X64},
																																					{"SBB",				"AL,imm8",					"1C ib",				Instruction::CPU_8086},
																																					{"SBB",				"AX,imm16",					"po 1D iw",				Instruction::CPU_8086},
																																					{"SBB",				"EAX,imm32",				"po 1D id",				Instruction::CPU_386},
																																					{"SBB",				"RAX,imm32",				"po 1D id",				Instruction::CPU_X64},
																																					{"SCASB",			"",							"AE",					Instruction::CPU_8086},
																																					{"SCASD",			"",							"po AF",				Instruction::CPU_386},
																																					{"SCASQ",			"",							"po AF",				Instruction::CPU_X64},
																																					{"SCASW",			"",							"po AF",				Instruction::CPU_8086},
																																					{"SETA",			"r/m8",						"0F 97 /2",				Instruction::CPU_386},
																																					{"SETAE",			"r/m8",						"0F 93 /2",				Instruction::CPU_386},
																																					{"SETB",			"r/m8",						"0F 92 /2",				Instruction::CPU_386},
																																					{"SETBE",			"r/m8",						"0F 96 /2",				Instruction::CPU_386},
																																					{"SETC",			"r/m8",						"0F 92 /2",				Instruction::CPU_386},
																																					{"SETE",			"r/m8",						"0F 94 /2",				Instruction::CPU_386},
																																					{"SETG",			"r/m8",						"0F 9F /2",				Instruction::CPU_386},
																																					{"SETGE",			"r/m8",						"0F 9D /2",				Instruction::CPU_386},
																																					{"SETL",			"r/m8",						"0F 9C /2",				Instruction::CPU_386},
																																					{"SETLE",			"r/m8",						"0F 9E /2",				Instruction::CPU_386},
																																					{"SETNA",			"r/m8",						"0F 96 /2",				Instruction::CPU_386},
																																					{"SETNB",			"r/m8",						"0F 93 /2",				Instruction::CPU_386},
																																					{"SETNBE",			"r/m8",						"0F 97 /2",				Instruction::CPU_386},
																																					{"SETNC",			"r/m8",						"0F 93 /2",				Instruction::CPU_386},
																																					{"SETNE",			"r/m8",						"0F 95 /2",				Instruction::CPU_386},
																																					{"SETNEA",			"r/m8",						"0F 92 /2",				Instruction::CPU_386},
																																					{"SETNG",			"r/m8",						"0F 9E /2",				Instruction::CPU_386},
																																					{"SETNGE",			"r/m8",						"0F 9C /2",				Instruction::CPU_386},
																																					{"SETNL",			"r/m8",						"0F 9D /2",				Instruction::CPU_386},
																																					{"SETNLE",			"r/m8",						"0F 9F /2",				Instruction::CPU_386},
																																					{"SETNO",			"r/m8",						"0F 91 /2",				Instruction::CPU_386},
																																					{"SETNP",			"r/m8",						"0F 9B /2",				Instruction::CPU_386},
																																					{"SETNS",			"r/m8",						"0F 99 /2",				Instruction::CPU_386},
																																					{"SETNZ",			"r/m8",						"0F 95 /2",				Instruction::CPU_386},
																																					{"SETO",			"r/m8",						"0F 90 /2",				Instruction::CPU_386},
																																					{"SETP",			"r/m8",						"0F 9A /2",				Instruction::CPU_386},
																																					{"SETPE",			"r/m8",						"0F 9A /2",				Instruction::CPU_386},
																																					{"SETPO",			"r/m8",						"0F 9B /2",				Instruction::CPU_386},
																																					{"SETS",			"r/m8",						"0F 98 /2",				Instruction::CPU_386},
																																					{"SETZ",			"r/m8",						"0F 94 /2",				Instruction::CPU_386},
																																					{"SFENCE",			"",							"0F AE F8",				Instruction::CPU_KATMAI},
																																					//	{"SGDT",			"mem",						"0F 01 /0",				Instruction::CPU_286 | Instruction::CPU_PRIV},
																																					//	{"SIDT",			"mem",						"0F 01 /1",				Instruction::CPU_286 | Instruction::CPU_PRIV},
																																					//	{"SLDT",			"r/m16",					"0F 00 /0",				Instruction::CPU_286 | Instruction::CPU_PRIV},
																																						{"SHL",				"BYTE r/m8,1",				"D0 /4",				Instruction::CPU_8086},
																																						{"SHL",				"BYTE r/m8,CL",				"D2 /4",				Instruction::CPU_8086},
																																						{"SHL",				"BYTE r/m8,imm8",			"C0 /4 ib",				Instruction::CPU_286},
																																						{"SHL",				"WORD r/m16,1",				"po D1 /4",				Instruction::CPU_8086},
																																						{"SHL",				"WORD r/m16,CL",			"po D3 /4",				Instruction::CPU_8086},
																																						{"SHL",				"WORD r/m16,imm8",			"po C1 /4 ib",			Instruction::CPU_286},
																																						{"SHL",				"DWORD r/m32,1",			"po D1 /4",				Instruction::CPU_386},
																																						{"SHL",				"DWORD r/m32,CL",			"po D3 /4",				Instruction::CPU_386},
																																						{"SHL",				"DWORD r/m32,imm8",			"po C1 /4 ib",			Instruction::CPU_386},
																																						{"SHL",				"QWORD r/m64,1",			"po D1 /4",				Instruction::CPU_X64},
																																						{"SHL",				"QWORD r/m64,CL",			"po D3 /4",				Instruction::CPU_X64},
																																						{"SHL",				"QWORD r/m64,imm8",			"po C1 /4 ib",			Instruction::CPU_X64},
																																						{"SHLD",			"r/m16,reg16,imm8",			"po 0F A4 /r ib",		Instruction::CPU_386},
																																						{"SHLD",			"r/m32,reg32,imm8",			"po 0F A4 /r ib",		Instruction::CPU_386},
																																						{"SHLD",			"r/m64,reg64,imm8",			"po 0F A4 /r ib",		Instruction::CPU_X64},
																																						{"SHLD",			"r/m16,reg16,CL",			"po 0F A5 /r",			Instruction::CPU_386},
																																						{"SHLD",			"r/m32,reg32,CL",			"po 0F A5 /r",			Instruction::CPU_386},
																																						{"SHLD",			"r/m64,reg64,CL",			"po 0F A5 /r",			Instruction::CPU_X64},
																																						{"SHR",				"BYTE r/m8,1",				"D0 /5",				Instruction::CPU_8086},
																																						{"SHR",				"BYTE r/m8,CL",				"D2 /5",				Instruction::CPU_8086},
																																						{"SHR",				"BYTE r/m8,imm8",			"C0 /5 ib",				Instruction::CPU_286},
																																						{"SHR",				"WORD r/m16,1",				"po D1 /5",				Instruction::CPU_8086},
																																						{"SHR",				"WORD r/m16,CL",			"po D3 /5",				Instruction::CPU_8086},
																																						{"SHR",				"WORD r/m16,imm8",			"po C1 /5 ib",			Instruction::CPU_286},
																																						{"SHR",				"DWORD r/m32,1",			"po D1 /5",				Instruction::CPU_386},
																																						{"SHR",				"DWORD r/m32,CL",			"po D3 /5",				Instruction::CPU_386},
																																						{"SHR",				"DWORD r/m32,imm8",			"po C1 /5 ib",			Instruction::CPU_386},
																																						{"SHR",				"QWORD r/m64,1",			"po D1 /5",				Instruction::CPU_X64},
																																						{"SHR",				"QWORD r/m64,CL",			"po D3 /5",				Instruction::CPU_X64},
																																						{"SHR",				"QWORD r/m64,imm8",			"po C1 /5 ib",			Instruction::CPU_X64},
																																						{"SHRD",			"r/m16,reg16,imm8",			"po 0F AC /r ib",		Instruction::CPU_386},
																																						{"SHRD",			"r/m32,reg32,imm8",			"po 0F AC /r ib",		Instruction::CPU_386},
																																						{"SHRD",			"r/m64,reg64,imm8",			"po 0F AC /r ib",		Instruction::CPU_X64},
																																						{"SHRD",			"r/m16,reg16,CL",			"po 0F AD /r",			Instruction::CPU_386},
																																						{"SHRD",			"r/m32,reg32,CL",			"po 0F AD /r",			Instruction::CPU_386},
																																						{"SHRD",			"r/m64,reg64,CL",			"po 0F AD /r",			Instruction::CPU_X64},
																																						{"SHUFPD",			"xmmreg,r/m128,imm8",		"66 0F C6 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																																						{"SHUFPS",			"xmmreg,r/m128,imm8",		"0F C6 /r ib",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																																						//	{"SMI",				"",							"F1",					Instruction::CPU_386 | Instruction::CPU_UNDOC},
																																							{"SMINT",			"",							"0F 38",				Instruction::CPU_P6 | Instruction::CPU_CYRIX},
																																							{"SMINTOLD",		"",							"0F 7E",				Instruction::CPU_486 | Instruction::CPU_CYRIX},
																																							//	{"SMSW",			"r/m16",					"0F 01 /4",				Instruction::CPU_286 | Instruction::CPU_PRIV},
																																								{"SQRTPD",			"xmmreg,r/m128",			"66 0F 51 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																																								{"SQRTPS",			"xmmreg,r/m128",			"0F 51 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																																								{"SQRTSD",			"xmmreg,xmm64",				"p2 0F 51 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																																								{"SQRTSS",			"xmmreg,xmm32",				"p3 0F 51 /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																																								{"STC",				"",							"F9",					Instruction::CPU_8086},
																																								{"STD",				"",							"FD",					Instruction::CPU_8086},
																																								{"STI",				"",							"FB",					Instruction::CPU_8086},
																																								{"STMXCSR",			"mem32",					"0F AE /3",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																																								{"STOSB",			"",							"AA",					Instruction::CPU_8086},
																																								{"STOSD",			"",							"po AB",				Instruction::CPU_386},
																																								{"STOSQ",			"",							"po AB",				Instruction::CPU_X64},
																																								{"STOSW",			"",							"po AB",				Instruction::CPU_8086},
																																								//	{"STR",				"r/m16",					"0F 00 /1",				Instruction::CPU_286 | Instruction::CPU_PRIV},
																																									{"SUB",				"r/m8,reg8",				"28 /r",				Instruction::CPU_8086},
																																									{"SUB",				"r/m16,reg16",				"po 29 /r",				Instruction::CPU_8086},
																																									{"SUB",				"r/m32,reg32",				"po 29 /r",				Instruction::CPU_386},
																																									{"SUB",				"r/m64,reg64",				"po 29 /r",				Instruction::CPU_X64},
																																									{"SUB",				"reg8,r/m8",				"2A /r",				Instruction::CPU_8086},
																																									{"SUB",				"reg16,r/m16",				"po 2B /r",				Instruction::CPU_8086},
																																									{"SUB",				"reg32,r/m32",				"po 2B /r",				Instruction::CPU_386},
																																									{"SUB",				"reg64,r/m64",				"po 2B /r",				Instruction::CPU_X64},
																																									{"SUB",				"BYTE r/m8,imm8",			"80 /5 ib",				Instruction::CPU_8086},
																																									{"SUB",				"WORD r/m16,imm16",			"po 81 /5 iw",			Instruction::CPU_8086},
																																									{"SUB",				"DWORD r/m32,imm32",		"po 81 /5 id",			Instruction::CPU_386},
																																									{"SUB",				"QWORD r/m64,imm32",		"po 81 /5 id",			Instruction::CPU_X64},
																																									{"SUB",				"WORD r/m16,imm8",			"po 83 /5 ib",			Instruction::CPU_8086},
																																									{"SUB",				"DWORD r/m32,imm8",			"po 83 /5 ib",			Instruction::CPU_386},
																																									{"SUB",				"QWORD r/m64,imm8",			"po 83 /5 ib",			Instruction::CPU_X64},
																																									{"SUB",				"AL,imm8",					"2C ib",				Instruction::CPU_8086},
																																									{"SUB",				"AX,imm16",					"po 2D iw",				Instruction::CPU_8086},
																																									{"SUB",				"EAX,imm32",				"po 2D id",				Instruction::CPU_386},
																																									{"SUB",				"RAX,imm32",				"po 2D id",				Instruction::CPU_X64},
																																									{"SUBPD",			"xmmreg,r/m128",			"66 0F 5C /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																																									{"SUBPS",			"xmmreg,r/m128",			"0F 5C /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																																									{"SUBSD",			"xmmreg,xmm64",				"p2 0F 5C /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																																									{"SUBSS",			"xmmreg,xmm32",				"p3 0F 5C /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																																									//	{"SVDC",			"mem80,segreg",				"0F 78 /r",				Instruction::CPU_486 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
																																									//	{"SVLDT",			"mem80",					"0F 7A /0",				Instruction::CPU_486 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
																																									//	{"SVTS",			"mem80",					"0F 7C /0",				Instruction::CPU_486 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
																																									//	{"SYSCALL",			"",							"0F 05",				Instruction::CPU_P6 | Instruction::CPU_AMD},
																																										{"SYSENTER",		"",							"0F 34",				Instruction::CPU_P6 | Instruction::CPU_INVALID64},
																																										//	{"SYSEXIT",			"",							"0F 35",				Instruction::CPU_P6 | Instruction::CPU_PRIV | Instruction::CPU_INVALID64},
																																										//	{"SYSRET",			"",							"0F 07",				Instruction::CPU_P6 | Instruction::CPU_AMD | Instruction::CPU_PRIV},
																																											{"TEST",			"r/m8,reg8",				"84 /r",				Instruction::CPU_8086},
																																											{"TEST",			"r/m16,reg16",				"po 85 /r",				Instruction::CPU_8086},
																																											{"TEST",			"r/m32,reg32",				"po 85 /r",				Instruction::CPU_386},
																																											{"TEST",			"r/m64,reg64",				"po 85 /r",				Instruction::CPU_X64},
																																											{"TEST",			"BYTE r/m8,imm8",			"F6 /0 ib",				Instruction::CPU_8086},
																																											{"TEST",			"WORD r/m16,imm16",			"po F7 /0 iw",			Instruction::CPU_8086},
																																											{"TEST",			"DWORD r/m32,imm32",		"po F7 /0 id",			Instruction::CPU_386},
																																											{"TEST",			"QWORD r/m64,imm32",		"po F7 /0 id",			Instruction::CPU_X64},
																																											{"TEST",			"AL,imm8",					"A8 ib",				Instruction::CPU_8086},
																																											{"TEST",			"AX,imm16",					"po A9 iw",				Instruction::CPU_8086},
																																											{"TEST",			"EAX,imm32",				"po A9 id",				Instruction::CPU_386},
																																											{"TEST",			"RAX,imm32",				"po A9 id",				Instruction::CPU_X64},
																																											{"UCOMISD",			"xmmreg,xmm64",				"66 0F 2E /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																																											{"UCOMISS",			"xmmreg,xmm32",				"0F 2E /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																																											{"UD2",				"",							"0F 0B",				Instruction::CPU_286},
																																											//	{"UMOV",			"r/m8,reg8",				"0F 10 /r",				Instruction::CPU_386 | Instruction::CPU_UNDOC},
																																											//	{"UMOV",			"r/m16,reg16",				"po 0F 11 /r",			Instruction::CPU_386 | Instruction::CPU_UNDOC},
																																											//	{"UMOV",			"r/m32,reg32",				"po 0F 11 /r",			Instruction::CPU_386 | Instruction::CPU_UNDOC},
																																											//	{"UMOV",			"reg8,r/m8",				"0F 12 /r",				Instruction::CPU_386 | Instruction::CPU_UNDOC},
																																											//	{"UMOV",			"reg16,r/m16",				"po 0F 13 /r",			Instruction::CPU_386 | Instruction::CPU_UNDOC},
																																											//	{"UMOV",			"reg32,r/m32",				"po 0F 13 /r",			Instruction::CPU_386 | Instruction::CPU_UNDOC},
																																												{"UNPCKHPD",		"xmmreg,r/m128",			"66 0F 15 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																																												{"UNPCKHPS",		"xmmreg,r/m128",			"0F 15 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																																												{"UNPCKLPD",		"xmmreg,r/m128",			"66 0F 14 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
																																												{"UNPCKLPS",		"xmmreg,r/m128",			"0F 14 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
																																												//	{"VERR",			"r/m16",					"0F 00 /4",				Instruction::CPU_286 | Instruction::CPU_PRIV},
																																												//	{"VERW",			"r/m16",					"0F 00 /5",				Instruction::CPU_286 | Instruction::CPU_PRIV},
																																													{"WAIT",			"",							"9B",					Instruction::CPU_8086},
																																													//	{"WBINVD",			"",							"0F 09",				Instruction::CPU_486},
																																														{"WRMSR",			"",							"0F 30",				Instruction::CPU_PENTIUM},
																																														//	{"WRSHR",			"",							"0F 37",				Instruction::CPU_P6 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
																																															{"XADD",			"r/m8,reg8",				"0F C0 /r",				Instruction::CPU_486},
																																															{"XADD",			"r/m16,reg16",				"po 0F C1 /r",			Instruction::CPU_486},
																																															{"XADD",			"r/m32,reg32",				"po 0F C1 /r",			Instruction::CPU_486},
																																															{"XADD",			"r/m64,reg64",				"po 0F C1 /r",			Instruction::CPU_X64},
																																															//	{"XBTS",			"reg16,r/m16",				"po 0F A6 /r",			Instruction::CPU_386 | Instruction::CPU_UNDOC},
																																															//	{"XBTS",			"reg32,r/m32",				"po 0F A6 /r",			Instruction::::CPU_386 | Instruction::CPU_UNDOC},
																																																{"XCHG",			"reg8,r/m8",				"86 /r",				Instruction::CPU_8086},
																																																{"XCHG",			"reg16,r/m16",				"po 87 /r",				Instruction::CPU_8086},
																																																{"XCHG",			"reg32,r/m32",				"po 87 /r",				Instruction::CPU_386},
																																																{"XCHG",			"reg64,r/m64",				"po 87 /r",				Instruction::CPU_X64},
																																																{"XCHG",			"r/m8,reg8",				"86 /r",				Instruction::CPU_8086},
																																																{"XCHG",			"r/m16,reg16",				"po 87 /r",				Instruction::CPU_8086},
																																																{"XCHG",			"r/m32,reg32",				"po 87 /r",				Instruction::CPU_386},
																																																{"XCHG",			"r/m64,reg64",				"po 87 /r",				Instruction::CPU_X64},
																																																{"XCHG",			"AX,reg16",					"po 90 +r",				Instruction::CPU_8086},
																																																{"XCHG",			"EAX,reg32",				"po 90 +r",				Instruction::CPU_386},
																																																{"XCHG",			"RAX,reg64",				"po 90 +r",				Instruction::CPU_X64},
																																																{"XCHG",			"reg16,AX",					"po 90 +r",				Instruction::CPU_8086},
																																																{"XCHG",			"reg32,EAX",				"po 90 +r",				Instruction::CPU_386},
																																																{"XCHG",			"reg64,RAX",				"po 90 +r",				Instruction::CPU_X64},
																																																{"XLATB",			"",							"D7",					Instruction::CPU_8086},
																																																{"XOR",				"r/m8,reg8",				"30 /r",				Instruction::CPU_8086},
																																																{"XOR",				"r/m16,reg16",				"po 31 /r",				Instruction::CPU_8086},
																																																{"XOR",				"r/m32,reg32",				"po 31 /r",				Instruction::CPU_386},
																																																{"XOR",				"r/m64,reg64",				"po 31 /r",				Instruction::CPU_X64},
																																																{"XOR",				"reg8,r/m8",				"32 /r",				Instruction::CPU_8086},
																																																{"XOR",				"reg16,r/m16",				"po 33 /r",				Instruction::CPU_8086},
																																																{"XOR",				"reg32,r/m32",				"po 33 /r",				Instruction::CPU_386},
																																																{"XOR",				"reg64,r/m64",				"po 33 /r",				Instruction::CPU_X64},
																																																{"XOR",				"BYTE r/m8,imm8",			"80 /6 ib",				Instruction::CPU_8086},
																																																{"XOR",				"WORD r/m16,imm16",			"po 81 /6 iw",			Instruction::CPU_8086},
																																																{"XOR",				"DWORD r/m32,imm32",		"po 81 /6 id",			Instruction::CPU_386},
																																																{"XOR",				"QWORD r/m64,imm32",		"po 81 /6 id",			Instruction::CPU_X64},
																																																{"XOR",				"WORD r/m16,imm8",			"po 83 /6 ib",			Instruction::CPU_8086},
																																																{"XOR",				"DWORD r/m32,imm8",			"po 83 /6 ib",			Instruction::CPU_386},
																																																{"XOR",				"QWORD r/m64,imm8",			"po 83 /6 ib",			Instruction::CPU_X64},
																																																{"XOR",				"AL,imm8",					"34 ib",				Instruction::CPU_8086},
																																																{"XOR",				"AX,imm16",					"po 35 iw",				Instruction::CPU_8086},
																																																{"XOR",				"EAX,imm32",				"po 35 id",				Instruction::CPU_386},
																																																{"XOR",				"RAX,imm32",				"po 35 id",				Instruction::CPU_X64},
																																																{"XORPS",			"xmmreg,r/m128",			"0F 57 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
	};

	const int InstructionSet::numInstructions = sizeof(instructionSet) / sizeof(Instruction::Syntax);

	InstructionSet::InstructionSet()
	{
		intrinsicMap = new Instruction[numInstructions];

		for (int k = 0; k < numInstructions; k++)
		{
			intrinsicMap[k] = Instruction(&instructionSet[k]);
		}

		//generateIntrinsics();   // Uncomment this line when you make changes to the instruction set
	}

	InstructionSet::~InstructionSet()
	{
		delete[] intrinsicMap;
	}

	const Instruction* InstructionSet::instruction(int i)
	{
		return &intrinsicMap[i];
	}

	void InstructionSet::generateIntrinsics()
	{
		assert(intrinsicMap);

		// Check alphabetical order
		for (int k = 0; k < numInstructions; k++)
		{
			if (k != 0)
			{
				if (strcmp(instructionSet[k - 1].mnemonic, instructionSet[k].mnemonic) > 0)
				{
					assert(false);
				}
			}
		}

		FILE* intrinsics = fopen("Intrinsics.hpp", "w");

		fprintf(intrinsics, "/* Automatically generated file, do not modify */\n"
			"/* To regenerate this file uncomment generateIntrinsics() in InstructionSet.cpp */\n\n");

		fprintf(intrinsics, "#ifndef SOFTWIRE_NO_INTRINSICS\n\n");

		fprintf(intrinsics, "typedef OperandIMM IMM;\n");
		fprintf(intrinsics, "typedef OperandAL AL;\n");
		fprintf(intrinsics, "typedef OperandAX AX;\n");
		fprintf(intrinsics, "typedef OperandEAX EAX;\n");
		fprintf(intrinsics, "typedef OperandRAX RAX;\n");
		fprintf(intrinsics, "typedef OperandDX DX;\n");
		fprintf(intrinsics, "typedef OperandCL CL;\n");
		fprintf(intrinsics, "typedef OperandCX CX;\n");
		fprintf(intrinsics, "typedef OperandECX ECX;\n");
		fprintf(intrinsics, "typedef OperandST0 ST0;\n");
		fprintf(intrinsics, "typedef OperandREG8 REG8;\n");
		fprintf(intrinsics, "typedef OperandREG16 REG16;\n");
		fprintf(intrinsics, "typedef OperandREG32 REG32;\n");
		fprintf(intrinsics, "typedef OperandREG64 REG64;\n");
		fprintf(intrinsics, "typedef OperandFPUREG FPUREG;\n");
		fprintf(intrinsics, "typedef OperandMMREG MMREG;\n");
		fprintf(intrinsics, "typedef OperandXMMREG XMMREG;\n");
		fprintf(intrinsics, "typedef OperandMEM8 MEM8;\n");
		fprintf(intrinsics, "typedef OperandMEM16 MEM16;\n");
		fprintf(intrinsics, "typedef OperandMEM32 MEM32;\n");
		fprintf(intrinsics, "typedef OperandMEM64 MEM64;\n");
		fprintf(intrinsics, "typedef OperandMEM128 MEM128;\n");
		fprintf(intrinsics, "typedef OperandR_M8 R_M8;\n");
		fprintf(intrinsics, "typedef OperandR_M16 R_M16;\n");
		fprintf(intrinsics, "typedef OperandR_M32 R_M32;\n");
		fprintf(intrinsics, "typedef OperandR_M64 R_M64;\n");
		fprintf(intrinsics, "typedef OperandR_M128 R_M128;\n");
		fprintf(intrinsics, "typedef OperandXMM32 XMM32;\n");
		fprintf(intrinsics, "typedef OperandXMM64 XMM64;\n");
		fprintf(intrinsics, "typedef OperandMM64 MM64;\n");
		fprintf(intrinsics, "typedef OperandREF REF;\n");
		fprintf(intrinsics, "\n");
		fprintf(intrinsics, "typedef unsigned char byte;\n");
		fprintf(intrinsics, "typedef unsigned short word;\n");
		fprintf(intrinsics, "typedef unsigned int dword;\n");
		fprintf(intrinsics, "typedef unsigned __int64 qword;\n");
		fprintf(intrinsics, "\n");
		fprintf(intrinsics, "#define enc virtual Encoding*\n");
		fprintf(intrinsics, "\n");

		struct InstructionSignature
		{
			const char* mnemonic;

			const char* firstOperand;
			const char* secondOperand;
			const char* thirdOperand;

			Operand::Type firstType;
			Operand::Type secondType;
			Operand::Type thirdType;
		};

		InstructionSignature* uniqueSignature = new InstructionSignature[10000];
		int n = 0;	// Number of unique instructions

		for (int t = 0; t < numInstructions; t++)
		{
			const Instruction* instruction = &intrinsicMap[t];

			char mnemonic[256] = { 0 };
			strcpy(mnemonic, instruction->getMnemonic());
			_strlwr(mnemonic);
			if (mnemonic[3] == ' ') mnemonic[3] = '_';	// Append REP prefix
			if (mnemonic[4] == ' ') mnemonic[4] = '_';	// Append LOCK prefix
			if (mnemonic[5] == ' ') mnemonic[5] = '_';	// Append REPNE/REPNZ prefix

			Operand::Type t1 = instruction->getFirstOperand();
			Operand::Type t2 = instruction->getSecondOperand();
			Operand::Type t3 = instruction->getThirdOperand();

			const Operand::Register subtypeTable[] =
			{
				{Operand::OPERAND_VOID,		0},

				{Operand::OPERAND_REF,		"REF"},

				{Operand::OPERAND_IMM,		"dword"},
				{Operand::OPERAND_EXT8,		"byte"},
				{Operand::OPERAND_IMM8,		"byte"},
				{Operand::OPERAND_IMM16,	"word"},
				{Operand::OPERAND_IMM32,	"dword"},
				//	{Operand::OPERAND_IMM64,	"qword"},

					{Operand::OPERAND_REG8,		"REG8"},
					{Operand::OPERAND_REG16,	"REG16"},
					{Operand::OPERAND_REG32,	"REG32"},
					{Operand::OPERAND_REG64,	"REG64"},
					{Operand::OPERAND_FPUREG,	"FPUREG"},
					{Operand::OPERAND_MMREG,	"MMREG"},
					{Operand::OPERAND_XMMREG,	"XMMREG"},

					// Specializations that don't offer significant benefit
					// Placed lower to avoid outputting them
					{Operand::OPERAND_AL,		"AL"},
					{Operand::OPERAND_AX,		"AX"},
					{Operand::OPERAND_EAX,		"EAX"},
					{Operand::OPERAND_RAX,		"RAX"},
					{Operand::OPERAND_DX,		"DX"},
					{Operand::OPERAND_CL,		"CL"},
					{Operand::OPERAND_CX,		"CX"},
					{Operand::OPERAND_ECX,		"ECX"},
					{Operand::OPERAND_ST0,		"ST0"},

					{Operand::OPERAND_MEM8,		"MEM8"},
					{Operand::OPERAND_MEM16,	"MEM16"},
					{Operand::OPERAND_MEM32,	"MEM32"},
					{Operand::OPERAND_MEM64,	"MEM64"},
					{Operand::OPERAND_MEM128,	"MEM128"},

					{Operand::OPERAND_R_M8,		"R_M8"},
					{Operand::OPERAND_R_M16,	"R_M16"},
					{Operand::OPERAND_R_M32,	"R_M32"},
					{Operand::OPERAND_R_M64,	"R_M64"},
					{Operand::OPERAND_R_M128,	"R_M128"},

					{Operand::OPERAND_XMM32,	"XMM32"},
					{Operand::OPERAND_XMM64,	"XMM64"},
					{Operand::OPERAND_MM64,		"MM64"}
			};

			for (int i = 0; i < sizeof(subtypeTable) / sizeof(Operand::Register); i++)
				for (int j = 0; j < sizeof(subtypeTable) / sizeof(Operand::Register); j++)
					for (int k = 0; k < sizeof(subtypeTable) / sizeof(Operand::Register); k++)
					{
						if (Operand::isSubtypeOf(subtypeTable[i].type, t1))
							if (Operand::isSubtypeOf(subtypeTable[j].type, t2))
								if (Operand::isSubtypeOf(subtypeTable[k].type, t3))
								{
									int u = 0;

									for (u = 0; u < n; u++)
									{
										// Must have unique signature
										if (strcmp(instruction->getMnemonic(), uniqueSignature[u].mnemonic) == 0)
											if (strcmp(subtypeTable[i].notation, uniqueSignature[u].firstOperand) == 0)
												if (strcmp(subtypeTable[j].notation, uniqueSignature[u].secondOperand) == 0)
													if (strcmp(subtypeTable[k].notation, uniqueSignature[u].thirdOperand) == 0)
													{
														break;
													}

										// Don't output specialized instructions
										if (strcmp(instruction->getMnemonic(), uniqueSignature[u].mnemonic) == 0)
											if (Operand::isSubtypeOf(subtypeTable[i].type, uniqueSignature[u].firstType))
												if (Operand::isSubtypeOf(subtypeTable[j].type, uniqueSignature[u].secondType))
													if (Operand::isSubtypeOf(subtypeTable[k].type, uniqueSignature[u].thirdType))
													{
														break;
													}
									}

									if (u < n)
									{
										continue;
									}

									fprintf(intrinsics, "enc %s(", mnemonic);
									if (subtypeTable[i].notation) fprintf(intrinsics, "%s a", subtypeTable[i].notation);
									if (subtypeTable[j].notation) fprintf(intrinsics, ",%s b", subtypeTable[j].notation);
									if (subtypeTable[k].notation) fprintf(intrinsics, ",%s c", subtypeTable[k].notation);
									fprintf(intrinsics, "){return x86(%d", t);
									if (subtypeTable[i].notation)
									{
										fprintf(intrinsics, ",");
										if (Operand::isSubtypeOf(subtypeTable[i].type, Operand::OPERAND_IMM) &&
											subtypeTable[i].type != Operand::OPERAND_REF)
										{
											fprintf(intrinsics, "(IMM)");
										}
										fprintf(intrinsics, "a");
									}
									if (subtypeTable[j].notation)
									{
										fprintf(intrinsics, ",");
										if (Operand::isSubtypeOf(subtypeTable[j].type, Operand::OPERAND_IMM) &&
											subtypeTable[j].type != Operand::OPERAND_REF)
										{
											fprintf(intrinsics, "(IMM)");
										}
										fprintf(intrinsics, "b");
									}
									if (subtypeTable[k].notation)
									{
										fprintf(intrinsics, ",");
										if (Operand::isSubtypeOf(subtypeTable[k].type, Operand::OPERAND_IMM) &&
											subtypeTable[k].type != Operand::OPERAND_REF)
										{
											fprintf(intrinsics, "(IMM)");
										}
										fprintf(intrinsics, "c");
									}
									fprintf(intrinsics, ");}\n");

									printf("%s\n", mnemonic);

									uniqueSignature[n].mnemonic = instruction->getMnemonic();
									uniqueSignature[n].firstOperand = subtypeTable[i].notation;
									uniqueSignature[n].secondOperand = subtypeTable[j].notation;
									uniqueSignature[n].thirdOperand = subtypeTable[k].notation;
									uniqueSignature[n].firstType = subtypeTable[i].type;
									uniqueSignature[n].secondType = subtypeTable[j].type;
									uniqueSignature[n].thirdType = subtypeTable[k].type;
									n++;
								}
					}
		}

		fprintf(intrinsics, "\n");
		fprintf(intrinsics, "#undef enc\n\n");
		fprintf(intrinsics, "#endif   // SOFTWIRE_NO_INTRINSICS\n");

		delete[] uniqueSignature;
		fclose(intrinsics);
	}

	int InstructionSet::strcmp(const char* string1, const char* string2)
	{
		if (!string1 || !string2)
		{
			return (string1 == string2) ? 0 : -1;
		}

		return ::strcmp(string1, string2);
	}
}

namespace SoftWire
{
	const OperandAL Assembler::al;
	const OperandCL Assembler::cl;
	const OperandREG8 Assembler::dl(Encoding::DL);
	const OperandREG8 Assembler::bl(Encoding::BL);
	const OperandREG8 Assembler::ah(Encoding::AH);
	const OperandREG8 Assembler::ch(Encoding::CH);
	const OperandREG8 Assembler::dh(Encoding::DH);
	const OperandREG8 Assembler::bh(Encoding::BH);
	const OperandAL Assembler::r0b;
	const OperandCL Assembler::r1b;
	const OperandREG8 Assembler::r2b(Encoding::R2);
	const OperandREG8 Assembler::r3b(Encoding::R3);
	const OperandREG8 Assembler::r4b(Encoding::R4);
	const OperandREG8 Assembler::r5b(Encoding::R5);
	const OperandREG8 Assembler::r6b(Encoding::R6);
	const OperandREG8 Assembler::r7b(Encoding::R7);
	const OperandREG8 Assembler::r8b(Encoding::R8);
	const OperandREG8 Assembler::r9b(Encoding::R9);
	const OperandREG8 Assembler::r10b(Encoding::R10);
	const OperandREG8 Assembler::r11b(Encoding::R11);
	const OperandREG8 Assembler::r12b(Encoding::R12);
	const OperandREG8 Assembler::r13b(Encoding::R13);
	const OperandREG8 Assembler::r14b(Encoding::R14);
	const OperandREG8 Assembler::r15b(Encoding::R15);

	const OperandAX Assembler::ax;
	const OperandCX Assembler::cx;
	const OperandDX Assembler::dx;
	const OperandREG16 Assembler::bx(Encoding::BX);
	const OperandREG16 Assembler::sp(Encoding::SP);
	const OperandREG16 Assembler::bp(Encoding::BP);
	const OperandREG16 Assembler::si(Encoding::SI);
	const OperandREG16 Assembler::di(Encoding::DI);
	const OperandAX Assembler::r0w;
	const OperandCX Assembler::r1w;
	const OperandDX Assembler::r2w;
	const OperandREG16 Assembler::r3w(Encoding::R3);
	const OperandREG16 Assembler::r4w(Encoding::R4);
	const OperandREG16 Assembler::r5w(Encoding::R5);
	const OperandREG16 Assembler::r6w(Encoding::R6);
	const OperandREG16 Assembler::r7w(Encoding::R7);
	const OperandREG16 Assembler::r8w(Encoding::R8);
	const OperandREG16 Assembler::r9w(Encoding::R9);
	const OperandREG16 Assembler::r10w(Encoding::R10);
	const OperandREG16 Assembler::r11w(Encoding::R11);
	const OperandREG16 Assembler::r12w(Encoding::R12);
	const OperandREG16 Assembler::r13w(Encoding::R13);
	const OperandREG16 Assembler::r14w(Encoding::R14);
	const OperandREG16 Assembler::r15w(Encoding::R15);

	const OperandEAX Assembler::eax;
	const OperandECX Assembler::ecx;
	const OperandREG32 Assembler::edx(Encoding::EDX);
	const OperandREG32 Assembler::ebx(Encoding::EBX);
	const OperandREG32 Assembler::esp(Encoding::ESP);
	const OperandREG32 Assembler::ebp(Encoding::EBP);
	const OperandREG32 Assembler::esi(Encoding::ESI);
	const OperandREG32 Assembler::edi(Encoding::EDI);
	const OperandEAX Assembler::r0d;
	const OperandECX Assembler::r1d;
	const OperandREG32 Assembler::r2d(Encoding::R2);
	const OperandREG32 Assembler::r3d(Encoding::R3);
	const OperandREG32 Assembler::r4d(Encoding::R4);
	const OperandREG32 Assembler::r5d(Encoding::R5);
	const OperandREG32 Assembler::r6d(Encoding::R6);
	const OperandREG32 Assembler::r7d(Encoding::R7);
	const OperandREG32 Assembler::r8d(Encoding::R8);
	const OperandREG32 Assembler::r9d(Encoding::R9);
	const OperandREG32 Assembler::r10d(Encoding::R10);
	const OperandREG32 Assembler::r11d(Encoding::R11);
	const OperandREG32 Assembler::r12d(Encoding::R12);
	const OperandREG32 Assembler::r13d(Encoding::R13);
	const OperandREG32 Assembler::r14d(Encoding::R14);
	const OperandREG32 Assembler::r15d(Encoding::R15);

	const OperandREG64 Assembler::rax(Encoding::R0);
	const OperandREG64 Assembler::rcx(Encoding::R1);
	const OperandREG64 Assembler::rdx(Encoding::R2);
	const OperandREG64 Assembler::rbx(Encoding::R3);
	const OperandREG64 Assembler::rsp(Encoding::R4);
	const OperandREG64 Assembler::rbp(Encoding::R5);
	const OperandREG64 Assembler::rsi(Encoding::R6);
	const OperandREG64 Assembler::rdi(Encoding::R7);
	const OperandREG64 Assembler::r0(Encoding::R0);
	const OperandREG64 Assembler::r1(Encoding::R1);
	const OperandREG64 Assembler::r2(Encoding::R2);
	const OperandREG64 Assembler::r3(Encoding::R3);
	const OperandREG64 Assembler::r4(Encoding::R4);
	const OperandREG64 Assembler::r5(Encoding::R5);
	const OperandREG64 Assembler::r6(Encoding::R6);
	const OperandREG64 Assembler::r7(Encoding::R7);
	const OperandREG64 Assembler::r8(Encoding::R8);
	const OperandREG64 Assembler::r9(Encoding::R9);
	const OperandREG64 Assembler::r10(Encoding::R10);
	const OperandREG64 Assembler::r11(Encoding::R11);
	const OperandREG64 Assembler::r12(Encoding::R12);
	const OperandREG64 Assembler::r13(Encoding::R13);
	const OperandREG64 Assembler::r14(Encoding::R14);
	const OperandREG64 Assembler::r15(Encoding::R15);

	const OperandST0 Assembler::st;
	const OperandST0 Assembler::st0;
	const OperandFPUREG Assembler::st1(Encoding::ST1);
	const OperandFPUREG Assembler::st2(Encoding::ST2);
	const OperandFPUREG Assembler::st3(Encoding::ST3);
	const OperandFPUREG Assembler::st4(Encoding::ST4);
	const OperandFPUREG Assembler::st5(Encoding::ST5);
	const OperandFPUREG Assembler::st6(Encoding::ST6);
	const OperandFPUREG Assembler::st7(Encoding::ST7);

	const OperandMMREG Assembler::mm0(Encoding::MM0);
	const OperandMMREG Assembler::mm1(Encoding::MM1);
	const OperandMMREG Assembler::mm2(Encoding::MM2);
	const OperandMMREG Assembler::mm3(Encoding::MM3);
	const OperandMMREG Assembler::mm4(Encoding::MM4);
	const OperandMMREG Assembler::mm5(Encoding::MM5);
	const OperandMMREG Assembler::mm6(Encoding::MM6);
	const OperandMMREG Assembler::mm7(Encoding::MM7);

	const OperandXMMREG Assembler::xmm0(Encoding::XMM0);
	const OperandXMMREG Assembler::xmm1(Encoding::XMM1);
	const OperandXMMREG Assembler::xmm2(Encoding::XMM2);
	const OperandXMMREG Assembler::xmm3(Encoding::XMM3);
	const OperandXMMREG Assembler::xmm4(Encoding::XMM4);
	const OperandXMMREG Assembler::xmm5(Encoding::XMM5);
	const OperandXMMREG Assembler::xmm6(Encoding::XMM6);
	const OperandXMMREG Assembler::xmm7(Encoding::XMM7);

	const OperandMEM8 Assembler::byte_ptr;
	const OperandMEM16 Assembler::word_ptr;
	const OperandMEM32 Assembler::dword_ptr;
	const OperandMEM64 Assembler::mmword_ptr;
	const OperandMEM64 Assembler::qword_ptr;
	const OperandMEM128 Assembler::xmmword_ptr;
	const OperandMEM128 Assembler::xword_ptr;

	InstructionSet *Assembler::instructionSet = 0;
	int Assembler::referenceCount = 0;
	bool Assembler::listingEnabled = true;

	Assembler::Assembler(bool x64) : x64(x64)
	{
		echoFile = 0;
		entryLabel = 0;

		if(!instructionSet)
		{
			instructionSet = new InstructionSet();
		}
	
		referenceCount++;

		linker = new Linker();
		loader = new Loader(*linker, x64);
		synthesizer = new Synthesizer(x64);
	}

	Assembler::~Assembler()
	{
		delete[] entryLabel;
		entryLabel = 0;

		delete linker;
		linker = 0;

		delete loader;
		loader = 0;

		delete synthesizer;
		synthesizer = 0;

		referenceCount--;
		if(!referenceCount)
		{
			delete instructionSet;
			instructionSet = 0;
		}

		delete[] echoFile;
		echoFile = 0;
	}

	void (*Assembler::callable(const char *entryLabel))()
	{
		if(!loader) return 0;

		if(entryLabel)
		{
			return loader->callable(entryLabel);
		}
		else
		{
			return loader->callable(this->entryLabel);
		}
	}

	void (*Assembler::finalize(const char *entryLabel))()
	{
		if(!loader) throw Error("Assembler could not be finalized (cannot re-finalize)");

		delete linker;
		linker = 0;

		delete synthesizer;
		synthesizer = 0;

		delete[] echoFile;
		echoFile = 0;

		if(entryLabel)
		{
			delete[] this->entryLabel;
			this->entryLabel = 0;

			return loader->finalize(entryLabel);
		}
		else
		{
			return loader->finalize(this->entryLabel);
		}
	}

	void *Assembler::acquire()
	{
		if(!loader) return 0;

		return loader->acquire();
	}

	const char *Assembler::getListing() const
	{
		return loader->getListing();
	}

	void Assembler::clearListing() const
	{
		loader->clearListing();
	}

	void Assembler::setEchoFile(const char *echoFile, const char *mode)
	{
		if(!listingEnabled) return;

		if(this->echoFile)
		{
			delete[] this->echoFile;
			this->echoFile = 0;
		}

		if(echoFile)
		{
			this->echoFile = strdup(echoFile);

			FILE *file = fopen(echoFile, mode);
			const time_t t = time(0);
			fprintf(file, "\n;%s\n", ctime(&t));
			fclose(file);
		}
	}

	void Assembler::annotate(const char *format, ...)
	{
		if(!echoFile) return;

		char buffer[256];
		va_list argList;

		va_start(argList, format);
		vsnprintf(buffer, 256, format, argList);
		va_end(argList);

		FILE *file = fopen(echoFile, "at");
		fprintf(file, "; ");
		fprintf(file, buffer);
		fprintf(file, "\n");
		fclose(file);
	}

	void Assembler::reset()
	{
		if(!loader) return;

		loader->reset();
	}

	int Assembler::instructionCount()
	{
		if(!loader)
		{
			return 0;
		}

		return loader->instructionCount();
	}

	void Assembler::enableListing()
	{
		listingEnabled = true;
	}

	void Assembler::disableListing()
	{
		listingEnabled = false;
	}

	Encoding *Assembler::x86(int instructionID, const Operand &firstOperand, const Operand &secondOperand, const Operand &thirdOperand)
	{
		if(!loader || !synthesizer || !instructionSet) throw INTERNAL_ERROR;

		const Instruction *instruction = instructionSet->instruction(instructionID);

		if(echoFile)
		{
			FILE *file = fopen(echoFile, "at");

			fprintf(file, "\t%s", instruction->getMnemonic());
			if(!Operand::isVoid(firstOperand)) fprintf(file, "\t%s", firstOperand.string());
			if(!Operand::isVoid(secondOperand)) fprintf(file, ",\t%s", secondOperand.string());
			if(!Operand::isVoid(thirdOperand)) fprintf(file, ",\t%s", thirdOperand.string());
			fprintf(file, "\n");

			fclose(file);
		}

		synthesizer->reset();

		synthesizer->encodeFirstOperand(firstOperand);
		synthesizer->encodeSecondOperand(secondOperand);
		synthesizer->encodeThirdOperand(thirdOperand);
		const Encoding &encoding = synthesizer->encodeInstruction(instruction);

		return loader->appendEncoding(encoding);
	}

	void Assembler::label(const char *label)
	{
		if(!loader || !synthesizer) return;

		if(echoFile)
		{
			FILE *file = fopen(echoFile, "at");
			fprintf(file, "%s:\n", label);
			fclose(file);
		}

		synthesizer->reset();

		synthesizer->defineLabel(label);
		const Encoding &encoding = synthesizer->encodeInstruction(0);

		loader->appendEncoding(encoding);
	}
};

namespace SoftWire
{
	float Emulator::sse[8][4];   // Storage for SSE emulation registers

	bool Emulator::emulateSSE = false;

	Emulator::Emulator(bool x64) : Optimizer(x64)
	{
	}

	Emulator::~Emulator()
	{
	}

	OperandREG8 Emulator::t8(unsigned int i)
	{
		static char t[8];

		if (i >= 8) throw;

		return r8(&t[i]);
	}

	OperandREG16 Emulator::t16(unsigned int i)
	{
		static short t[8];

		if (i >= 8) throw;

		return r16(&t[i]);
	}

	OperandREG32 Emulator::t32(unsigned int i)
	{
		static int t[8];

		if (i >= 8) throw;

		return r32(&t[i]);
	}

	Encoding* Emulator::addps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			fld(dword_ptr[&sse[i][0]]);
			fadd(dword_ptr[&sse[j][0]]);
			fstp(dword_ptr[&sse[i][0]]);

			fld(dword_ptr[&sse[i][1]]);
			fadd(dword_ptr[&sse[j][1]]);
			fstp(dword_ptr[&sse[i][1]]);

			fld(dword_ptr[&sse[i][2]]);
			fadd(dword_ptr[&sse[j][2]]);
			fstp(dword_ptr[&sse[i][2]]);

			fld(dword_ptr[&sse[i][3]]);
			fadd(dword_ptr[&sse[j][3]]);
			fstp(dword_ptr[&sse[i][3]]);

			return 0;
		}

		return Optimizer::addps(xmmi, xmmj);
	}

	Encoding* Emulator::addps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fld(dword_ptr[&sse[i][0]]);
			fadd((OperandMEM32)(mem128 + 0));
			fstp(dword_ptr[&sse[i][0]]);

			fld(dword_ptr[&sse[i][1]]);
			fadd((OperandMEM32)(mem128 + 4));
			fstp(dword_ptr[&sse[i][1]]);

			fld(dword_ptr[&sse[i][2]]);
			fadd((OperandMEM32)(mem128 + 8));
			fstp(dword_ptr[&sse[i][2]]);

			fld(dword_ptr[&sse[i][3]]);
			fadd((OperandMEM32)(mem128 + 12));
			fstp(dword_ptr[&sse[i][3]]);

			return 0;
		}

		return Optimizer::addps(xmm, mem128);
	}

	Encoding* Emulator::addps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return addps(xmm, (OperandXMMREG)r_m128);
		else                                       return addps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::addss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr[&sse[i][0]]);
			fadd(dword_ptr[&sse[j][0]]);
			fstp(dword_ptr[&sse[i][0]]);
			return 0;
		}

		return Optimizer::addss(xmmi, xmmj);
	}

	Encoding* Emulator::addss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld(dword_ptr[&sse[i][0]]);
			fadd((OperandMEM32)mem32);
			fstp(dword_ptr[&sse[i][0]]);
			return 0;
		}

		return Optimizer::addss(xmm, mem32);
	}

	Encoding* Emulator::addss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return addss(xmm, (OperandXMMREG)xmm32);
		else                                      return addss(xmm, (OperandMEM32)xmm32);
	}

	Encoding* Emulator::andnps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr[&sse[j][0]]);
			not(dword_ptr[&sse[i][0]]);
			and (dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][1]]);
			not(dword_ptr[&sse[i][1]]);
			and (dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][2]]);
			not(dword_ptr[&sse[i][2]]);
			and (dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][3]]);
			not(dword_ptr[&sse[i][3]]);
			and (dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::andnps(xmmi, xmmj);
	}

	Encoding* Emulator::andnps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(mem128 + 0));
			not(dword_ptr[&sse[i][0]]);
			and (dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 4));
			not(dword_ptr[&sse[i][1]]);
			and (dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 8));
			not(dword_ptr[&sse[i][2]]);
			and (dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 12));
			not(dword_ptr[&sse[i][3]]);
			and (dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::andnps(xmm, mem128);
	}

	Encoding* Emulator::andnps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return andnps(xmm, (OperandXMMREG)r_m128);
		else                                       return andnps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::andps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr[&sse[j][0]]);
			and (dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][1]]);
			and (dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][2]]);
			and (dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][3]]);
			and (dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::andps(xmmi, xmmj);
	}

	Encoding* Emulator::andps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(mem128 + 0));
			and (dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 4));
			and (dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 8));
			and (dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 12));
			and (dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::andps(xmm, mem128);
	}

	Encoding* Emulator::andps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return andps(xmm, (OperandXMMREG)r_m128);
		else                                       return andps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::cmpps(OperandXMMREG xmmi, OperandXMMREG xmmj, char c)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			static float zero = 0;
			static float one = 1;
			fld(dword_ptr[&zero]);		// st2
			fld(dword_ptr[&one]);		// st1

			fld(dword_ptr[&sse[j][0]]);
			fld(dword_ptr[&sse[i][0]]);
			fcomip(st0, st1);
			switch (c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr[&sse[i][0]]);

			fld(dword_ptr[&sse[j][1]]);
			fld(dword_ptr[&sse[i][1]]);
			fcomip(st0, st1);
			switch (c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr[&sse[i][1]]);

			fld(dword_ptr[&sse[j][2]]);
			fld(dword_ptr[&sse[i][2]]);
			fcomip(st0, st1);
			switch (c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr[&sse[i][2]]);

			fld(dword_ptr[&sse[j][3]]);
			fld(dword_ptr[&sse[i][3]]);
			fcomip(st0, st1);
			switch (c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr[&sse[i][3]]);

			ffree(st0);
			ffree(st1);

			return 0;
		}

		return Optimizer::cmpps(xmmi, xmmj, c);
	}

	Encoding* Emulator::cmpps(OperandXMMREG xmm, OperandMEM128 mem128, char c)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static float zero = 0;
			static float one = 1;
			fld(dword_ptr[&zero]);		// st2
			fld(dword_ptr[&one]);		// st1

			fld((OperandMEM32)(mem128 + 0));
			fld(dword_ptr[&sse[i][0]]);
			fcomip(st0, st1);
			switch (c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr[&sse[i][0]]);

			fld((OperandMEM32)(mem128 + 4));
			fld(dword_ptr[&sse[i][1]]);
			fcomip(st0, st1);
			switch (c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr[&sse[i][1]]);

			fld((OperandMEM32)(mem128 + 8));
			fld(dword_ptr[&sse[i][2]]);
			fcomip(st0, st1);
			switch (c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr[&sse[i][2]]);

			fld((OperandMEM32)(mem128 + 12));
			fld(dword_ptr[&sse[i][3]]);
			fcomip(st0, st1);
			switch (c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr[&sse[i][3]]);

			ffree(st0);
			ffree(st1);

			return 0;
		}

		return Optimizer::cmpps(xmm, mem128, c);
	}

	Encoding* Emulator::cmpps(OperandXMMREG xmm, OperandR_M128 r_m128, char c)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return cmpps(xmm, (OperandXMMREG)r_m128, c);
		else                                       return cmpps(xmm, (OperandMEM128)r_m128, c);
	}

	Encoding* Emulator::cmpeqps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 0);
	}

	Encoding* Emulator::cmpeqps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 0);
	}

	Encoding* Emulator::cmpeqps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 0);
	}

	Encoding* Emulator::cmpleps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 2);
	}

	Encoding* Emulator::cmpleps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 2);
	}

	Encoding* Emulator::cmpleps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 2);
	}

	Encoding* Emulator::cmpltps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 1);
	}

	Encoding* Emulator::cmpltps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 1);
	}

	Encoding* Emulator::cmpltps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 1);
	}

	Encoding* Emulator::cmpneqps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 4);
	}

	Encoding* Emulator::cmpneqps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 4);
	}

	Encoding* Emulator::cmpneqps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 4);
	}

	Encoding* Emulator::cmpnleps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 6);
	}

	Encoding* Emulator::cmpnleps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 6);
	}

	Encoding* Emulator::cmpnleps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 6);
	}

	Encoding* Emulator::cmpnltps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 5);
	}

	Encoding* Emulator::cmpnltps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 5);
	}

	Encoding* Emulator::cmpnltps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 5);
	}

	Encoding* Emulator::cmpordps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 7);
	}

	Encoding* Emulator::cmpordps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 7);
	}

	Encoding* Emulator::cmpordps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 7);
	}

	Encoding* Emulator::cmpunordps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 3);
	}

	Encoding* Emulator::cmpunordps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 3);
	}

	Encoding* Emulator::cmpunordps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 3);
	}

	Encoding* Emulator::cmpss(OperandXMMREG xmmi, OperandXMMREG xmmj, char c)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			static float zero = 0;
			static float one = 1;
			fld(dword_ptr[&zero]);		// st2
			fld(dword_ptr[&one]);		// st1

			fld(dword_ptr[&sse[j][0]]);
			fld(dword_ptr[&sse[i][0]]);
			fcomip(st0, st1);
			switch (c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr[&sse[i][0]]);

			ffree(st0);
			ffree(st1);

			return 0;
		}

		return Optimizer::cmpss(xmmi, xmmj, c);
	}

	Encoding* Emulator::cmpss(OperandXMMREG xmm, OperandMEM32 mem32, char c)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static float zero = 0;
			static float one = 1;
			fld(dword_ptr[&zero]);		// st2
			fld(dword_ptr[&one]);		// st1

			fld((OperandMEM32)(mem32 + 0));
			fld(dword_ptr[&sse[i][0]]);
			fcomip(st0, st1);
			switch (c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr[&sse[i][0]]);

			ffree(st0);
			ffree(st1);

			return 0;
		}

		return Optimizer::cmpss(xmm, mem32, c);
	}

	Encoding* Emulator::cmpss(OperandXMMREG xmm, OperandXMM32 xmm32, char c)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return cmpss(xmm, (OperandXMMREG)xmm32, c);
		else                                      return cmpss(xmm, (OperandMEM32)xmm32, c);
	}

	Encoding* Emulator::cmpeqss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 0);
	}

	Encoding* Emulator::cmpeqss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 0);
	}

	Encoding* Emulator::cmpeqss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 0);
	}

	Encoding* Emulator::cmpless(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 2);
	}

	Encoding* Emulator::cmpless(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 2);
	}

	Encoding* Emulator::cmpless(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 2);
	}

	Encoding* Emulator::cmpltss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 1);
	}

	Encoding* Emulator::cmpltss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 1);
	}

	Encoding* Emulator::cmpltss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 1);
	}

	Encoding* Emulator::cmpneqss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 4);
	}

	Encoding* Emulator::cmpneqss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 4);
	}

	Encoding* Emulator::cmpneqss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 4);
	}

	Encoding* Emulator::cmpnless(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 6);
	}

	Encoding* Emulator::cmpnless(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 6);
	}

	Encoding* Emulator::cmpnless(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 6);
	}

	Encoding* Emulator::cmpnltss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 5);
	}

	Encoding* Emulator::cmpnltss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 5);
	}

	Encoding* Emulator::cmpnltss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 5);
	}

	Encoding* Emulator::cmpordss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 7);
	}

	Encoding* Emulator::cmpordss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 7);
	}

	Encoding* Emulator::cmpordss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 7);
	}

	Encoding* Emulator::cmpunordss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 3);
	}

	Encoding* Emulator::cmpunordss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 3);
	}

	Encoding* Emulator::cmpunordss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 3);
	}

	Encoding* Emulator::comiss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			fld(dword_ptr[&sse[j][0]]);
			fld(dword_ptr[&sse[i][0]]);
			fcomip(st0, st1);
			ffree(st0);

			return 0;
		}

		return Optimizer::comiss(xmmi, xmmj);
	}

	Encoding* Emulator::comiss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fld(mem32);
			fld(dword_ptr[&sse[i][0]]);
			fcomip(st0, st1);
			ffree(st0);

			return 0;
		}

		return Optimizer::comiss(xmm, mem32);
	}

	Encoding* Emulator::comiss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return comiss(xmm, (OperandXMMREG)xmm32);
		else                                      return comiss(xmm, (OperandMEM32)xmm32);
	}

	Encoding* Emulator::cvtpi2ps(OperandXMMREG xmm, OperandMMREG mm)
	{
		if (emulateSSE)
		{
			static int dword[2];
			movq(qword_ptr[dword], mm);
			const int i = xmm.reg;
			spillMMX();

			fild(dword_ptr[&dword[0]]);
			fstp(dword_ptr[&sse[i][0]]);
			fild(dword_ptr[&dword[1]]);
			fstp(dword_ptr[&sse[i][1]]);

			return 0;
		}

		return Optimizer::cvtpi2ps(xmm, mm);
	}

	Encoding* Emulator::cvtpi2ps(OperandXMMREG xmm, OperandMEM64 mem64)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fild((OperandMEM32)(mem64 + 0));
			fstp(dword_ptr[&sse[i][0]]);
			fild((OperandMEM32)(mem64 + 4));
			fstp(dword_ptr[&sse[i][1]]);

			return 0;
		}

		return Optimizer::cvtpi2ps(xmm, mem64);
	}

	Encoding* Emulator::cvtpi2ps(OperandXMMREG xmm, OperandMM64 r_m64)
	{
		if (r_m64.type == Operand::OPERAND_MMREG) return cvtpi2ps(xmm, (OperandMMREG)r_m64);
		else                                     return cvtpi2ps(xmm, (OperandMEM64)r_m64);
	}

	Encoding* Emulator::cvtps2pi(OperandMMREG mm, OperandXMMREG xmm)
	{
		if (emulateSSE)
		{
			static int dword[2];

			spillMMXcept(mm);
			const int i = xmm.reg;
			//	short fpuCW1;
			//	short fpuCW2;

			//	fldcw(word_ptr [&fpuCW1]);
			//	fldcw(word_ptr [&fpuCW2]);
			//	and(word_ptr [&fpuCW2], (short)0xF3FF);
			//	fstcw(word_ptr [&fpuCW2]);

			fld(dword_ptr[&sse[i][0]]);
			fistp(dword_ptr[&dword[0]]);
			fld(dword_ptr[&sse[i][1]]);
			fistp(dword_ptr[&dword[1]]);

			//	fstcw(word_ptr [&fpuCW1]);
			movq(mm, qword_ptr[dword]);

			return 0;
		}

		return Optimizer::cvtps2pi(mm, xmm);
	}

	Encoding* Emulator::cvtps2pi(OperandMMREG mm, OperandMEM64 mem64)
	{
		if (emulateSSE)
		{
			static int dword[2];

			spillMMXcept(mm);
			//	short fpuCW1;
			//	short fpuCW2;

			//	fldcw(word_ptr [&fpuCW1]);
			//	fldcw(word_ptr [&fpuCW2]);
			//	and(word_ptr [&fpuCW2], (short)0xF3FF);
			//	fstcw(word_ptr [&fpuCW2]);

			fld((OperandMEM32)(mem64 + 0));
			fistp(dword_ptr[&dword[0]]);
			fld((OperandMEM32)(mem64 + 4));
			fistp(dword_ptr[&dword[1]]);

			//	fstcw(word_ptr [&fpuCW1]);
			movq(mm, qword_ptr[dword]);

			return 0;
		}

		return Optimizer::cvtps2pi(mm, mem64);
	}

	Encoding* Emulator::cvtps2pi(OperandMMREG mm, OperandXMM64 xmm64)
	{
		if (xmm64.type == Operand::OPERAND_XMMREG) return cvtps2pi(mm, (OperandXMMREG)xmm64);
		else                                      return cvtps2pi(mm, (OperandMEM64)xmm64);
	}

	Encoding* Emulator::cvttps2pi(OperandMMREG mm, OperandXMMREG xmm)
	{
		if (emulateSSE)
		{
			static int dword[2];
			spillMMXcept(mm);
			const int i = xmm.reg;
			short fpuCW1;
			short fpuCW2;

			fstcw(word_ptr[&fpuCW1]);
			fstcw(word_ptr[&fpuCW2]);
			or (word_ptr[&fpuCW2], (unsigned short)0x0C00);
			fldcw(word_ptr[&fpuCW2]);

			fld(dword_ptr[&sse[i][0]]);
			fistp(dword_ptr[&dword[0]]);
			fld(dword_ptr[&sse[i][1]]);
			fistp(dword_ptr[&dword[1]]);

			fldcw(word_ptr[&fpuCW1]);
			movq(mm, qword_ptr[dword]);

			return 0;
		}

		return Optimizer::cvttps2pi(mm, xmm);
	}

	Encoding* Emulator::cvttps2pi(OperandMMREG mm, OperandMEM64 mem64)
	{
		if (emulateSSE)
		{
			static int dword[2];

			spillMMXcept(mm);
			static short fpuCW1;
			static short fpuCW2;

			fstcw(word_ptr[&fpuCW1]);
			fstcw(word_ptr[&fpuCW2]);
			or (word_ptr[&fpuCW2], (unsigned short)0x0C00);
			fldcw(word_ptr[&fpuCW2]);

			fld((OperandMEM32)(mem64 + 0));
			fistp(dword_ptr[&dword[0]]);
			fld((OperandMEM32)(mem64 + 4));
			fistp(dword_ptr[&dword[1]]);

			fldcw(word_ptr[&fpuCW1]);
			movq(mm, qword_ptr[dword]);

			return 0;
		}

		return Optimizer::cvttps2pi(mm, mem64);
	}

	Encoding* Emulator::cvttps2pi(OperandMMREG mm, OperandXMM64 xmm64)
	{
		if (xmm64.type == Operand::OPERAND_XMMREG) return cvttps2pi(mm, (OperandXMMREG)xmm64);
		else                                      return cvttps2pi(mm, (OperandMEM64)xmm64);
	}

	Encoding* Emulator::cvtsi2ss(OperandXMMREG xmm, OperandREG32 reg32)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static int dword;

			mov(dword_ptr[&dword], reg32);
			fild(dword_ptr[&dword]);
			fstp(dword_ptr[&sse[i][0]]);

			return 0;
		}

		return Optimizer::cvtsi2ss(xmm, reg32);
	}

	Encoding* Emulator::cvtsi2ss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fild(mem32);
			fstp(dword_ptr[&sse[i][0]]);

			return 0;
		}

		return Optimizer::cvtsi2ss(xmm, mem32);
	}

	Encoding* Emulator::cvtsi2ss(OperandXMMREG xmm, OperandR_M32 r_m32)
	{
		if (r_m32.type == Operand::OPERAND_REG32) return cvtsi2ss(xmm, (OperandREG32)r_m32);
		else                                     return cvtsi2ss(xmm, (OperandMEM32)r_m32);
	}

	Encoding* Emulator::cvtss2si(OperandREG32 reg32, OperandXMMREG xmm)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			//	short fpuCW1;
			//	short fpuCW2;
			static int dword;

			//	fldcw(word_ptr [&fpuCW1]);
			//	fldcw(word_ptr [&fpuCW2]);
			//	and(word_ptr [&fpuCW2], (short)0xF3FF);
			//	fstcw(word_ptr [&fpuCW2]);

			fld(dword_ptr[&sse[i][0]]);
			fistp(dword_ptr[&dword]);
			mov(reg32, dword_ptr[&dword]);

			//	fstcw(word_ptr [&fpuCW1]);

			return 0;
		}

		return Optimizer::cvtss2si(reg32, xmm);
	}

	Encoding* Emulator::cvtss2si(OperandREG32 reg32, OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			spillMMX();
			//	short fpuCW1;
			//	short fpuCW2;
			static int dword;

			//	fldcw(word_ptr [&fpuCW1]);
			//	fldcw(word_ptr [&fpuCW2]);
			//	and(word_ptr [&fpuCW2], (short)0xF3FF);
			//	fstcw(word_ptr [&fpuCW2]);

			fld(mem32);
			fistp(dword_ptr[&dword]);
			mov(reg32, dword_ptr[&dword]);

			//	fstcw(word_ptr [&fpuCW1]);

			return 0;
		}

		return Optimizer::cvtss2si(reg32, mem32);
	}

	Encoding* Emulator::cvtss2si(OperandREG32 reg32, OperandXMM32 xmm32)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return cvtss2si(reg32, (OperandXMMREG)xmm32);
		else                                      return cvtss2si(reg32, (OperandMEM32)xmm32);
	}

	Encoding* Emulator::cvttss2si(OperandREG32 reg32, OperandXMMREG xmm)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static short fpuCW1;
			static short fpuCW2;
			static int dword;

			fstcw(word_ptr[&fpuCW1]);
			fstcw(word_ptr[&fpuCW2]);
			or (word_ptr[&fpuCW2], (unsigned short)0x0C00);
			fldcw(word_ptr[&fpuCW2]);

			fld(dword_ptr[&sse[i][0]]);
			fistp(dword_ptr[&dword]);
			mov(reg32, dword_ptr[&dword]);

			fldcw(word_ptr[&fpuCW1]);

			return 0;
		}

		return Optimizer::cvttss2si(reg32, xmm);
	}

	Encoding* Emulator::cvttss2si(OperandREG32 reg32, OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			spillMMX();
			static short fpuCW1;
			static short fpuCW2;
			static int dword;

			fstcw(word_ptr[&fpuCW1]);
			fstcw(word_ptr[&fpuCW2]);
			or (word_ptr[&fpuCW2], (unsigned short)0x0C00);
			fldcw(word_ptr[&fpuCW2]);

			fld(mem32);
			fistp(dword_ptr[&dword]);
			mov(reg32, dword_ptr[&dword]);

			fldcw(word_ptr[&fpuCW1]);

			return 0;
		}

		return Optimizer::cvttss2si(reg32, mem32);
	}

	Encoding* Emulator::cvttss2si(OperandREG32 reg32, OperandXMM32 xmm32)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return cvttss2si(reg32, (OperandXMMREG)xmm32);
		else                                      return cvttss2si(reg32, (OperandMEM32)xmm32);
	}

	Encoding* Emulator::divps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr[&sse[i][0]]);
			fdiv(dword_ptr[&sse[j][0]]);
			fstp(dword_ptr[&sse[i][0]]);
			fld(dword_ptr[&sse[i][1]]);
			fdiv(dword_ptr[&sse[j][1]]);
			fstp(dword_ptr[&sse[i][1]]);
			fld(dword_ptr[&sse[i][2]]);
			fdiv(dword_ptr[&sse[j][2]]);
			fstp(dword_ptr[&sse[i][2]]);
			fld(dword_ptr[&sse[i][3]]);
			fdiv(dword_ptr[&sse[j][3]]);
			fstp(dword_ptr[&sse[i][3]]);
			return 0;
		}

		return Optimizer::divps(xmmi, xmmj);
	}

	Encoding* Emulator::divps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld(dword_ptr[&sse[i][0]]);
			fdiv((OperandMEM32)(mem128 + 0));
			fstp(dword_ptr[&sse[i][0]]);
			fld(dword_ptr[&sse[i][1]]);
			fdiv((OperandMEM32)(mem128 + 4));
			fstp(dword_ptr[&sse[i][1]]);
			fld(dword_ptr[&sse[i][2]]);
			fdiv((OperandMEM32)(mem128 + 8));
			fstp(dword_ptr[&sse[i][2]]);
			fld(dword_ptr[&sse[i][3]]);
			fdiv((OperandMEM32)(mem128 + 12));
			fstp(dword_ptr[&sse[i][3]]);
			return 0;
		}

		return Optimizer::divps(xmm, mem128);
	}

	Encoding* Emulator::divps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return divps(xmm, (OperandXMMREG)r_m128);
		else                                       return divps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::divss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr[&sse[i][0]]);
			fdiv(dword_ptr[&sse[j][0]]);
			fstp(dword_ptr[&sse[i][0]]);
			return 0;
		}

		return Optimizer::divss(xmmi, xmmj);
	}

	Encoding* Emulator::divss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld(dword_ptr[&sse[i][0]]);
			fdiv((OperandMEM32)mem32);
			fstp(dword_ptr[&sse[i][0]]);
			return 0;
		}

		return Optimizer::divss(xmm, mem32);
	}

	Encoding* Emulator::divss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return divss(xmm, (OperandXMMREG)xmm32);
		else                                      return divss(xmm, (OperandMEM32)xmm32);
	}

	Encoding* Emulator::ldmxcsr(OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			return 0;
		}

		return Optimizer::ldmxcsr(mem32);
	}

	Encoding* Emulator::maskmovq(OperandMMREG mmi, OperandMMREG mmj)
	{
		if (emulateSSE)
		{
			static short qword1[4];
			static short qword2[4];

			movq(qword_ptr[&qword1], mmi);
			movq(qword_ptr[&qword2], mmj);

			test(byte_ptr[&qword2 + (intptr_t)0], (char)0x80);
			mov(t8(0), byte_ptr[edi + (intptr_t)0]);
			cmovnz(t32(0), dword_ptr[&qword1 + (intptr_t)0]);
			mov(byte_ptr[edi + (intptr_t)0], t8(0));

			test(byte_ptr[&qword2 + 1], (char)0x80);
			mov(t8(0), byte_ptr[edi + 1]);
			cmovnz(t32(0), dword_ptr[&qword1 + 1]);
			mov(byte_ptr[edi + 1], t8(0));

			test(byte_ptr[&qword2 + 2], (char)0x80);
			mov(t8(0), byte_ptr[edi + 2]);
			cmovnz(t32(0), dword_ptr[&qword1 + 2]);
			mov(byte_ptr[edi + 2], t8(0));

			test(byte_ptr[&qword2 + 3], (char)0x80);
			mov(t8(0), byte_ptr[edi + 3]);
			cmovnz(t32(0), dword_ptr[&qword1 + 3]);
			mov(byte_ptr[edi + 3], t8(0));

			test(byte_ptr[&qword2 + 4], (char)0x80);
			mov(t8(0), byte_ptr[edi + (intptr_t)4]);
			cmovnz(t32(0), dword_ptr[&qword1 + 4]);
			mov(byte_ptr[edi + (intptr_t)4], t8(0));

			test(byte_ptr[&qword2 + 5], (char)0x80);
			mov(t8(0), byte_ptr[edi + (intptr_t)5]);
			cmovnz(t32(0), dword_ptr[&qword1 + 5]);
			mov(byte_ptr[edi + 5], t8(0));

			test(byte_ptr[&qword2 + 6], (char)0x80);
			mov(t8(0), byte_ptr[edi + (intptr_t)6]);
			cmovnz(t32(0), dword_ptr[&qword1 + 6]);
			mov(byte_ptr[edi + 6], t8(0));

			test(byte_ptr[&qword2 + (intptr_t)7], (char)0x80);
			mov(t8(0), byte_ptr[edi + (intptr_t)7]);
			cmovnz(t32(0), dword_ptr[&qword1 + 7]);
			mov(byte_ptr[edi + (intptr_t)7], t8(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::maskmovq(mmi, mmj);
	}

	Encoding* Emulator::maxps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			fld(dword_ptr[&sse[j][0]]);
			fld(dword_ptr[&sse[i][0]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr[&sse[i][0]]);
			ffree(st0);

			fld(dword_ptr[&sse[j][1]]);
			fld(dword_ptr[&sse[i][1]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr[&sse[i][1]]);
			ffree(st0);

			fld(dword_ptr[&sse[j][2]]);
			fld(dword_ptr[&sse[i][2]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr[&sse[i][2]]);
			ffree(st0);

			fld(dword_ptr[&sse[j][3]]);
			fld(dword_ptr[&sse[i][3]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr[&sse[i][3]]);
			ffree(st0);

			return 0;
		}

		return Optimizer::maxps(xmmi, xmmj);
	}

	Encoding* Emulator::maxps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fld((OperandMEM32)(mem128 + 0));
			fld(dword_ptr[&sse[i][0]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr[&sse[i][0]]);
			ffree(st0);

			fld((OperandMEM32)(mem128 + 4));
			fld(dword_ptr[&sse[i][1]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr[&sse[i][1]]);
			ffree(st0);

			fld((OperandMEM32)(mem128 + 8));
			fld(dword_ptr[&sse[i][2]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr[&sse[i][2]]);
			ffree(st0);

			fld((OperandMEM32)(mem128 + 0));
			fld(dword_ptr[&sse[i][3]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr[&sse[i][3]]);
			ffree(st0);

			return 0;
		}

		return Optimizer::maxps(xmm, mem128);
	}

	Encoding* Emulator::maxps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return maxps(xmm, (OperandXMMREG)r_m128);
		else                                       return maxps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::maxss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			fld(dword_ptr[&sse[j][0]]);
			fld(dword_ptr[&sse[i][0]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr[&sse[i][0]]);
			ffree(st0);

			return 0;
		}

		return Optimizer::maxss(xmmi, xmmj);
	}

	Encoding* Emulator::maxss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fld(mem32);
			fld(dword_ptr[&sse[i][0]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr[&sse[i][0]]);
			ffree(st0);

			return 0;
		}

		return Optimizer::maxss(xmm, mem32);
	}

	Encoding* Emulator::maxss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return maxss(xmm, (OperandXMMREG)xmm32);
		else                                      return maxss(xmm, (OperandMEM32)xmm32);
	}

	Encoding* Emulator::minps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			fld(dword_ptr[&sse[j][0]]);
			fld(dword_ptr[&sse[i][0]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr[&sse[i][0]]);
			ffree(st0);

			fld(dword_ptr[&sse[j][1]]);
			fld(dword_ptr[&sse[i][1]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr[&sse[i][1]]);
			ffree(st0);

			fld(dword_ptr[&sse[j][2]]);
			fld(dword_ptr[&sse[i][2]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr[&sse[i][2]]);
			ffree(st0);

			fld(dword_ptr[&sse[j][3]]);
			fld(dword_ptr[&sse[i][3]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr[&sse[i][3]]);
			ffree(st0);

			return 0;
		}

		return Optimizer::minps(xmmi, xmmj);
	}

	Encoding* Emulator::minps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fld((OperandMEM32)(mem128 + 0));
			fld(dword_ptr[&sse[i][0]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr[&sse[i][0]]);
			ffree(st0);

			fld((OperandMEM32)(mem128 + 4));
			fld(dword_ptr[&sse[i][1]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr[&sse[i][1]]);
			ffree(st0);

			fld((OperandMEM32)(mem128 + 8));
			fld(dword_ptr[&sse[i][2]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr[&sse[i][2]]);
			ffree(st0);

			fld((OperandMEM32)(mem128 + 0));
			fld(dword_ptr[&sse[i][3]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr[&sse[i][3]]);
			ffree(st0);

			return 0;
		}

		return Optimizer::minps(xmm, mem128);
	}

	Encoding* Emulator::minps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return minps(xmm, (OperandXMMREG)r_m128);
		else                                       return minps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::minss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			fld(dword_ptr[&sse[j][0]]);
			fld(dword_ptr[&sse[i][0]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr[&sse[i][0]]);
			ffree(st0);

			return 0;
		}

		return Optimizer::minss(xmmi, xmmj);
	}

	Encoding* Emulator::minss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fld(mem32);
			fld(dword_ptr[&sse[i][0]]);
			fucomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr[&sse[i][0]]);
			ffree(st0);

			return 0;
		}

		return Optimizer::minss(xmm, mem32);
	}

	Encoding* Emulator::minss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return minss(xmm, (OperandXMMREG)xmm32);
		else                                      return minss(xmm, (OperandMEM32)xmm32);
	}

	Encoding* Emulator::movaps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr[&sse[j][0]]);
			mov(dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][1]]);
			mov(dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][2]]);
			mov(dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][3]]);
			mov(dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movaps(xmmi, xmmj);
	}

	Encoding* Emulator::movaps(OperandXMMREG xmm, OperandMEM128 m128)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(m128 + 0));
			mov(dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), (OperandMEM32)(m128 + 4));
			mov(dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(m128 + 8));
			mov(dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(m128 + 12));
			mov(dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movaps(xmm, m128);
	}

	Encoding* Emulator::movaps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return movaps(xmm, (OperandXMMREG)r_m128);
		else                                       return movaps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::movaps(OperandMEM128 m128, OperandXMMREG xmm)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr[&sse[i][0]]);
			mov((OperandMEM32)(m128 + 0), t32(0));

			mov(t32(0), dword_ptr[&sse[i][1]]);
			mov((OperandMEM32)(m128 + 4), t32(0));

			mov(t32(0), dword_ptr[&sse[i][2]]);
			mov((OperandMEM32)(m128 + 8), t32(0));

			mov(t32(0), dword_ptr[&sse[i][3]]);
			mov((OperandMEM32)(m128 + 12), t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movaps(m128, xmm);
	}

	Encoding* Emulator::movaps(OperandR_M128 r_m128, OperandXMMREG xmm)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return movaps((OperandXMMREG)r_m128, xmm);
		else                                       return movaps((OperandMEM128)r_m128, xmm);
	}

	Encoding* Emulator::movhlps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr[&sse[j][2]]);
			mov(dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][3]]);
			mov(dword_ptr[&sse[i][1]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movhlps(xmmi, xmmj);
	}

	Encoding* Emulator::movhps(OperandXMMREG xmm, OperandMEM64 m64)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(m64 + 0));
			mov(dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(m64 + 4));
			mov(dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movhps(xmm, m64);
	}

	Encoding* Emulator::movhps(OperandMEM64 m64, OperandXMMREG xmm)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr[&sse[i][2]]);
			mov((OperandMEM32)(m64 + 0), t32(0));

			mov(t32(0), dword_ptr[&sse[i][3]]);
			mov((OperandMEM32)(m64 + 4), t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movhps(m64, xmm);
	}

	Encoding* Emulator::movhps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr[&sse[j][2]]);
			mov(dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][3]]);
			mov(dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movhps(xmmi, xmmj);
	}

	Encoding* Emulator::movlhps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr[&sse[j][0]]);
			mov(dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][1]]);
			mov(dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movlhps(xmmi, xmmj);
	}

	Encoding* Emulator::movlps(OperandXMMREG xmm, OperandMEM64 m64)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(m64 + 0));
			mov(dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), (OperandMEM32)(m64 + 4));
			mov(dword_ptr[&sse[i][1]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movlps(xmm, m64);
	}

	Encoding* Emulator::movlps(OperandMEM64 m64, OperandXMMREG xmm)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr[&sse[i][0]]);
			mov((OperandMEM32)(m64 + 0), t32(0));

			mov(t32(0), dword_ptr[&sse[i][1]]);
			mov((OperandMEM32)(m64 + 4), t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movlps(m64, xmm);
	}

	Encoding* Emulator::movmskps(OperandREG32 reg32, OperandXMMREG xmm)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr[&sse[i][0]]);
			shr(t32(0), 31);
			mov(reg32, t32(0));

			mov(t32(0), dword_ptr[&sse[i][1]]);
			shr(t32(0), 31);
			shl(t32(0), 1);
			or (reg32, t32(0));

			mov(t32(0), dword_ptr[&sse[i][2]]);
			shr(t32(0), 31);
			shl(t32(0), 2);
			or (reg32, t32(0));

			mov(t32(0), dword_ptr[&sse[i][3]]);
			shr(t32(0), 31);
			shl(t32(0), 3);
			or (reg32, t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movmskps(reg32, xmm);
	}

	Encoding* Emulator::movntps(OperandMEM128 m128, OperandXMMREG xmm)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr[&sse[i][0]]);
			mov((OperandMEM32)(m128 + 0), t32(0));

			mov(t32(0), dword_ptr[&sse[i][1]]);
			mov((OperandMEM32)(m128 + 4), t32(0));

			mov(t32(0), dword_ptr[&sse[i][2]]);
			mov((OperandMEM32)(m128 + 8), t32(0));

			mov(t32(0), dword_ptr[&sse[i][3]]);
			mov((OperandMEM32)(m128 + 12), t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movntps(m128, xmm);
	}

	Encoding* Emulator::movntq(OperandMEM64 m64, OperandMMREG xmm)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr[&sse[i][0]]);
			mov((OperandMEM32)(m64 + 0), t32(0));

			mov(t32(0), dword_ptr[&sse[i][1]]);
			mov((OperandMEM32)(m64 + 4), t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movntq(m64, xmm);
	}

	Encoding* Emulator::movss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr[&sse[j][0]]);
			mov(dword_ptr[&sse[i][0]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movss(xmmi, xmmj);
	}

	Encoding* Emulator::movss(OperandXMMREG xmm, OperandMEM32 m32)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), m32);
			mov(dword_ptr[&sse[i][0]], t32(0));

			mov(dword_ptr[&sse[i][1]], 0);
			mov(dword_ptr[&sse[i][2]], 0);
			mov(dword_ptr[&sse[i][3]], 0);

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movss(xmm, m32);
	}

	Encoding* Emulator::movss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return movss(xmm, (OperandXMMREG)xmm32);
		else                                      return movss(xmm, (OperandMEM32)xmm32);
	}

	Encoding* Emulator::movss(OperandMEM32 m32, OperandXMMREG xmm)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr[&sse[i][0]]);
			mov(m32, t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movss(m32, xmm);
	}

	Encoding* Emulator::movss(OperandXMM32 xmm32, OperandXMMREG xmm)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return movss((OperandXMMREG)xmm32, xmm);
		else                                      return movss((OperandMEM32)xmm32, xmm);
	}

	Encoding* Emulator::movups(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr[&sse[j][0]]);
			mov(dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][1]]);
			mov(dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][2]]);
			mov(dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][3]]);
			mov(dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movups(xmmi, xmmj);
	}

	Encoding* Emulator::movups(OperandXMMREG xmm, OperandMEM128 m128)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(m128 + 0));
			mov(dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), (OperandMEM32)(m128 + 4));
			mov(dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(m128 + 8));
			mov(dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(m128 + 12));
			mov(dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::movups(xmm, m128);
	}

	Encoding* Emulator::movups(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (emulateSSE)
		{
			return movaps(xmm, r_m128);
		}
		else
		{
			return Optimizer::movups(xmm, r_m128);
		}
	}

	Encoding* Emulator::movups(OperandMEM128 m128, OperandXMMREG xmm)
	{
		if (emulateSSE)
		{
			return movaps(m128, xmm);
		}
		else
		{
			return Optimizer::movups(m128, xmm);
		}
	}

	Encoding* Emulator::movups(OperandR_M128 r_m128, OperandXMMREG xmm)
	{
		if (emulateSSE)
		{
			return movaps(r_m128, xmm);
		}
		else
		{
			return Optimizer::movups(r_m128, xmm);
		}
	}

	Encoding* Emulator::mulps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr[&sse[i][0]]);
			fmul(dword_ptr[&sse[j][0]]);
			fstp(dword_ptr[&sse[i][0]]);
			fld(dword_ptr[&sse[i][1]]);
			fmul(dword_ptr[&sse[j][1]]);
			fstp(dword_ptr[&sse[i][1]]);
			fld(dword_ptr[&sse[i][2]]);
			fmul(dword_ptr[&sse[j][2]]);
			fstp(dword_ptr[&sse[i][2]]);
			fld(dword_ptr[&sse[i][3]]);
			fmul(dword_ptr[&sse[j][3]]);
			fstp(dword_ptr[&sse[i][3]]);
			return 0;
		}

		return Optimizer::mulps(xmmi, xmmj);
	}

	Encoding* Emulator::mulps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld(dword_ptr[&sse[i][0]]);
			fmul((OperandMEM32)(mem128 + 0));
			fstp(dword_ptr[&sse[i][0]]);
			fld(dword_ptr[&sse[i][1]]);
			fmul((OperandMEM32)(mem128 + 4));
			fstp(dword_ptr[&sse[i][1]]);
			fld(dword_ptr[&sse[i][2]]);
			fmul((OperandMEM32)(mem128 + 8));
			fstp(dword_ptr[&sse[i][2]]);
			fld(dword_ptr[&sse[i][3]]);
			fmul((OperandMEM32)(mem128 + 12));
			fstp(dword_ptr[&sse[i][3]]);
			return 0;
		}

		return Optimizer::mulps(xmm, mem128);
	}

	Encoding* Emulator::mulps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return mulps(xmm, (OperandXMMREG)r_m128);
		else                                       return mulps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::mulss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr[&sse[i][0]]);
			fmul(dword_ptr[&sse[j][0]]);
			fstp(dword_ptr[&sse[i][0]]);
			return 0;
		}

		return Optimizer::mulss(xmmi, xmmj);
	}

	Encoding* Emulator::mulss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld(dword_ptr[&sse[i][0]]);
			fmul((OperandMEM32)mem32);
			fstp(dword_ptr[&sse[i][0]]);
			return 0;
		}

		return Optimizer::mulss(xmm, mem32);
	}

	Encoding* Emulator::mulss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return mulss(xmm, (OperandXMMREG)xmm32);
		else                                      return mulss(xmm, (OperandMEM32)xmm32);
	}

	Encoding* Emulator::orps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr[&sse[j][0]]);
			or (dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][1]]);
			or (dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][2]]);
			or (dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][3]]);
			or (dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::orps(xmmi, xmmj);
	}

	Encoding* Emulator::orps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(mem128 + 0));
			or (dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 4));
			or (dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 8));
			or (dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 12));
			or (dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::orps(xmm, mem128);
	}

	Encoding* Emulator::orps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return orps(xmm, (OperandXMMREG)r_m128);
		else                                       return orps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::pavgb(OperandMMREG mmi, OperandMMREG mmj)
	{
		if (emulateSSE)
		{
			static unsigned char byte1[8];
			static unsigned char byte2[8];

			movq(qword_ptr[byte1], mmi);
			movq(qword_ptr[byte2], mmj);

			movzx(t32(0), byte_ptr[&byte1[0]]);
			movzx(t32(1), byte_ptr[&byte2[0]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[0]], t8(0));

			movzx(t32(0), byte_ptr[&byte1[1]]);
			movzx(t32(1), byte_ptr[&byte2[1]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[1]], t8(0));

			movzx(t32(0), byte_ptr[&byte1[2]]);
			movzx(t32(1), byte_ptr[&byte2[2]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[2]], t8(0));

			movzx(t32(0), byte_ptr[&byte1[3]]);
			movzx(t32(1), byte_ptr[&byte2[3]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[3]], t8(0));

			movzx(t32(0), byte_ptr[&byte1[4]]);
			movzx(t32(1), byte_ptr[&byte2[4]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[4]], t8(0));

			movzx(t32(0), byte_ptr[&byte1[5]]);
			movzx(t32(1), byte_ptr[&byte2[5]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[5]], t8(0));

			movzx(t32(0), byte_ptr[&byte1[6]]);
			movzx(t32(1), byte_ptr[&byte2[6]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[6]], t8(0));

			movzx(t32(0), byte_ptr[&byte1[7]]);
			movzx(t32(1), byte_ptr[&byte2[7]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[7]], t8(0));

			movq(mmi, qword_ptr[byte1]);

			free((OperandREF)(intptr_t)0);
			free((OperandREF)1);
			return 0;
		}

		return Optimizer::pavgb(mmi, mmj);
	}

	Encoding* Emulator::pavgb(OperandMMREG mm, OperandMEM64 m64)
	{
		if (emulateSSE)
		{
			static unsigned char byte1[8];

			movq(qword_ptr[byte1], mm);

			static int t1;

			movzx(t32(0), byte_ptr[&byte1[0]]);
			movzx(t32(1), (OperandMEM8)(m64 + 0));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[0]], t8(0));

			movzx(t32(0), byte_ptr[&byte1[1]]);
			movzx(t32(1), (OperandMEM8)(m64 + 1));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[1]], t8(0));

			movzx(t32(0), byte_ptr[&byte1[2]]);
			movzx(t32(1), (OperandMEM8)(m64 + 2));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[2]], t8(0));

			movzx(t32(0), byte_ptr[&byte1[3]]);
			movzx(t32(1), (OperandMEM8)(m64 + 3));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[3]], t8(0));

			movzx(t32(0), byte_ptr[&byte1[4]]);
			movzx(t32(1), (OperandMEM8)(m64 + 4));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[4]], t8(0));

			movzx(t32(0), byte_ptr[&byte1[5]]);
			movzx(t32(1), (OperandMEM8)(m64 + 5));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[5]], t8(0));

			movzx(t32(0), byte_ptr[&byte1[6]]);
			movzx(t32(1), (OperandMEM8)(m64 + 6));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[6]], t8(0));

			movzx(t32(0), byte_ptr[&byte1[7]]);
			movzx(t32(1), (OperandMEM8)(m64 + 7));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr[&byte1[7]], t8(0));

			movq(mm, qword_ptr[byte1]);

			free((OperandREF)(intptr_t)0);
			free((OperandREF)1);
			return 0;
		}

		return Optimizer::pavgb(mm, m64);
	}
	/*
						mov			edi, src1p
						mov			esi, src2p
						mov			ebx, myy
						movq		mm0, ox7f7f7f7f7f7f7f7f	;get shift mask

					fastyuy32loop:
							mov         edx, myx
							xor         ecx, ecx
							shr			edx,1

							align 16
						fastyuy32xloop:
								//---- fetch src1/dest

								movq		mm7, [edi + ecx*8] ;src1/dest;
								movq		mm6, [esi + ecx*8] ;src2
								movq		mm5, mm7
								pxor		mm7, mm6
	# if 1			// ------------------------------------------------
							// Use (a + b + 1) >> 1 = (a | b) - ((a ^ b) >> 1)
								por			mm6, mm5
								psrlq		mm7, 1		// Fuck Intel! Where is psrlb
								inc         ecx
								pand		mm7, mm0
								psubb		mm6, mm7
	# else			// ------------------------------------------------
							// Use (a + b) >> 1 = (a & b) + ((a ^ b) >> 1)
								pand		mm6, mm5
								psrlq		mm7, 1		// Fuck Intel! Where is psrlb
								inc         ecx
								pand		mm7, mm0
								paddb		mm6, mm7
	# endif			// ------------------------------------------------
								cmp         ecx, edx
								movq        [edi + ecx*8 - 8],mm6
							jnz         fastyuy32xloop

							add			edi, src1_pitch
							add			esi, src2_pitch
							dec		ebx
						jnz		fastyuy32loop
						emms
	*/

	Encoding* Emulator::pavgb(OperandMMREG mm, OperandMM64 r_m64)
	{
		if (r_m64.type == Operand::OPERAND_MMREG) return pavgb(mm, (OperandMMREG)r_m64);
		else                                     return pavgb(mm, (OperandMEM64)r_m64);
	}

	Encoding* Emulator::pavgw(OperandMMREG mmi, OperandMMREG mmj)
	{
		if (emulateSSE)
		{
			static unsigned short word1[4];
			static unsigned short word2[4];

			movq(qword_ptr[word1], mmi);
			movq(qword_ptr[word2], mmj);

			movzx(t32(0), word_ptr[&word1[0]]);
			movzx(t32(1), word_ptr[&word2[0]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr[&word1[0]], t16(0));

			movzx(t32(0), word_ptr[&word1[1]]);
			movzx(t32(1), word_ptr[&word2[1]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr[&word1[1]], t16(0));

			movzx(t32(0), word_ptr[&word1[2]]);
			movzx(t32(1), word_ptr[&word2[2]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr[&word1[2]], t16(0));

			movzx(t32(0), word_ptr[&word1[3]]);
			movzx(t32(1), word_ptr[&word2[3]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr[&word1[3]], t16(0));

			movq(mmi, qword_ptr[word1]);

			free((OperandREF)(intptr_t)0);
			free((OperandREF)1);
			return 0;
		}

		return Optimizer::pavgw(mmi, mmj);
	}

	Encoding* Emulator::pavgw(OperandMMREG mm, OperandMEM64 m64)
	{
		if (emulateSSE)
		{
			static unsigned char word1[8];

			movq(qword_ptr[word1], mm);

			movzx(t32(0), word_ptr[&word1[0]]);
			movzx(t32(1), (OperandMEM16)(m64 + 0));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr[&word1[0]], t16(0));

			movzx(t32(0), word_ptr[&word1[1]]);
			movzx(t32(1), (OperandMEM16)(m64 + 2));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr[&word1[1]], t16(0));

			movzx(t32(0), word_ptr[&word1[2]]);
			movzx(t32(1), (OperandMEM16)(m64 + 4));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr[&word1[2]], t16(0));

			movzx(t32(0), word_ptr[&word1[3]]);
			movzx(t32(1), (OperandMEM16)(m64 + 6));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr[&word1[3]], t16(0));

			movq(mm, qword_ptr[word1]);

			free((OperandREF)(intptr_t)0);
			free((OperandREF)1);
			return 0;
		}

		return Optimizer::pavgw(mm, m64);
	}

	Encoding* Emulator::pavgw(OperandMMREG mm, OperandMM64 r_m64)
	{
		if (r_m64.type == Operand::OPERAND_MMREG) return pavgw(mm, (OperandMMREG)r_m64);
		else                                     return pavgw(mm, (OperandMEM64)r_m64);
	}

	Encoding* Emulator::pextrw(OperandREG32 r32, OperandMMREG mm, unsigned char c)
	{
		if (emulateSSE)
		{
			static short word[4];

			movq(qword_ptr[word], mm);
			xor (r32, r32);
			mov((OperandREG16)r32, word_ptr[&word[c & 0x03]]);

			return 0;
		}

		return Optimizer::pextrw(r32, mm, c);
	}

	Encoding* Emulator::pinsrw(OperandMMREG mm, OperandREG16 r16, unsigned char c)
	{
		if (emulateSSE)
		{
			static short word[4];

			movq(qword_ptr[word], mm);
			mov(word_ptr[&word[c & 0x03]], r16);
			movq(mm, qword_ptr[word]);

			return 0;
		}

		return Optimizer::pinsrw(mm, r16, c);
	}

	Encoding* Emulator::pinsrw(OperandMMREG mm, OperandMEM16 m16, unsigned char c)
	{
		if (emulateSSE)
		{
			static short word[4];

			movq(qword_ptr[word], mm);
			mov(t16(0), m16);
			mov(word_ptr[&word[c & 0x03]], t16(0));
			movq(mm, qword_ptr[word]);

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::pinsrw(mm, m16, c);
	}

	Encoding* Emulator::pinsrw(OperandMMREG mm, OperandR_M16 r_m16, unsigned char c)
	{
		if (r_m16.type == Operand::OPERAND_REG16) return pinsrw(mm, (OperandREG16)r_m16, c);
		else                                     return pinsrw(mm, (OperandMEM16)r_m16, c);
	}

	Encoding* Emulator::pmaxsw(OperandMMREG mmi, OperandMMREG mmj)
	{
		if (emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}

		return Optimizer::pmaxsw(mmi, mmj);
	}

	Encoding* Emulator::pmaxsw(OperandMMREG mm, OperandMEM64 m64)
	{
		if (emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}

		return Optimizer::pmaxsw(mm, m64);
	}

	Encoding* Emulator::pmaxsw(OperandMMREG mm, OperandMM64 r_m64)
	{
		if (emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}

		return Optimizer::pmaxsw(mm, r_m64);
	}

	Encoding* Emulator::pmaxub(OperandMMREG mmi, OperandMMREG mmj)
	{
		if (emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}

		return Optimizer::pmaxub(mmi, mmj);
	}

	Encoding* Emulator::pmaxub(OperandMMREG mm, OperandMEM64 m64)
	{
		if (emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}

		return Optimizer::pmaxub(mm, m64);
	}

	Encoding* Emulator::pmaxub(OperandMMREG mm, OperandMM64 r_m64)
	{
		if (emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}

		return Optimizer::pmaxub(mm, r_m64);
	}

	Encoding* Emulator::pminsw(OperandMMREG mmi, OperandMMREG mmj)
	{
		if (emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}

		return Optimizer::pminsw(mmi, mmj);
	}

	Encoding* Emulator::pminsw(OperandMMREG mm, OperandMEM64 m64)
	{
		if (emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}

		return Optimizer::pminsw(mm, m64);
	}

	Encoding* Emulator::pminsw(OperandMMREG mm, OperandMM64 r_m64)
	{
		if (emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}

		return Optimizer::pminsw(mm, r_m64);
	}

	Encoding* Emulator::pminub(OperandMMREG mmi, OperandMMREG mmj)
	{
		if (emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}

		return Optimizer::pminub(mmi, mmj);
	}

	Encoding* Emulator::pminub(OperandMMREG mm, OperandMEM64 m64)
	{
		if (emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}

		return Optimizer::pminub(mm, m64);
	}

	Encoding* Emulator::pminub(OperandMMREG mm, OperandMM64 r_m64)
	{
		if (emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}

		return Optimizer::pminub(mm, r_m64);
	}

	Encoding* Emulator::pmulhuw(OperandMMREG mmi, OperandMMREG mmj)
	{
		if (emulateSSE)
		{
			static short word1[4];
			static short word2[4];

			movq(qword_ptr[word1], mmi);
			movq(qword_ptr[word2], mmj);
			push(eax);
			push(edx);

			mov(ax, word_ptr[&word1[0]]);
			mul(word_ptr[&word2[0]]);
			mov(word_ptr[&word1[0]], dx);

			mov(ax, word_ptr[&word1[1]]);
			mul(word_ptr[&word2[1]]);
			mov(word_ptr[&word1[1]], dx);

			mov(ax, word_ptr[&word1[2]]);
			mul(word_ptr[&word2[2]]);
			mov(word_ptr[&word1[2]], dx);

			mov(ax, word_ptr[&word1[3]]);
			mul(word_ptr[&word2[3]]);
			mov(word_ptr[&word1[3]], dx);

			pop(edx);
			pop(eax);
			movq(mmi, qword_ptr[word1]);

			return 0;
		}

		return Optimizer::pmulhuw(mmi, mmj);
	}

	Encoding* Emulator::pmulhuw(OperandMMREG mm, OperandMEM64 m64)
	{
		if (emulateSSE)
		{
			static short word1[4];
			static short word2[4];

			movq(qword_ptr[word1], mm);
			movq(mm, m64);
			movq(qword_ptr[word2], mm);
			push(eax);
			push(edx);

			mov(ax, word_ptr[&word1[0]]);
			mul(word_ptr[&word2[0]]);
			mov(word_ptr[&word1[0]], dx);

			mov(ax, word_ptr[&word1[1]]);
			mul(word_ptr[&word2[1]]);
			mov(word_ptr[&word1[1]], dx);

			mov(ax, word_ptr[&word1[2]]);
			mul(word_ptr[&word2[2]]);
			mov(word_ptr[&word1[2]], dx);

			mov(ax, word_ptr[&word1[3]]);
			mul(word_ptr[&word2[3]]);
			mov(word_ptr[&word1[3]], dx);

			pop(edx);
			pop(eax);
			movq(mm, qword_ptr[word1]);

			return 0;
		}

		return Optimizer::pmulhuw(mm, m64);
	}

	Encoding* Emulator::pmulhuw(OperandMMREG mm, OperandMM64 r_m64)
	{
		if (r_m64.type == Operand::OPERAND_MMREG) return pmulhuw(mm, (OperandMMREG)r_m64);
		else                                     return pmulhuw(mm, (OperandMEM64)r_m64);
	}

	Encoding* Emulator::prefetchnta(OperandMEM mem)
	{
		if (emulateSSE)
		{
			return 0;
		}

		return Optimizer::prefetchnta((OperandMEM8&)mem);
	}

	Encoding* Emulator::prefetcht0(OperandMEM mem)
	{
		if (emulateSSE)
		{
			return 0;
		}

		return Optimizer::prefetcht0((OperandMEM8&)mem);
	}

	Encoding* Emulator::prefetcht1(OperandMEM mem)
	{
		if (emulateSSE)
		{
			return 0;
		}

		return Optimizer::prefetcht1((OperandMEM8&)mem);
	}

	Encoding* Emulator::prefetcht2(OperandMEM mem)
	{
		if (emulateSSE)
		{
			return 0;
		}

		return Optimizer::prefetcht2((OperandMEM8&)mem);
	}

	Encoding* Emulator::pshufw(OperandMMREG mmi, OperandMMREG mmj, unsigned char c)
	{
		if (emulateSSE)
		{
			static short word1[4];
			static short word2[4];

			movq(qword_ptr[word1], mmj);

			mov(t16(0), word_ptr[&word1[(c >> 0) & 0x03]]);
			mov(word_ptr[&word2[0]], t16(0));

			mov(t16(0), word_ptr[&word1[(c >> 2) & 0x03]]);
			mov(word_ptr[&word2[1]], t16(0));

			mov(t16(0), word_ptr[&word1[(c >> 4) & 0x03]]);
			mov(word_ptr[&word2[2]], t16(0));

			mov(t16(0), word_ptr[&word1[(c >> 6) & 0x03]]);
			mov(word_ptr[&word2[3]], t16(0));

			movq(mmi, qword_ptr[word2]);

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::pshufw(mmi, mmj, c);
	}

	Encoding* Emulator::pshufw(OperandMMREG mm, OperandMEM64 m64, unsigned char c)
	{
		if (emulateSSE)
		{
			static short word[4];

			mov(t16(0), (OperandMEM16)(m64 + ((c >> 0) & 0x03) * 2));
			mov(word_ptr[&word[0]], t16(0));

			mov(t16(0), (OperandMEM16)(m64 + ((c >> 2) & 0x03) * 2));
			mov(word_ptr[&word[1]], t16(0));

			mov(t16(0), (OperandMEM16)(m64 + ((c >> 4) & 0x03) * 2));
			mov(word_ptr[&word[2]], t16(0));

			mov(t16(0), (OperandMEM16)(m64 + ((c >> 6) & 0x03) * 2));
			mov(word_ptr[&word[3]], t16(0));

			movq(mm, qword_ptr[word]);

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::pshufw(mm, m64, c);
	}

	Encoding* Emulator::pshufw(OperandMMREG mm, OperandMM64 r_m64, unsigned char c)
	{
		if (r_m64.type == Operand::OPERAND_MMREG) return pshufw(mm, (OperandMMREG)r_m64, c);
		else                                     return pshufw(mm, (OperandMEM64)r_m64, c);
	}

	Encoding* Emulator::rcpps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			static float one = 1.0f;
			fld(dword_ptr[&one]);
			fdiv(dword_ptr[&sse[j][0]]);
			fstp(dword_ptr[&sse[i][0]]);
			fld(dword_ptr[&one]);
			fdiv(dword_ptr[&sse[j][1]]);
			fstp(dword_ptr[&sse[i][1]]);
			fld(dword_ptr[&one]);
			fdiv(dword_ptr[&sse[j][2]]);
			fstp(dword_ptr[&sse[i][2]]);
			fld(dword_ptr[&one]);
			fdiv(dword_ptr[&sse[j][3]]);
			fstp(dword_ptr[&sse[i][3]]);
			return 0;
		}

		return Optimizer::rcpps(xmmi, xmmj);
	}

	Encoding* Emulator::rcpps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static float one = 1.0f;
			fld(dword_ptr[&one]);
			fdiv((OperandMEM32)(mem128 + 0));
			fstp(dword_ptr[&sse[i][0]]);
			fld(dword_ptr[&one]);
			fdiv((OperandMEM32)(mem128 + 4));
			fstp(dword_ptr[&sse[i][1]]);
			fld(dword_ptr[&one]);
			fdiv((OperandMEM32)(mem128 + 8));
			fstp(dword_ptr[&sse[i][2]]);
			fld(dword_ptr[&one]);
			fdiv((OperandMEM32)(mem128 + 12));
			fstp(dword_ptr[&sse[i][3]]);
			return 0;
		}

		return Optimizer::rcpps(xmm, mem128);
	}

	Encoding* Emulator::rcpps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return rcpps(xmm, (OperandXMMREG)r_m128);
		else                                       return rcpps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::rcpss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			static float one = 1.0f;
			fld(dword_ptr[&one]);
			fdiv(dword_ptr[&sse[j][0]]);
			fstp(dword_ptr[&sse[i][0]]);
			return 0;
		}

		return Optimizer::rcpss(xmmi, xmmj);
	}

	Encoding* Emulator::rcpss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static float one = 1.0f;
			fld(dword_ptr[&one]);
			fdiv((OperandMEM32)mem32);
			fstp(dword_ptr[&sse[i][0]]);
			return 0;
		}

		return Optimizer::rcpss(xmm, mem32);
	}

	Encoding* Emulator::rcpss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return rcpss(xmm, (OperandXMMREG)xmm32);
		else                                      return rcpss(xmm, (OperandMEM32)xmm32);
	}

	Encoding* Emulator::rsqrtps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			static float one = 1.0f;
			fld(dword_ptr[&one]);
			fdiv(dword_ptr[&sse[j][0]]);
			fsqrt();
			fstp(dword_ptr[&sse[i][0]]);
			fld(dword_ptr[&one]);
			fdiv(dword_ptr[&sse[j][1]]);
			fsqrt();
			fstp(dword_ptr[&sse[i][1]]);
			fld(dword_ptr[&one]);
			fdiv(dword_ptr[&sse[j][2]]);
			fsqrt();
			fstp(dword_ptr[&sse[i][2]]);
			fld(dword_ptr[&one]);
			fdiv(dword_ptr[&sse[j][3]]);
			fsqrt();
			fstp(dword_ptr[&sse[i][3]]);
			return 0;
		}

		return Optimizer::rsqrtps(xmmi, xmmj);
	}

	Encoding* Emulator::rsqrtps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static float one = 1.0f;
			fld(dword_ptr[&one]);
			fdiv((OperandMEM32)(mem128 + 0));
			fsqrt();
			fstp(dword_ptr[&sse[i][0]]);
			fld(dword_ptr[&one]);
			fdiv((OperandMEM32)(mem128 + 4));
			fsqrt();
			fstp(dword_ptr[&sse[i][1]]);
			fld(dword_ptr[&one]);
			fdiv((OperandMEM32)(mem128 + 8));
			fsqrt();
			fstp(dword_ptr[&sse[i][2]]);
			fld(dword_ptr[&one]);
			fdiv((OperandMEM32)(mem128 + 12));
			fsqrt();
			fstp(dword_ptr[&sse[i][3]]);
			return 0;
		}

		return Optimizer::rsqrtps(xmm, mem128);
	}

	Encoding* Emulator::rsqrtps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return rsqrtps(xmm, (OperandXMMREG)r_m128);
		else                                       return rsqrtps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::rsqrtss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			static float one = 1.0f;
			fld(dword_ptr[&one]);
			fdiv(dword_ptr[&sse[j][0]]);
			fsqrt();
			fstp(dword_ptr[&sse[i][0]]);
			return 0;
		}

		return Optimizer::rsqrtss(xmmi, xmmj);
	}

	Encoding* Emulator::rsqrtss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static float one = 1.0f;
			fld(dword_ptr[&one]);
			fdiv((OperandMEM32)mem32);
			fsqrt();
			fstp(dword_ptr[&sse[i][0]]);
			return 0;
		}

		return Optimizer::rsqrtss(xmm, mem32);
	}

	Encoding* Emulator::rsqrtss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return rsqrtss(xmm, (OperandXMMREG)xmm32);
		else                                      return rsqrtss(xmm, (OperandMEM32)xmm32);
	}

	Encoding* Emulator::sfence()
	{
		if (emulateSSE)
		{
			return 0;
		}

		return Optimizer::sfence();
	}

	Encoding* Emulator::shufps(OperandXMMREG xmmi, OperandXMMREG xmmj, unsigned char c)
	{
		if (emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr[&sse[i][(c >> 0) & 0x03]]);
			mov(dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr[&sse[i][(c >> 2) & 0x03]]);
			mov(dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][(c >> 4) & 0x03]]);
			mov(dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][(c >> 6) & 0x03]]);
			mov(dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::shufps(xmmi, xmmj, c);
	}

	Encoding* Emulator::shufps(OperandXMMREG xmm, OperandMEM128 m128, unsigned char c)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr[&sse[i][(c >> 0) & 0x03]]);
			mov(dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr[&sse[i][(c >> 2) & 0x03]]);
			mov(dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(m128 + ((c >> 4) & 0x03) * 4));
			mov(dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(m128 + ((c >> 6) & 0x03) * 4));
			mov(dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::shufps(xmm, m128, c);
	}

	Encoding* Emulator::shufps(OperandXMMREG xmm, OperandR_M128 r_m128, unsigned char c)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return shufps(xmm, (OperandXMMREG)r_m128, c);
		else                                       return shufps(xmm, (OperandMEM128)r_m128, c);
	}

	Encoding* Emulator::sqrtps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr[&sse[j][0]]);
			fsqrt();
			fstp(dword_ptr[&sse[i][0]]);
			fld(dword_ptr[&sse[j][1]]);
			fsqrt();
			fstp(dword_ptr[&sse[i][1]]);
			fld(dword_ptr[&sse[j][2]]);
			fsqrt();
			fstp(dword_ptr[&sse[i][2]]);
			fld(dword_ptr[&sse[j][3]]);
			fsqrt();
			fstp(dword_ptr[&sse[i][3]]);
			return 0;
		}

		return Optimizer::sqrtps(xmmi, xmmj);
	}

	Encoding* Emulator::sqrtps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld((OperandMEM32)(mem128 + 0));
			fsqrt();
			fstp(dword_ptr[&sse[i][0]]);
			fld((OperandMEM32)(mem128 + 4));
			fsqrt();
			fstp(dword_ptr[&sse[i][1]]);
			fld((OperandMEM32)(mem128 + 8));
			fsqrt();
			fstp(dword_ptr[&sse[i][2]]);
			fld((OperandMEM32)(mem128 + 12));
			fsqrt();
			fstp(dword_ptr[&sse[i][3]]);
			return 0;
		}

		return Optimizer::sqrtps(xmm, mem128);
	}

	Encoding* Emulator::sqrtps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return sqrtps(xmm, (OperandXMMREG)r_m128);
		else                                       return sqrtps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::sqrtss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr[&sse[j][0]]);
			fsqrt();
			fstp(dword_ptr[&sse[i][0]]);
			return 0;
		}

		return Optimizer::sqrtss(xmmi, xmmj);
	}

	Encoding* Emulator::sqrtss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static float one = 1.0f;
			fld(mem32);
			fsqrt();
			fstp(dword_ptr[&sse[i][0]]);
			return 0;
		}

		return Optimizer::sqrtss(xmm, mem32);
	}

	Encoding* Emulator::sqrtss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return sqrtss(xmm, (OperandXMMREG)xmm32);
		else                                      return sqrtss(xmm, (OperandMEM32)xmm32);
	}

	Encoding* Emulator::stmxcsr(OperandMEM32 m32)
	{
		if (emulateSSE)
		{
			return 0;
		}

		return Optimizer::stmxcsr(m32);
	}

	Encoding* Emulator::subps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr[&sse[i][0]]);
			fsub(dword_ptr[&sse[j][0]]);
			fstp(dword_ptr[&sse[i][0]]);
			fld(dword_ptr[&sse[i][1]]);
			fsub(dword_ptr[&sse[j][1]]);
			fstp(dword_ptr[&sse[i][1]]);
			fld(dword_ptr[&sse[i][2]]);
			fsub(dword_ptr[&sse[j][2]]);
			fstp(dword_ptr[&sse[i][2]]);
			fld(dword_ptr[&sse[i][3]]);
			fsub(dword_ptr[&sse[j][3]]);
			fstp(dword_ptr[&sse[i][3]]);
			return 0;
		}

		return Optimizer::subps(xmmi, xmmj);
	}

	Encoding* Emulator::subps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld(dword_ptr[&sse[i][0]]);
			fsub((OperandMEM32)(mem128 + 0));
			fstp(dword_ptr[&sse[i][0]]);
			fld(dword_ptr[&sse[i][1]]);
			fsub((OperandMEM32)(mem128 + 4));
			fstp(dword_ptr[&sse[i][1]]);
			fld(dword_ptr[&sse[i][2]]);
			fsub((OperandMEM32)(mem128 + 8));
			fstp(dword_ptr[&sse[i][2]]);
			fld(dword_ptr[&sse[i][3]]);
			fsub((OperandMEM32)(mem128 + 12));
			fstp(dword_ptr[&sse[i][3]]);
			return 0;
		}

		return Optimizer::subps(xmm, mem128);
	}

	Encoding* Emulator::subps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return subps(xmm, (OperandXMMREG)r_m128);
		else                                       return subps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::subss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr[&sse[i][0]]);
			fsub(dword_ptr[&sse[j][0]]);
			fstp(dword_ptr[&sse[i][0]]);
			return 0;
		}

		return Optimizer::subss(xmmi, xmmj);
	}

	Encoding* Emulator::subss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld(dword_ptr[&sse[i][0]]);
			fsub((OperandMEM32)mem32);
			fstp(dword_ptr[&sse[i][0]]);
			return 0;
		}

		return Optimizer::subss(xmm, mem32);
	}

	Encoding* Emulator::subss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return subss(xmm, (OperandXMMREG)xmm32);
		else                                      return subss(xmm, (OperandMEM32)xmm32);
	}

	Encoding* Emulator::ucomiss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			fld(dword_ptr[&sse[j][0]]);
			fld(dword_ptr[&sse[i][0]]);
			fcomip(st0, st1);
			ffree(st0);

			return 0;
		}

		return Optimizer::ucomiss(xmmi, xmmj);
	}

	Encoding* Emulator::ucomiss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if (emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fld(mem32);
			fld(dword_ptr[&sse[i][0]]);
			fcomip(st0, st1);
			ffree(st0);

			return 0;
		}

		return Optimizer::ucomiss(xmm, mem32);
	}

	Encoding* Emulator::ucomiss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if (xmm32.type == Operand::OPERAND_XMMREG) return ucomiss(xmm, (OperandXMMREG)xmm32);
		else                                      return ucomiss(xmm, (OperandMEM32)xmm32);
	}

	Encoding* Emulator::unpckhps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr[&sse[i][2]]);
			mov(dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr[&sse[i][3]]);
			mov(dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][2]]);
			mov(dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][3]]);
			mov(dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::unpckhps(xmmi, xmmj);
	}

	Encoding* Emulator::unpckhps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr[&sse[i][2]]);
			mov(dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr[&sse[i][3]]);
			mov(dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 8));
			mov(dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 12));
			mov(dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::unpckhps(xmm, mem128);
	}

	Encoding* Emulator::unpckhps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return unpckhps(xmm, (OperandXMMREG)r_m128);
		else                                       return unpckhps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::unpcklps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr[&sse[i][0]]);
			mov(dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr[&sse[i][1]]);
			mov(dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][0]]);
			mov(dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][1]]);
			mov(dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::unpcklps(xmmi, xmmj);
	}

	Encoding* Emulator::unpcklps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr[&sse[i][0]]);
			mov(dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr[&sse[i][1]]);
			mov(dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 0));
			mov(dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 4));
			mov(dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::unpcklps(xmm, mem128);
	}

	Encoding* Emulator::unpcklps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return unpcklps(xmm, (OperandXMMREG)r_m128);
		else                                       return unpcklps(xmm, (OperandMEM128)r_m128);
	}

	Encoding* Emulator::xorps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if (emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr[&sse[j][0]]);
			xor (dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][1]]);
			xor (dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][2]]);
			xor (dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr[&sse[j][3]]);
			xor (dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::xorps(xmmi, xmmj);
	}

	Encoding* Emulator::xorps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if (emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(mem128 + 0));
			xor (dword_ptr[&sse[i][0]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 4));
			xor (dword_ptr[&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 8));
			xor (dword_ptr[&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128 + 12));
			xor (dword_ptr[&sse[i][3]], t32(0));

			free((OperandREF)(intptr_t)0);
			return 0;
		}

		return Optimizer::xorps(xmm, mem128);
	}

	Encoding* Emulator::xorps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if (r_m128.type == Operand::OPERAND_XMMREG) return xorps(xmm, (OperandXMMREG)r_m128);
		else                                       return xorps(xmm, (OperandMEM128)r_m128);
	}

	void Emulator::enableEmulateSSE()
	{
		emulateSSE = true;
	}

	void Emulator::disableEmulateSSE()
	{
		emulateSSE = false;
	}

	void Emulator::dumpSSE()
	{
		pushad();
		emms();

		static float sse[8][4];

		movups(xword_ptr[sse[0]], xmm0);
		movups(xword_ptr[sse[1]], xmm1);
		movups(xword_ptr[sse[2]], xmm2);
		movups(xword_ptr[sse[3]], xmm3);
		movups(xword_ptr[sse[4]], xmm4);
		movups(xword_ptr[sse[5]], xmm5);
		movups(xword_ptr[sse[6]], xmm6);
		movups(xword_ptr[sse[7]], xmm7);

		static FILE* file;
		static char* perm = "a";
		static char* name;

		if (emulateSSE)
		{
			name = "dumpEmulate.txt";
		}
		else
		{
			name = "dumpNative.txt";
		}

		mov(eax, dword_ptr[&perm]);
		push(eax);
		mov(ecx, dword_ptr[&name]);
		push(ecx);
		call((intptr_t)fopen);
		add(esp, 8);
		mov(dword_ptr[&file], eax);

		static char* string0 = "xmm0: %f, %f, %f, %f\n";
		static char* string1 = "xmm1: %f, %f, %f, %f\n";
		static char* string2 = "xmm2: %f, %f, %f, %f\n";
		static char* string3 = "xmm3: %f, %f, %f, %f\n";
		static char* string4 = "xmm4: %f, %f, %f, %f\n";
		static char* string5 = "xmm5: %f, %f, %f, %f\n";
		static char* string6 = "xmm6: %f, %f, %f, %f\n";
		static char* string7 = "xmm7: %f, %f, %f, %f\n";
		static char* newline = "\n";

		// fprintf(file, string0, sse[0][0], sse[0][1], sse[0][2], sse[0][3]);
		fld(dword_ptr[&sse[0][3]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[0][2]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[0][1]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[0][0]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		mov(eax, dword_ptr[&string0]);
		push(eax);
		mov(ecx, dword_ptr[&file]);
		push(ecx);
		call((intptr_t)fprintf);
		add(esp, 0x28);

		// fprintf(file, string1, sse[1][0], sse[1][1], sse[1][2], sse[1][3]);
		fld(dword_ptr[&sse[1][3]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[1][2]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[1][1]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[1][0]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		mov(eax, dword_ptr[&string1]);
		push(eax);
		mov(ecx, dword_ptr[&file]);
		push(ecx);
		call((intptr_t)fprintf);
		add(esp, 0x28);

		// fprintf(file, string2, sse[2][0], sse[2][1], sse[2][2], sse[2][3]);
		fld(dword_ptr[&sse[2][3]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[2][2]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[2][1]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[2][0]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		mov(eax, dword_ptr[&string2]);
		push(eax);
		mov(ecx, dword_ptr[&file]);
		push(ecx);
		call((intptr_t)fprintf);
		add(esp, 0x28);

		// fprintf(file, string3, sse[3][0], sse[3][1], sse[3][2], sse[3][3]);
		fld(dword_ptr[&sse[3][3]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[3][2]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[3][1]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[3][0]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		mov(eax, dword_ptr[&string3]);
		push(eax);
		mov(ecx, dword_ptr[&file]);
		push(ecx);
		call((intptr_t)fprintf);
		add(esp, 0x28);

		// fprintf(file, string4, sse[4][0], sse[4][1], sse[4][2], sse[4][3]);
		fld(dword_ptr[&sse[4][3]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[4][2]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[4][1]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[4][0]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		mov(eax, dword_ptr[&string4]);
		push(eax);
		mov(ecx, dword_ptr[&file]);
		push(ecx);
		call((intptr_t)fprintf);
		add(esp, 0x28);

		// fprintf(file, string5, sse[5][0], sse[5][1], sse[5][2], sse[5][3]);
		fld(dword_ptr[&sse[5][3]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[5][2]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[5][1]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[5][0]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		mov(eax, dword_ptr[&string5]);
		push(eax);
		mov(ecx, dword_ptr[&file]);
		push(ecx);
		call((intptr_t)fprintf);
		add(esp, 0x28);

		// fprintf(file, string6, sse[6][0], sse[6][1], sse[6][2], sse[6][3]);
		fld(dword_ptr[&sse[6][3]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[6][2]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[6][1]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[6][0]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		mov(eax, dword_ptr[&string6]);
		push(eax);
		mov(ecx, dword_ptr[&file]);
		push(ecx);
		call((intptr_t)fprintf);
		add(esp, 0x28);

		// fprintf(file, string7, sse[7][0], sse[7][1], sse[7][2], sse[7][3]);
		fld(dword_ptr[&sse[7][3]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[7][2]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[7][1]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		fld(dword_ptr[&sse[7][0]]);
		sub(esp, 8);
		fstp(qword_ptr[esp]);
		mov(eax, dword_ptr[&string7]);
		push(eax);
		mov(ecx, dword_ptr[&file]);
		push(ecx);
		call((intptr_t)fprintf);
		add(esp, 0x28);

		// fprintf(file, newline);
		mov(eax, dword_ptr[&newline]);
		push(eax);
		mov(ecx, dword_ptr[&file]);
		push(ecx);
		call((intptr_t)fprintf);
		add(esp, 8);

		// fclose(file);
		mov(eax, dword_ptr[&file]);
		push(eax);
		call((intptr_t)fclose);
		add(esp, 4);

		popad();

		//	int3();
	}
}

namespace SoftWire
{
	Encoding::Encoding(const Instruction* instruction)
	{
		this->instruction = instruction;

		label = 0;
		reference = 0;
		reset();

		O1 = 0xCC;   // Breakpoint
		format.O1 = true;

		emit = true;
	}

	Encoding::Encoding(const Encoding& encoding)
	{
		*this = encoding;
	}

	Encoding::~Encoding()
	{
		delete[] label;
		delete[] reference;
	}

	Encoding& Encoding::operator=(const Encoding& encoding)
	{
		instruction = encoding.instruction;

		if (encoding.label) label = strdup(encoding.label);
		else label = 0;

		if (encoding.reference) reference = strdup(encoding.reference);
		else reference = 0;

		relative = encoding.relative;

		format = encoding.format;

		P1 = encoding.P1;
		P2 = encoding.P2;
		P3 = encoding.P3;
		P4 = encoding.P4;
		REX.b = encoding.REX.b;
		O4 = encoding.O4;
		O3 = encoding.O3;
		O2 = encoding.O2;
		O1 = encoding.O1;
		modRM.b = encoding.modRM.b;
		SIB.b = encoding.SIB.b;

		immediate = encoding.immediate;
		displacement = encoding.displacement;

		emit = encoding.emit;

		return *this;
	}

	void Encoding::reset()
	{
		delete[] label;
		delete[] reference;

		label = 0;
		reference = 0;
		relative = false;

		format.P1 = false;
		format.P2 = false;
		format.P3 = false;
		format.P4 = false;
		format.REX = false;
		format.O4 = false;
		format.O3 = false;
		format.O2 = false;
		format.O1 = false;
		format.modRM = false;
		format.SIB = false;
		format.D1 = false;
		format.D2 = false;
		format.D3 = false;
		format.D4 = false;
		format.I1 = false;
		format.I2 = false;
		format.I3 = false;
		format.I4 = false;

		P1 = 0xCC;
		P2 = 0xCC;
		P3 = 0xCC;
		P4 = 0xCC;
		REX.b = 0xCC;
		O4 = 0xCC;
		O3 = 0xCC;
		O2 = 0xCC;
		O1 = 0xCC;
		modRM.b = 0xCC;
		SIB.b = 0xCC;
		D1 = 0xCC;
		D2 = 0xCC;
		D3 = 0xCC;
		D4 = 0xCC;
		I1 = 0xCC;
		I2 = 0xCC;
		I3 = 0xCC;
		I4 = 0xCC;

		address = 0;

		emit = true;
	}

	const char* Encoding::getLabel() const
	{
		return label;
	}

	const char* Encoding::getReference() const
	{
		if (P1 != 0xF1)
		{
			return reference;
		}
		else
		{
			return 0;
		}
	}

	const char* Encoding::getLiteral() const
	{
		if (!format.O1)
		{
			return literal;
		}
		else
		{
			return 0;
		}
	}

	int Encoding::getImmediate() const
	{
		return immediate;
	}

	__int64 Encoding::getDisplacement() const
	{
		return displacement;
	}

	int Encoding::length(const unsigned char* output) const
	{
		return writeCode((unsigned char*)output, false);
	}

	void Encoding::addPrefix(unsigned char p)
	{
		if (!format.P1)
		{
			P1 = p;
			format.P1 = true;
		}
		else if (!format.P2)
		{
			P2 = p;
			format.P2 = true;
		}
		else if (!format.P3)
		{
			P3 = p;
			format.P3 = true;
		}
		else if (!format.P4)
		{
			P4 = p;
			format.P4 = true;
		}
		else
		{
			throw Error("Too many prefixes in opcode");
		}
	}

	void Encoding::setImmediate(int immediate)
	{
		this->immediate = immediate;
	}

	void Encoding::setDisplacement(__int64 displacement)
	{
		this->displacement = displacement;
	}

	void Encoding::addDisplacement(__int64 displacement)
	{
		this->displacement += displacement;
	}

	void Encoding::setJumpOffset(int offset)
	{
		if ((char)offset != offset && !format.I2)
		{
			throw Error("Jump offset range too big");
		}

		this->immediate = offset;
	}

	void Encoding::setCallOffset(int offset)
	{
		if (!(format.I1 && format.I2 && format.I3 && format.I4))
		{
			throw Error("Call offset should be 32-bit");
		}

		this->immediate = offset;
	}

	void Encoding::setLabel(const char* label)
	{
		if (label != 0)
		{
			this->label = strdup(label);
		}
	}

	void Encoding::setReference(const char* label)
	{
		if (label != 0)
		{
			reference = strdup(label);
		}
	}

	bool Encoding::relativeReference() const
	{
		return relative;
	}

	bool Encoding::absoluteReference() const
	{
		return !relativeReference();
	}

	bool Encoding::hasDisplacement() const
	{
		return format.D1 || format.D2 || format.D3 || format.D4;
	}

	bool Encoding::hasImmediate() const
	{
		return format.I1 || format.I2 || format.I3 || format.I4;
	}

	bool Encoding::isRipRelative() const
	{
		return modRM.mod == 0 && modRM.r_m == 5;
	}

	void Encoding::setAddress(const unsigned char* address)
	{
		this->address = address;
	}

	const unsigned char* Encoding::getAddress() const
	{
		return address;
	}

	Encoding* Encoding::reserve()
	{
		emit = false;

		return this;
	}

	void Encoding::retain()
	{
		emit = true;
	}

	bool Encoding::isEmitting()
	{
		return emit;
	}

	int Encoding::writeCode(unsigned char* buffer, bool write) const
	{
		// Eliminated instruction
		if (!emit) return 0;

		unsigned char* start = buffer;

#define OUTPUT_BYTE(x) if(write) {*buffer++ = (x);} else {buffer++;}

		if (P1 == 0xF1)   // Special 'instructions', indicated by INT01 prefix byte
		{
			if (O1 == 0x00 && literal)   // String constant
			{
				for (int i = 0; literal[i]; i++)
				{
					OUTPUT_BYTE(literal[i]);
				}

				OUTPUT_BYTE('\0');
			}
			else if (O1 == 0x90 && immediate)   // ALIGN
			{
				if (immediate > 256)
				{
					throw Error("ALIGN value too big: %d bytes", immediate);
				}

				buffer += align(buffer, immediate, write);
			}
			else if ((O1 == 0x01 || O1 == 0x02 || O1 == 0x04) && displacement)   // Array
			{
				for (int i = 0; i < O1 * displacement; i++)
				{
					OUTPUT_BYTE(0xCC);   // INT3
				}
			}
			else   // Constant
			{
				if (format.I1)		OUTPUT_BYTE(I1);
				if (format.I2)		OUTPUT_BYTE(I2);
				if (format.I3)		OUTPUT_BYTE(I3);
				if (format.I4)		OUTPUT_BYTE(I4);
			}
		}
		else   // Normal instructions
		{
			if (format.P1)		OUTPUT_BYTE(P1);
			if (format.P2)		OUTPUT_BYTE(P2);
			if (format.P3)		OUTPUT_BYTE(P3);
			if (format.P4)		OUTPUT_BYTE(P4);
			if (format.REX)		OUTPUT_BYTE(REX.b);
			if (format.O4)		OUTPUT_BYTE(O4);
			if (format.O3)		OUTPUT_BYTE(O3);
			if (format.O2)		OUTPUT_BYTE(O2);
			if (format.O1)		OUTPUT_BYTE(O1);
			if (format.modRM)	OUTPUT_BYTE(modRM.b);
			if (format.SIB)		OUTPUT_BYTE(SIB.b);
			if (format.D1)		OUTPUT_BYTE(D1);
			if (format.D2)		OUTPUT_BYTE(D2);
			if (format.D3)		OUTPUT_BYTE(D3);
			if (format.D4)		OUTPUT_BYTE(D4);
			if (format.I1)		OUTPUT_BYTE(I1);
			if (format.I2)		OUTPUT_BYTE(I2);
			if (format.I3)		OUTPUT_BYTE(I3);
			if (format.I4)		OUTPUT_BYTE(I4);
		}

#undef OUTPUT_BYTE

		return (int)(buffer - start);
	}

	int Encoding::align(unsigned char* buffer, int alignment, bool write)
	{
		if (alignment > 64) throw Error("Alignment greater than 64");

		int padding = (int)(alignment - (__int64)buffer % alignment);

		if (padding == alignment)
		{
			padding = 0;
		}

		if (write)
		{
			int i = 0;

			while (i + 3 <= padding)
			{
				// 3-byte NOP
				*buffer++ = 0x66;
				*buffer++ = 0x66;
				*buffer++ = 0x90;

				i += 3;
			}

			if (i + 2 <= padding)
			{
				// 2-byte NOP
				*buffer++ = 0x66;
				*buffer++ = 0x90;

				i += 2;
			}

			if (i + 1 <= padding)
			{
				// 1-byte NOP
				*buffer++ = 0x90;
			}
		}

		return padding;
	}

	int Encoding::printCode(char* buffer) const
	{
		// Eliminated instruction
		if (!emit) return 0;

		char* start = buffer;

		if (format.P1) { sprintf(buffer, "%.2X ", P1);		buffer += 3; }
		if (format.P2) { sprintf(buffer, "%.2X ", P2);		buffer += 3; }
		if (format.P3) { sprintf(buffer, "%.2X ", P3);		buffer += 3; }
		if (format.P4) { sprintf(buffer, "%.2X ", P4);		buffer += 3; }
		if (format.REX) { sprintf(buffer, "%.2X ", REX.B);		buffer += 3; }
		if (format.O4) { sprintf(buffer, "%.2X ", O4);		buffer += 3; }
		if (format.O3) { sprintf(buffer, "%.2X ", O3);		buffer += 3; }
		if (format.O2) { sprintf(buffer, "%.2X ", O2);		buffer += 3; }
		if (format.O1) { sprintf(buffer, "%.2X ", O1);		buffer += 3; }
		if (format.modRM) { sprintf(buffer, "%.2X ", modRM.b);	buffer += 3; }
		if (format.SIB) { sprintf(buffer, "%.2X ", SIB.b);	buffer += 3; }
		if (format.D1) { sprintf(buffer, "%.2X ", D1);		buffer += 3; }
		if (format.D2) { sprintf(buffer, "%.2X ", D2);		buffer += 3; }
		if (format.D3) { sprintf(buffer, "%.2X ", D3);		buffer += 3; }
		if (format.D4) { sprintf(buffer, "%.2X ", D4);		buffer += 3; }
		if (format.I1) { sprintf(buffer, "%.2X ", I1);		buffer += 3; }
		if (format.I2) { sprintf(buffer, "%.2X ", I2);		buffer += 3; }
		if (format.I3) { sprintf(buffer, "%.2X ", I3);		buffer += 3; }
		if (format.I4) { sprintf(buffer, "%.2X ", I4);		buffer += 3; }

		sprintf(buffer++, "\n");

		return (int)(buffer - start);
	}
}


namespace SoftWire
{
	Error::Error(const char* format, ...)
	{
		string[0] = '\0';

		va_list argList;
		va_start(argList, format);
		vsnprintf(string, 255, format, argList);
		va_end(argList);
	}

	const char* Error::getString() const
	{
		if (string[0] == '\0')
		{
			return "<Unknown>";
		}
		else
		{
			return string;
		}
	}

	Error& Error::operator<<(const Error& error)
	{
		snprintf(string, 255, "%s\n%s", string, error.getString());

		return *this;
	}

	Error& Error::operator>>(const Error& error)
	{
		snprintf(string, 255, "%s\n%s", error.getString(), string);

		return *this;
	}
}


namespace SoftWire
{
	Instruction::Instruction()
	{
	}

	Instruction::Instruction(const Syntax* syntax)
	{
		this->syntax = syntax;

		extractOperands(syntax->operands);

		if (secondOperand == Operand::OPERAND_IMM8)
		{
			if (Operand::isSubtypeOf(firstOperand, Operand::OPERAND_R_M16) ||
				Operand::isSubtypeOf(firstOperand, Operand::OPERAND_R_M32))
			{
				secondOperand = Operand::OPERAND_EXT8;
			}
		}
	}

	Instruction::~Instruction()
	{
	}

	Instruction& Instruction::operator=(const Instruction& instruction)
	{
		syntax = instruction.syntax;

		specifier = instruction.specifier;
		firstOperand = instruction.firstOperand;
		secondOperand = instruction.secondOperand;
		thirdOperand = instruction.thirdOperand;

		return *this;
	}

	void Instruction::extractOperands(const char* syntax)
	{
		if (!syntax)
		{
			throw INTERNAL_ERROR;
		}

		specifier = Specifier::TYPE_UNKNOWN;
		firstOperand = Operand::OPERAND_VOID;
		secondOperand = Operand::OPERAND_VOID;
		thirdOperand = Operand::OPERAND_VOID;

		char string[256];
		strncpy(string, syntax, 255);
		const char* token = strtok(string, " ,");

		if (!token)
		{
			return;
		}

		specifier = Specifier::scan(token);

		if (specifier != Specifier::TYPE_UNKNOWN)
		{
			token = strtok(0, " ,");

			if (!token)
			{
				return;
			}
		}

		firstOperand = Operand::scanSyntax(token);

		if (firstOperand != Operand::OPERAND_UNKNOWN)
		{
			token = strtok(0, " ,");

			if (token == 0)
			{
				return;
			}
		}

		secondOperand = Operand::scanSyntax(token);

		if (secondOperand != Operand::OPERAND_UNKNOWN)
		{
			token = strtok(0, " ,");

			if (token == 0)
			{
				return;
			}
		}

		thirdOperand = Operand::scanSyntax(token);

		if (thirdOperand != Operand::OPERAND_UNKNOWN)
		{
			token = strtok(0, " ,");

			if (token == 0)
			{
				return;
			}
		}

		if (token == 0)
		{
			return;
		}
		else
		{
			throw Error("Invalid operand encoding '%s'", syntax);
		}
	}

	const char* Instruction::getMnemonic() const
	{
		return syntax->mnemonic;
	}

	Operand::Type Instruction::getFirstOperand() const
	{
		return firstOperand;
	}

	Operand::Type Instruction::getSecondOperand() const
	{
		return secondOperand;
	}

	Operand::Type Instruction::getThirdOperand() const
	{
		return thirdOperand;
	}

	const char* Instruction::getOperandSyntax() const
	{
		return syntax->operands;
	}

	const char* Instruction::getEncoding() const
	{
		return syntax->encoding;
	}

	bool Instruction::is16Bit() const
	{
		return (syntax->flags & CPU_386) != CPU_386;
	}

	bool Instruction::is32Bit() const
	{
		return (syntax->flags & CPU_386) == CPU_386;
	}

	bool Instruction::is64Bit() const
	{
		return (syntax->flags & CPU_X64) == CPU_X64;
	}

	bool Instruction::isInvalid64() const
	{
		return (syntax->flags & CPU_INVALID64) == CPU_INVALID64;
	}

	int Instruction::approximateSize() const
	{
		const char* format = syntax->encoding;

		if (!format)
		{
			throw INTERNAL_ERROR;
		}

		int size = 0;

		while (*format)
		{
			switch ((format[0] << 8) | format[1])
			{
			case LOCK_PRE:
			case CONST_PRE:
			case REPNE_PRE:
			case REP_PRE:
				size += 1;
				break;
			case OFF_PRE:
				if (!is32Bit())
				{
					size += 1;
				}
				break;
			case ADDR_PRE:
				if (!is32Bit())
				{
					size += 1;
				}
				break;
			case ADD_REG:
				break;
			case EFF_ADDR:
			case MOD_RM_0:
			case MOD_RM_1:
			case MOD_RM_2:
			case MOD_RM_3:
			case MOD_RM_4:
			case MOD_RM_5:
			case MOD_RM_6:
			case MOD_RM_7:
				size += 1;
				break;
			case DWORD_IMM:
			case DWORD_REL:
				size += 4;
				break;
			case WORD_IMM:
				size += 2;
				break;
			case BYTE_IMM:
			case BYTE_REL:
				size += 1;
				break;
			default:
				size += 1;
			}

			format += 2;

			if (*format == ' ')
			{
				format++;
			}
			else if (*format == '\0')
			{
				break;
			}
			else
			{
				throw INTERNAL_ERROR;
			}
		}

		return size;
	}
}


namespace SoftWire
{
	RegisterAllocator::Allocation RegisterAllocator::GPR[16];
	RegisterAllocator::Allocation RegisterAllocator::MMX[16];
	RegisterAllocator::Allocation RegisterAllocator::XMM[16];

	bool RegisterAllocator::autoEMMS = false;
	bool RegisterAllocator::copyPropagation = true;
	bool RegisterAllocator::loadElimination = true;
	bool RegisterAllocator::spillElimination = true;
	bool RegisterAllocator::minimalRestore = true;
	bool RegisterAllocator::dropUnmodified = true;

	RegisterAllocator::RegisterAllocator(bool x64) : Assembler(x64)
	{
		// Completely eraze allocation state
		{for (int i = 0; i < 16; i++)
		{
			GPR[i].free();
			XMM[i].free();
		}}

		{for (int i = 0; i < 8; i++)
		{
			MMX[i].free();
		}}
	}

	RegisterAllocator::~RegisterAllocator()
	{
		// Completely eraze allocation state
		for (int i = 0; i < 8; i++)
		{
			GPR[i].free();
			MMX[i].free();
			XMM[i].free();
		}
	}

	const OperandREG8 RegisterAllocator::r8(const OperandREF& ref, bool copy)
	{
		OperandREG32 reg = r32(ref, copy);

		// Make sure we only have al, cl, dl or bl
		if (reg.reg >= 4)
		{
			spill(reg);

			// Need to spill one of al, cl, dl or bl
			int candidate = 0;
			unsigned int priority = 0xFFFFFFFF;

			for (int i = 0; i < 4; i++)
			{
				if (GPR[i].priority < priority)
				{
					priority = GPR[i].priority;
					candidate = i;
				}
			}

			spill(OperandREG32(candidate));

			return (OperandREG8)allocate32(candidate, ref, copy, 1);
		}

		return (OperandREG8)reg;
	}

	const OperandR_M8 RegisterAllocator::m8(const OperandREF& ref)
	{
		return (OperandR_M8)m32(ref, 1);
	}

	const OperandREG16 RegisterAllocator::r16(const OperandREF& ref, bool copy)
	{
		return (OperandREG16)r32(ref, copy, 2);
	}

	const OperandR_M16 RegisterAllocator::m16(const OperandREF& ref)
	{
		return (OperandR_M16)m32(ref, 2);
	}

	OperandREG32 RegisterAllocator::r32(const OperandREF& ref, bool copy, int partial)
	{
		if (ref == (intptr_t)0 && copy) throw Error("Cannot dereference 0");

		// Check if already allocated
		{for (int i = 0; i < 8; i++)
		{
			if (i == Encoding::ESP || i == Encoding::EBP) continue;

			if (GPR[i].reference == ref)
			{
				return prioritize32(i);
			}
		}}

		// Check spilled but unused registers
		if (spillElimination)
		{
			for (int i = 0; i < 8; i++)
			{
				if (i == Encoding::ESP || i == Encoding::EBP) continue;

				if (GPR[i].priority == 0 && GPR[i].spill.reference == ref)
				{
					if (GPR[i].spillInstruction)
					{
						GPR[i].spillInstruction->reserve();
					}

					GPR[i].reference = GPR[i].spill.reference;
					GPR[i].partial = GPR[i].spill.partial;
					GPR[i].priority = GPR[i].spill.priority;
					GPR[i].copyInstruction = GPR[i].spill.copyInstruction;
					GPR[i].loadInstruction = GPR[i].spill.loadInstruction;
					GPR[i].spillInstruction = GPR[i].spill.spillInstruction;

					GPR[i].spill.free();

					return prioritize32(i);
				}
			}
		}

		// Search for free registers
		{for (int i = 0; i < 8; i++)
		{
			if (i == Encoding::ESP || i == Encoding::EBP) continue;

			if (GPR[i].priority == 0 && GPR[i].spill.priority == 0)
			{
				return allocate32(i, ref, copy, partial);
			}
		}}

		{for (int i = 0; i < 8; i++)
		{
			if (i == Encoding::ESP || i == Encoding::EBP) continue;

			if (GPR[i].priority == 0)
			{
				return allocate32(i, ref, copy, partial);
			}
		}}

		// Need to spill one
		int candidate = 0;
		int betterCandidate = -1;
		unsigned int priority = 0xFFFFFFFF;

		{for (int i = 0; i < 8; i++)
		{
			if (i == Encoding::ESP || i == Encoding::EBP) continue;

			if (GPR[i].priority < priority)
			{
				priority = GPR[i].priority;
				candidate = i;

				if (!GPR[i].modified && GPR[i].priority < 0xFFFFFFFF - 2)
				{
					betterCandidate = i;
				}
			}
		}}

		if (betterCandidate != -1)
		{
			candidate = betterCandidate;
		}

		Encoding* spillInstruction = spill32(candidate);

		GPR[candidate].spill.reference = GPR[candidate].reference;
		GPR[candidate].spill.priority = GPR[candidate].priority;
		GPR[candidate].spill.partial = GPR[candidate].partial;
		GPR[candidate].spill.copyInstruction = GPR[candidate].copyInstruction;
		GPR[candidate].spill.loadInstruction = GPR[candidate].loadInstruction;
		GPR[candidate].spill.spillInstruction = GPR[candidate].spillInstruction;

		GPR[candidate].reference = (intptr_t)0;
		GPR[candidate].priority = 0;
		GPR[candidate].partial = 0;
		GPR[candidate].copyInstruction = 0;
		GPR[candidate].loadInstruction = 0;
		GPR[candidate].spillInstruction = spillInstruction;

		return allocate32(candidate, ref, copy, partial);
	}

	OperandR_M32 RegisterAllocator::m32(const OperandREF& ref, int partial)
	{
		if (ref == (intptr_t)0) throw Error("Cannot dereference 0");

		// Check if already allocated
		for (int i = 0; i < 8; i++)
		{
			if (i == Encoding::ESP || i == Encoding::EBP) continue;

			if (GPR[i].reference == ref)
			{
				return prioritize32(i);
			}
		}

		// Check spilled but unused registers
		if (spillElimination)
		{
			for (int i = 0; i < 8; i++)
			{
				if (i == Encoding::ESP || i == Encoding::EBP) continue;

				if (GPR[i].priority == 0 && GPR[i].spill.reference == ref)
				{
					if (GPR[i].spillInstruction)
					{
						GPR[i].spillInstruction->reserve();
					}

					GPR[i].reference = GPR[i].spill.reference;
					GPR[i].partial = GPR[i].spill.partial;
					GPR[i].priority = GPR[i].spill.priority;
					GPR[i].copyInstruction = GPR[i].spill.copyInstruction;
					GPR[i].loadInstruction = GPR[i].spill.loadInstruction;
					GPR[i].spillInstruction = GPR[i].spill.spillInstruction;

					GPR[i].spill.free();

					return prioritize32(i);
				}
			}
		}

		return (OperandR_M32)dword_ptr[ref];
	}

	OperandREG32 RegisterAllocator::allocate32(int i, const OperandREF& ref, bool copy, int partial)
	{
		GPR[i].reference = ref;
		GPR[i].partial = partial;

		prioritize32(i);

		Encoding* loadInstruction = 0;
		Encoding* spillInstruction = GPR[i].spillInstruction;
		AllocationData spillAllocation = GPR[i].spill;

		if (copy)
		{
			if (partial == 1) loadInstruction = mov(OperandREG8(i), byte_ptr[ref]);
			else if (partial == 2) loadInstruction = mov(OperandREG16(i), word_ptr[ref]);
			else                  loadInstruction = mov(OperandREG32(i), dword_ptr[ref]);
		}

		GPR[i].loadInstruction = loadInstruction;
		GPR[i].spillInstruction = spillInstruction;
		GPR[i].spill = spillAllocation;
		GPR[i].modified = false;

		return OperandREG32(i);
	}

	OperandREG32 RegisterAllocator::prioritize32(int i)
	{
		// Give highest priority
		GPR[i].priority = 0xFFFFFFFF;

		// Decrease priority of other registers
		for (int j = 0; j < 8; j++)
		{
			if (i == Encoding::ESP || i == Encoding::EBP) continue;

			if (j != i && GPR[j].priority)
			{
				GPR[j].priority--;
			}
		}

		return OperandREG32(i);
	}

	void RegisterAllocator::free32(int i)
	{
		if (GPR[i].loadInstruction && loadElimination)
		{
			GPR[i].loadInstruction->reserve();
			GPR[i].loadInstruction = 0;
		}

		if (GPR[i].copyInstruction && copyPropagation)
		{
			GPR[i].copyInstruction->reserve();
			GPR[i].copyInstruction = 0;
		}

		GPR[i].reference = (intptr_t)0;
		GPR[i].partial = 0;
		GPR[i].priority = 0;
	}

	Encoding* RegisterAllocator::spill32(int i)
	{
		// Register loaded but not used, eliminate load and don't spill
		if (GPR[i].loadInstruction && loadElimination)
		{
			GPR[i].loadInstruction->reserve();
			GPR[i].loadInstruction = 0;

			GPR[i].reference = (intptr_t)0;
			GPR[i].priority = 0;
			GPR[i].partial = 0;
			GPR[i].copyInstruction = 0;
			GPR[i].loadInstruction = 0;
			//	GPR[i].spillInstruction = 0;   // NOTE: Keep previous spill info

			return 0;
		}

		Encoding* spillInstruction = 0;

		if (GPR[i].reference != (intptr_t)0 && (GPR[i].modified || !dropUnmodified))
		{
			if (GPR[i].partial == 1) spillInstruction = mov(byte_ptr[GPR[i].reference], OperandREG8(i));
			else if (GPR[i].partial == 2) spillInstruction = mov(word_ptr[GPR[i].reference], OperandREG16(i));
			else                         spillInstruction = mov(dword_ptr[GPR[i].reference], OperandREG32(i));
		}

		GPR[i].free();

		return spillInstruction;
	}

	void RegisterAllocator::free(const OperandREG32& r32)
	{
		free32(r32.reg);
	}

	void RegisterAllocator::spill(const OperandREG32& r32)
	{
		spill32(r32.reg);
	}

	OperandMMREG RegisterAllocator::r64(const OperandREF& ref, bool copy)
	{
		if (ref == (intptr_t)0 && copy) throw Error("Cannot dereference 0");

		// Check if already allocated
		{for (int i = 0; i < 8; i++)
		{
			if (MMX[i].reference == ref)
			{
				return prioritize64(i);
			}
		}}

		// Check spilled but unused registers
		if (spillElimination)
		{
			for (int i = 0; i < 8; i++)
			{
				if (MMX[i].priority == 0 && MMX[i].spill.reference == ref)
				{
					if (MMX[i].spillInstruction)
					{
						MMX[i].spillInstruction->reserve();
					}

					MMX[i].reference = MMX[i].spill.reference;
					MMX[i].partial = MMX[i].spill.partial;
					MMX[i].priority = MMX[i].spill.priority;
					MMX[i].copyInstruction = MMX[i].spill.copyInstruction;
					MMX[i].loadInstruction = MMX[i].spill.loadInstruction;
					MMX[i].spillInstruction = MMX[i].spill.spillInstruction;

					MMX[i].spill.free();

					return prioritize64(i);
				}
			}
		}

		// Search for free registers
		{for (int i = 0; i < 8; i++)
		{
			if (MMX[i].priority == 0 && MMX[i].spill.priority == 0)
			{
				return allocate64(i, ref, copy);
			}
		}}

		{for (int i = 0; i < 8; i++)
		{
			if (MMX[i].priority == 0)
			{
				return allocate64(i, ref, copy);
			}
		}}

		// Need to spill one
		int candidate = 0;
		int betterCandidate = -1;
		unsigned int priority = 0xFFFFFFFF;

		{for (int i = 0; i < 8; i++)
		{
			if (MMX[i].priority < priority)
			{
				priority = MMX[i].priority;
				candidate = i;

				if (!MMX[i].modified && MMX[i].priority < 0xFFFFFFFF - 2)
				{
					betterCandidate = i;
				}
			}
		}}

		if (betterCandidate != -1)
		{
			candidate = betterCandidate;
		}

		Encoding* spillInstruction = spill64(candidate);

		MMX[candidate].spill.reference = MMX[candidate].reference;
		MMX[candidate].spill.priority = MMX[candidate].priority;
		MMX[candidate].spill.partial = MMX[candidate].partial;
		MMX[candidate].spill.copyInstruction = MMX[candidate].copyInstruction;
		MMX[candidate].spill.loadInstruction = MMX[candidate].loadInstruction;
		MMX[candidate].spill.spillInstruction = MMX[candidate].spillInstruction;

		MMX[candidate].reference = (intptr_t)0;
		MMX[candidate].priority = 0;
		MMX[candidate].partial = 0;
		MMX[candidate].copyInstruction = 0;
		MMX[candidate].loadInstruction = 0;
		MMX[candidate].spillInstruction = spillInstruction;

		return allocate64(candidate, ref, copy);
	}

	OperandMM64 RegisterAllocator::m64(const OperandREF& ref)
	{
		if (ref == (intptr_t)0) throw Error("Cannot dereference 0");

		// Check if already allocated
		for (int i = 0; i < 8; i++)
		{
			if (MMX[i].reference == ref)
			{
				return prioritize64(i);
			}
		}

		// Check spilled but unused registers
		if (spillElimination)
		{
			for (int i = 0; i < 8; i++)
			{
				if (MMX[i].priority == 0 && MMX[i].spill.reference == ref)
				{
					if (MMX[i].spillInstruction)
					{
						MMX[i].spillInstruction->reserve();
					}

					MMX[i].reference = MMX[i].spill.reference;
					MMX[i].partial = MMX[i].spill.partial;
					MMX[i].priority = MMX[i].spill.priority;
					MMX[i].copyInstruction = MMX[i].spill.copyInstruction;
					MMX[i].loadInstruction = MMX[i].spill.loadInstruction;
					MMX[i].spillInstruction = MMX[i].spill.spillInstruction;

					MMX[i].spill.free();

					return prioritize64(i);
				}
			}
		}

		return (OperandMM64)qword_ptr[ref];
	}

	OperandMMREG RegisterAllocator::allocate64(int i, const OperandREF& ref, bool copy)
	{
		MMX[i].reference = ref;

		prioritize64(i);

		Encoding* loadInstruction = 0;
		Encoding* spillInstruction = MMX[i].spillInstruction;
		AllocationData spillAllocation = MMX[i].spill;

		if (copy)
		{
			loadInstruction = movq(OperandMMREG(i), qword_ptr[ref]);
		}

		MMX[i].loadInstruction = loadInstruction;
		MMX[i].spillInstruction = spillInstruction;
		MMX[i].spill = spillAllocation;
		MMX[i].modified = false;

		return OperandMMREG(i);
	}

	OperandMMREG RegisterAllocator::prioritize64(int i)
	{
		// Give highest priority
		MMX[i].priority = 0xFFFFFFFF;

		// Decrease priority of other registers
		for (int j = 0; j < 8; j++)
		{
			if (j != i && MMX[j].priority)
			{
				MMX[j].priority--;
			}
		}

		return OperandMMREG(i);
	}

	void RegisterAllocator::free64(int i)
	{
		bool free = (MMX[i].priority != 0);

		if (MMX[i].loadInstruction && loadElimination)
		{
			MMX[i].loadInstruction->reserve();
			MMX[i].loadInstruction = 0;
		}

		if (MMX[i].copyInstruction && copyPropagation)
		{
			MMX[i].copyInstruction->reserve();
			MMX[i].copyInstruction = 0;
		}

		MMX[i].reference = (intptr_t)0;
		MMX[i].partial = 0;
		MMX[i].priority = 0;

		if (free && autoEMMS)
		{
			{for (int i = 0; i < 8; i++)
			{
				if (MMX[i].priority != 0)
				{
					return;
				}
			}}

			// Last one freed
			emms();

			// Completely eraze MMX allocation state
			{for (int i = 0; i < 8; i++)
			{
				MMX[i].free();
			}}
		}
	}

	Encoding* RegisterAllocator::spill64(int i)
	{
		// Register loaded but not used, eliminate load and don't spill
		if (MMX[i].loadInstruction && loadElimination)
		{
			MMX[i].loadInstruction->reserve();
			MMX[i].loadInstruction = 0;

			MMX[i].reference = (intptr_t)0;
			MMX[i].priority = 0;
			MMX[i].partial = 0;
			MMX[i].copyInstruction = 0;
			MMX[i].loadInstruction = 0;
			//	MMX[i].spillInstruction = 0;   // NOTE: Keep previous spill info

			return 0;
		}

		Encoding* spillInstruction = 0;

		if (MMX[i].reference != (intptr_t)0 && (MMX[i].modified || !dropUnmodified))
		{
			spillInstruction = movq(qword_ptr[MMX[i].reference], OperandMMREG(i));
		}

		MMX[i].free();

		return spillInstruction;
	}

	void RegisterAllocator::free(const OperandMMREG& r64)
	{
		free64(r64.reg);
	}

	void RegisterAllocator::spill(const OperandMMREG& r64)
	{
		spill64(r64.reg);
	}

	OperandXMMREG RegisterAllocator::r128(const OperandREF& ref, bool copy, bool ss)
	{
		if (ref == (intptr_t)0 && copy) throw Error("Cannot dereference 0");

		// Check if already allocated
		{for (int i = 0; i < 8; i++)
		{
			if (XMM[i].reference == ref)
			{
				return prioritize128(i);
			}
		}}

		// Check spilled but unused registers
		if (spillElimination)
		{
			for (int i = 0; i < 8; i++)
			{
				if (XMM[i].priority == 0 && XMM[i].spill.reference == ref)
				{
					if (XMM[i].spillInstruction)
					{
						XMM[i].spillInstruction->reserve();
					}

					XMM[i].reference = XMM[i].spill.reference;
					XMM[i].partial = XMM[i].spill.partial;
					XMM[i].priority = XMM[i].spill.priority;
					XMM[i].copyInstruction = XMM[i].spill.copyInstruction;
					XMM[i].loadInstruction = XMM[i].spill.loadInstruction;
					XMM[i].spillInstruction = XMM[i].spill.spillInstruction;

					XMM[i].spill.free();

					return prioritize128(i);
				}
			}
		}

		// Search for free registers
		{for (int i = 0; i < 8; i++)
		{
			if (XMM[i].priority == 0 && XMM[i].spill.priority == 0)
			{
				return allocate128(i, ref, copy, ss);
			}
		}}

		{for (int i = 0; i < 8; i++)
		{
			if (XMM[i].priority == 0)
			{
				return allocate128(i, ref, copy, ss);
			}
		}}

		// Need to spill one
		int candidate = 0;
		int betterCandidate = -1;
		unsigned int priority = 0xFFFFFFFF;

		{for (int i = 0; i < 8; i++)
		{
			if (XMM[i].priority < priority)
			{
				priority = XMM[i].priority;
				candidate = i;

				if (!XMM[i].modified && XMM[i].priority < 0xFFFFFFFF - 2)
				{
					betterCandidate = i;
				}
			}
		}}

		if (betterCandidate != -1)
		{
			candidate = betterCandidate;
		}

		Encoding* spillInstruction = spill128(candidate);

		XMM[candidate].spill.reference = XMM[candidate].reference;
		XMM[candidate].spill.priority = XMM[candidate].priority;
		XMM[candidate].spill.partial = XMM[candidate].partial;
		XMM[candidate].spill.copyInstruction = XMM[candidate].copyInstruction;
		XMM[candidate].spill.loadInstruction = XMM[candidate].loadInstruction;
		XMM[candidate].spill.spillInstruction = XMM[candidate].spillInstruction;

		XMM[candidate].reference = (intptr_t)0;
		XMM[candidate].priority = 0;
		XMM[candidate].partial = 0;
		XMM[candidate].copyInstruction = 0;
		XMM[candidate].loadInstruction = 0;
		XMM[candidate].spillInstruction = spillInstruction;

		return allocate128(candidate, ref, copy, ss);
	}

	OperandR_M128 RegisterAllocator::m128(const OperandREF& ref, bool ss)
	{
		if (ref == (intptr_t)0) throw Error("Cannot dereference 0");

		// Check if already allocated
		for (int i = 0; i < 8; i++)
		{
			if (XMM[i].reference == ref)
			{
				return prioritize128(i);
			}
		}

		// Check spilled but unused registers
		if (spillElimination)
		{
			for (int i = 0; i < 8; i++)
			{
				if (XMM[i].priority == 0 && XMM[i].spill.reference == ref)
				{
					if (XMM[i].spillInstruction)
					{
						XMM[i].spillInstruction->reserve();
					}

					XMM[i].reference = XMM[i].spill.reference;
					XMM[i].partial = XMM[i].spill.partial;
					XMM[i].priority = XMM[i].spill.priority;
					XMM[i].copyInstruction = XMM[i].spill.copyInstruction;
					XMM[i].loadInstruction = XMM[i].spill.loadInstruction;
					XMM[i].spillInstruction = XMM[i].spill.spillInstruction;

					XMM[i].spill.free();

					return prioritize128(i);
				}
			}
		}

		return (OperandR_M128)xword_ptr[ref];
	}

	OperandXMMREG RegisterAllocator::allocate128(int i, const OperandREF& ref, bool copy, bool ss)
	{
		XMM[i].reference = ref;
		XMM[i].partial = ss ? 4 : 0;

		prioritize128(i);

		Encoding* loadInstruction = 0;
		Encoding* spillInstruction = XMM[i].spillInstruction;
		AllocationData spillAllocation = XMM[i].spill;

		if (copy)
		{
			if (ss) loadInstruction = movss(OperandXMMREG(i), dword_ptr[ref]);
			else   loadInstruction = movaps(OperandXMMREG(i), xword_ptr[ref]);
		}

		XMM[i].loadInstruction = loadInstruction;
		XMM[i].spillInstruction = spillInstruction;
		XMM[i].spill = spillAllocation;
		XMM[i].modified = false;

		return OperandXMMREG(i);
	}

	OperandXMMREG RegisterAllocator::prioritize128(int i)
	{
		// Give highest priority
		XMM[i].priority = 0xFFFFFFFF;

		// Decrease priority of other registers
		for (int j = 0; j < 8; j++)
		{
			if (j != i && XMM[j].priority)
			{
				XMM[j].priority--;
			}
		}

		return OperandXMMREG(i);
	}

	void RegisterAllocator::free128(int i)
	{
		if (XMM[i].loadInstruction && loadElimination)
		{
			XMM[i].loadInstruction->reserve();
			XMM[i].loadInstruction = 0;
		}

		if (XMM[i].copyInstruction && copyPropagation)
		{
			XMM[i].copyInstruction->reserve();
			XMM[i].copyInstruction = 0;
		}

		XMM[i].reference = (intptr_t)0;
		XMM[i].partial = 0;
		XMM[i].priority = 0;
	}

	Encoding* RegisterAllocator::spill128(int i)
	{
		// Register loaded but not used, eliminate load and don't spill
		if (XMM[i].loadInstruction && loadElimination)
		{
			XMM[i].loadInstruction->reserve();
			XMM[i].loadInstruction = 0;

			XMM[i].reference = (intptr_t)0;
			XMM[i].priority = 0;
			XMM[i].partial = 0;
			XMM[i].copyInstruction = 0;
			XMM[i].loadInstruction = 0;
			//	XMM[i].spillInstruction = 0;   // NOTE: Keep previous spill info

			return 0;
		}

		Encoding* spillInstruction = 0;

		if (XMM[i].reference != (intptr_t)0 && (XMM[i].modified || !dropUnmodified))
		{
			if (XMM[i].partial) spillInstruction = movss(dword_ptr[XMM[i].reference], OperandXMMREG(i));
			else               spillInstruction = movaps(xword_ptr[XMM[i].reference], OperandXMMREG(i));
		}

		XMM[i].free();

		return spillInstruction;
	}

	void RegisterAllocator::free(const OperandXMMREG& r128)
	{
		free128(r128.reg);
	}

	void RegisterAllocator::spill(const OperandXMMREG& r128)
	{
		spill128(r128.reg);
	}

	OperandXMMREG RegisterAllocator::rSS(const OperandREF& ref, bool copy, bool ss)
	{
		return r128(ref, copy, ss);
	}

	OperandXMM32 RegisterAllocator::mSS(const OperandREF& ref, bool ss)
	{
		return (OperandXMM32)m128(ref, ss);
	}

	void RegisterAllocator::free(const OperandREF& ref)
	{
		{for (int i = 0; i < 8; i++)
		{
			if (i == Encoding::ESP || i == Encoding::EBP) continue;

			if (GPR[i].reference == ref)
			{
				free32(i);
			}
		}}

		{for (int i = 0; i < 8; i++)
		{
			if (MMX[i].reference == ref)
			{
				free64(i);
			}
		}}

		{for (int i = 0; i < 8; i++)
		{
			if (XMM[i].reference == ref)
			{
				free128(i);
			}
		}}
	}

	void RegisterAllocator::spill(const OperandREF& ref)
	{
		{for (int i = 0; i < 8; i++)
		{
			if (i == Encoding::ESP || i == Encoding::EBP) continue;

			if (GPR[i].reference == ref)
			{
				spill32(i);
			}
		}}

		{for (int i = 0; i < 8; i++)
		{
			if (MMX[i].reference == ref)
			{
				spill64(i);
			}
		}}

		{for (int i = 0; i < 8; i++)
		{
			if (XMM[i].reference == ref)
			{
				spill128(i);
			}
		}}
	}

	void RegisterAllocator::freeAll()
	{
		for (int i = 0; i < 8; i++)
		{
			if (i == Encoding::ESP || i == Encoding::EBP) continue;

			free32(i);
		}

		{for (int i = 0; i < 8; i++)
		{
			free64(i);
		}}

		{for (int i = 0; i < 8; i++)
		{
			free128(i);
		}}
	}

	void RegisterAllocator::spillAll()
	{
		{for (int i = 0; i < 8; i++)
		{
			// Prevent optimizations
			markModified(OperandREG32(i));
			markModified(OperandMMREG(i));
			markModified(OperandXMMREG(i));
		}}

		{for (int i = 0; i < 8; i++)
		{
			spill32(i);
			spill64(i);
			spill128(i);
		}}

		{for (int i = 0; i < 8; i++)
		{
			// Prevent optimizations
			markModified(OperandREG32(i));
			markModified(OperandMMREG(i));
			markModified(OperandXMMREG(i));
		}}
	}

	void RegisterAllocator::spillMMX()
	{
		for (int i = 0; i < 8; i++)
		{
			spill64(i);
		}
	}

	void RegisterAllocator::spillMMXcept(const OperandMMREG& r64)
	{
		for (int i = 0; i < 8; i++)
		{
			if (r64.reg != i)
			{
				spill64(i);
			}
		}

		emms();
	}

	const RegisterAllocator::State RegisterAllocator::capture()
	{
		State state;

		if (!minimalRestore)
		{
			spillAll();
			return state;   // Empty state
		}

		{for (int i = 0; i < 8; i++)
		{
			// Prevent optimizations
			markModified(OperandREG32(i));
			markModified(OperandMMREG(i));
			markModified(OperandXMMREG(i));
		}}

		{for (int i = 0; i < 8; i++)
		{
			state.GPR[i] = GPR[i];
			state.MMX[i] = MMX[i];
			state.XMM[i] = XMM[i];
		}}

		return state;
	}

	void RegisterAllocator::restore(const State& state)
	{
		if (!minimalRestore)
		{
			spillAll();
			return;
		}

		{for (int i = 0; i < 8; i++)
		{
			if (GPR[i].reference != state.GPR[i].reference)
			{
				spill32(i);
			}

			if (MMX[i].reference != state.MMX[i].reference)
			{
				spill64(i);
			}

			if (XMM[i].reference != state.XMM[i].reference)
			{
				spill128(i);
			}
		}}

		{for (int i = 0; i < 8; i++)
		{
			if (GPR[i].reference != state.GPR[i].reference && state.GPR[i].reference != (intptr_t)0)
			{
				allocate32(i, state.GPR[i].reference, true, state.GPR[i].partial);
			}

			if (MMX[i].reference != state.MMX[i].reference && state.MMX[i].reference != (intptr_t)0)
			{
				allocate64(i, state.MMX[i].reference, true);
			}

			if (XMM[i].reference != state.XMM[i].reference && state.XMM[i].reference != (intptr_t)0)
			{
				allocate128(i, state.XMM[i].reference, true, state.XMM[i].partial != 0);
			}
		}}

		{for (int i = 0; i < 8; i++)
		{
			// Prevent optimizations
			markModified(OperandREG32(i));
			markModified(OperandMMREG(i));
			markModified(OperandXMMREG(i));
		}}
	}

	void RegisterAllocator::exclude(const OperandREG32& r32)
	{
		spill(r32);
		prioritize32(r32.reg);
	}

	Encoding* RegisterAllocator::mov(OperandREG32 r32i, OperandREG32 r32j)
	{
		if (r32i == r32j) return 0;

		// Register overwritten, when not used, eliminate load instruction
		if (GPR[r32i.reg].loadInstruction && loadElimination)
		{
			GPR[r32i.reg].loadInstruction->reserve();
			GPR[r32i.reg].loadInstruction = 0;
		}

		// Register overwritten, when not used, eliminate copy instruction
		if (GPR[r32i.reg].copyInstruction && copyPropagation)
		{
			GPR[r32i.reg].copyInstruction->reserve();
			GPR[r32i.reg].copyInstruction = 0;
		}

		Encoding* spillInstruction = GPR[r32i.reg].spillInstruction;
		AllocationData spillAllocation = GPR[r32i.reg].spill;

		Encoding* mov = Assembler::mov(r32i, r32j);

		if (GPR[r32i.reg].reference == (intptr_t)0 || GPR[r32j.reg].reference == (intptr_t)0)   // Return if not in allocation table
		{
			return mov;
		}

		// Attempt copy propagation
		if (mov && copyPropagation)
		{
			swap32(r32i.reg, r32j.reg);
			GPR[r32i.reg].copyInstruction = mov;
		}

		GPR[r32i.reg].spillInstruction = spillInstruction;
		GPR[r32i.reg].spill = spillAllocation;

		return mov;
	}

	Encoding* RegisterAllocator::mov(OperandREG32 r32, OperandMEM32 m32)
	{
		if (r32.reg == Encoding::ESP || r32.reg == Encoding::EBP)
		{
			return Assembler::mov(r32, m32);
		}

		// Register overwritten, when not used, eliminate load instruction
		if (GPR[r32.reg].loadInstruction && loadElimination)
		{
			GPR[r32.reg].loadInstruction->reserve();
			GPR[r32.reg].loadInstruction = 0;
		}

		// Register overwritten, when not used, eliminate copy instruction
		if (GPR[r32.reg].copyInstruction && copyPropagation)
		{
			GPR[r32.reg].copyInstruction->reserve();
			GPR[r32.reg].copyInstruction = 0;
		}

		Encoding* spillInstruction = GPR[r32.reg].spillInstruction;
		AllocationData spillAllocation = GPR[r32.reg].spill;

		Encoding* mov = Assembler::mov(r32, m32);

		GPR[r32.reg].spillInstruction = spillInstruction;
		GPR[r32.reg].spill = spillAllocation;

		return mov;
	}

	Encoding* RegisterAllocator::mov(OperandREG32 r32, OperandR_M32 r_m32)
	{
		if (r_m32.isSubtypeOf(Operand::OPERAND_REG32))
		{
			return mov(r32, (OperandREG32)r_m32);
		}
		else
		{
			return mov(r32, (OperandMEM32)r_m32);
		}
	}

	Encoding* RegisterAllocator::movq(OperandMMREG r64i, OperandMMREG r64j)
	{
		if (r64i == r64j) return 0;

		// Register overwritten, when not used, eliminate load instruction
		if (MMX[r64i.reg].loadInstruction && loadElimination)
		{
			MMX[r64i.reg].loadInstruction->reserve();
			MMX[r64i.reg].loadInstruction = 0;
		}

		// Register overwritten, when not used, eliminate copy instruction
		if (MMX[r64i.reg].copyInstruction && copyPropagation)
		{
			MMX[r64i.reg].copyInstruction->reserve();
			MMX[r64i.reg].copyInstruction = 0;
		}

		Encoding* spillInstruction = MMX[r64i.reg].spillInstruction;
		AllocationData spillAllocation = MMX[r64i.reg].spill;

		Encoding* movq = Assembler::movq(r64i, r64j);

		if (MMX[r64i.reg].reference == (intptr_t)0 || MMX[r64j.reg].reference == (intptr_t)0)   // Return if not in allocation table
		{
			return movq;
		}

		// Attempt copy propagation
		if (movq && copyPropagation)
		{
			swap64(r64i.reg, r64j.reg);
			MMX[r64i.reg].copyInstruction = movq;
		}

		MMX[r64i.reg].spillInstruction = spillInstruction;
		MMX[r64i.reg].spill = spillAllocation;

		return movq;
	}

	Encoding* RegisterAllocator::movq(OperandMMREG r64, OperandMEM64 m64)
	{
		// Register overwritten, when not used, eliminate load instruction
		if (MMX[r64.reg].loadInstruction && loadElimination)
		{
			MMX[r64.reg].loadInstruction->reserve();
			MMX[r64.reg].loadInstruction = 0;
		}

		// Register overwritten, when not used, eliminate copy instruction
		if (MMX[r64.reg].copyInstruction && copyPropagation)
		{
			MMX[r64.reg].copyInstruction->reserve();
			MMX[r64.reg].copyInstruction = 0;
		}

		Encoding* spillInstruction = MMX[r64.reg].spillInstruction;
		AllocationData spillAllocation = MMX[r64.reg].spill;

		Encoding* movq = Assembler::movq(r64, m64);

		MMX[r64.reg].spillInstruction = spillInstruction;
		MMX[r64.reg].spill = spillAllocation;

		return movq;
	}

	Encoding* RegisterAllocator::movq(OperandMMREG r64, OperandMM64 r_m64)
	{
		if (r_m64.isSubtypeOf(Operand::OPERAND_MMREG))
		{
			return movq(r64, (OperandMMREG)r_m64);
		}
		else
		{
			return movq(r64, (OperandMEM64)r_m64);
		}
	}

	Encoding* RegisterAllocator::movaps(OperandXMMREG r128i, OperandXMMREG r128j)
	{
		if (r128i == r128j) return 0;

		// Register overwritten, when not used, eliminate load instruction
		if (XMM[r128i.reg].loadInstruction && loadElimination)
		{
			XMM[r128i.reg].loadInstruction->reserve();
			XMM[r128i.reg].loadInstruction = 0;
		}

		// Register overwritten, when not used, eliminate copy instruction
		if (XMM[r128i.reg].copyInstruction && copyPropagation)
		{
			XMM[r128i.reg].copyInstruction->reserve();
			XMM[r128i.reg].copyInstruction = 0;
		}

		Encoding* spillInstruction = XMM[r128i.reg].spillInstruction;
		AllocationData spillAllocation = XMM[r128i.reg].spill;

		Encoding* movaps = Assembler::movaps(r128i, r128j);

		if (XMM[r128i.reg].reference == (intptr_t)0 || XMM[r128j.reg].reference == (intptr_t)0)   // Return if not in allocation table
		{
			return movaps;
		}

		// Attempt copy propagation
		if (movaps && copyPropagation)
		{
			swap128(r128i.reg, r128j.reg);
			XMM[r128i.reg].copyInstruction = movaps;
		}

		XMM[r128i.reg].spillInstruction = spillInstruction;
		XMM[r128i.reg].spill = spillAllocation;

		return movaps;
	}

	Encoding* RegisterAllocator::movaps(OperandXMMREG r128, OperandMEM128 m128)
	{
		// Register overwritten, when not used, eliminate load instruction
		if (XMM[r128.reg].loadInstruction && loadElimination)
		{
			XMM[r128.reg].loadInstruction->reserve();
			XMM[r128.reg].loadInstruction = 0;
		}

		// Register overwritten, when not used, eliminate copy instruction
		if (XMM[r128.reg].copyInstruction && copyPropagation)
		{
			XMM[r128.reg].copyInstruction->reserve();
			XMM[r128.reg].copyInstruction = 0;
		}

		Encoding* spillInstruction = XMM[r128.reg].spillInstruction;
		AllocationData spillAllocation = XMM[r128.reg].spill;

		Encoding* movaps = Assembler::movaps(r128, m128);

		XMM[r128.reg].spillInstruction = spillInstruction;
		XMM[r128.reg].spill = spillAllocation;

		return movaps;
	}

	Encoding* RegisterAllocator::movaps(OperandXMMREG r128, OperandR_M128 r_m128)
	{
		if (r_m128.isSubtypeOf(Operand::OPERAND_XMMREG))
		{
			return movaps(r128, (OperandXMMREG)r_m128);
		}
		else
		{
			return movaps(r128, (OperandMEM128)r_m128);
		}
	}

	void RegisterAllocator::enableAutoEMMS()
	{
		autoEMMS = true;
	}

	void RegisterAllocator::disableAutoEMMS()
	{
		autoEMMS = false;
	}

	void RegisterAllocator::enableCopyPropagation()
	{
		copyPropagation = true;
	}

	void RegisterAllocator::disableCopyPropagation()
	{
		copyPropagation = false;
	}

	void RegisterAllocator::enableLoadElimination()
	{
		loadElimination = true;
	}

	void RegisterAllocator::disableLoadElimination()
	{
		loadElimination = false;
	}

	void RegisterAllocator::enableSpillElimination()
	{
		spillElimination = true;
	}

	void RegisterAllocator::disableSpillElimination()
	{
		spillElimination = false;
	}

	void RegisterAllocator::enableMinimalRestore()
	{
		minimalRestore = true;
	}

	void RegisterAllocator::disableMinimalRestore()
	{
		minimalRestore = false;
	}

	void RegisterAllocator::enableDropUnmodified()
	{
		dropUnmodified = true;
	}

	void RegisterAllocator::disableDropUnmodified()
	{
		dropUnmodified = false;
	}

	Encoding* RegisterAllocator::x86(int instructionID, const Operand& firstOperand, const Operand& secondOperand, const Operand& thirdOperand)
	{
		markModified(firstOperand);
		markReferenced(secondOperand);

		return Assembler::x86(instructionID, firstOperand, secondOperand, thirdOperand);
	}

	void RegisterAllocator::markModified(const Operand& op)
	{
		if (Operand::isReg(op))
		{
			if (op.type == Operand::OPERAND_REG64 ||
				op.type == Operand::OPERAND_REG32 ||
				op.type == Operand::OPERAND_REG16 ||
				op.type == Operand::OPERAND_REG8 ||
				op.type == Operand::OPERAND_EAX ||
				op.type == Operand::OPERAND_ECX ||
				op.type == Operand::OPERAND_AX ||
				op.type == Operand::OPERAND_DX ||
				op.type == Operand::OPERAND_CX ||
				op.type == Operand::OPERAND_AL ||
				op.type == Operand::OPERAND_CL)
			{
				if (op.reg == Encoding::ESP || op.reg == Encoding::EBP) return;

				if (GPR[op.reg].copyInstruction)
				{
					GPR[op.reg].copyInstruction->retain();
					GPR[op.reg].copyInstruction = 0;
				}

				if (GPR[op.reg].loadInstruction)
				{
					GPR[op.reg].loadInstruction->retain();
					GPR[op.reg].loadInstruction = 0;
				}

				if (GPR[op.reg].spillInstruction)
				{
					GPR[op.reg].spillInstruction->retain();
					GPR[op.reg].spillInstruction = 0;

					GPR[op.reg].spill.free();
				}

				GPR[op.reg].modified = true;
			}
			else if (op.type == Operand::OPERAND_MMREG)
			{
				if (MMX[op.reg].copyInstruction)
				{
					MMX[op.reg].copyInstruction->retain();
					MMX[op.reg].copyInstruction = 0;
				}

				if (MMX[op.reg].loadInstruction)
				{
					MMX[op.reg].loadInstruction->retain();
					MMX[op.reg].loadInstruction = 0;
				}

				if (MMX[op.reg].spillInstruction)
				{
					MMX[op.reg].spillInstruction->retain();
					MMX[op.reg].spillInstruction = 0;

					MMX[op.reg].spill.free();
				}

				MMX[op.reg].modified = true;
			}
			else if (op.type == Operand::OPERAND_XMMREG)
			{
				if (XMM[op.reg].copyInstruction)
				{
					XMM[op.reg].copyInstruction->retain();
					XMM[op.reg].copyInstruction = 0;
				}

				if (XMM[op.reg].loadInstruction)
				{
					XMM[op.reg].loadInstruction->retain();
					XMM[op.reg].loadInstruction = 0;
				}

				if (XMM[op.reg].spillInstruction)
				{
					XMM[op.reg].spillInstruction->retain();
					XMM[op.reg].spillInstruction = 0;

					XMM[op.reg].spill.free();
				}

				XMM[op.reg].modified = true;
			}
			else if (op.isSubtypeOf(Operand::OPERAND_FPUREG))
			{
			}
			else
			{
				throw INTERNAL_ERROR;
			}
		}
		else if (Operand::isMem(op))
		{
			if (op.baseReg != Encoding::REG_UNKNOWN)
			{
				markReferenced(OperandREG32(op.baseReg));
			}

			if (op.indexReg != Encoding::REG_UNKNOWN)
			{
				markReferenced(OperandREG32(op.indexReg));
			}
		}
	}

	void RegisterAllocator::markReferenced(const Operand& op)
	{
		if (Operand::isReg(op))
		{
			if (op.type == Operand::OPERAND_REG64 ||
				op.type == Operand::OPERAND_REG32 ||
				op.type == Operand::OPERAND_REG16 ||
				op.type == Operand::OPERAND_REG8 ||
				op.type == Operand::OPERAND_EAX ||
				op.type == Operand::OPERAND_ECX ||
				op.type == Operand::OPERAND_AX ||
				op.type == Operand::OPERAND_DX ||
				op.type == Operand::OPERAND_CX ||
				op.type == Operand::OPERAND_AL ||
				op.type == Operand::OPERAND_CL)
			{
				if (op.reg == Encoding::ESP || op.reg == Encoding::EBP) return;

				if (GPR[op.reg].copyInstruction)
				{
					GPR[op.reg].copyInstruction->retain();
					GPR[op.reg].copyInstruction = 0;
				}

				if (GPR[op.reg].loadInstruction)
				{
					GPR[op.reg].loadInstruction->retain();
					GPR[op.reg].loadInstruction = 0;
				}

				if (GPR[op.reg].spillInstruction)
				{
					GPR[op.reg].spillInstruction->retain();
					GPR[op.reg].spillInstruction = 0;

					GPR[op.reg].spill.free();
				}
			}
			else if (op.type == Operand::OPERAND_MMREG)
			{
				if (MMX[op.reg].copyInstruction)
				{
					MMX[op.reg].copyInstruction->retain();
					MMX[op.reg].copyInstruction = 0;
				}

				if (MMX[op.reg].loadInstruction)
				{
					MMX[op.reg].loadInstruction->retain();
					MMX[op.reg].loadInstruction = 0;
				}

				if (MMX[op.reg].spillInstruction)
				{
					MMX[op.reg].spillInstruction->retain();
					MMX[op.reg].spillInstruction = 0;

					MMX[op.reg].spill.free();
				}
			}
			else if (op.type == Operand::OPERAND_XMMREG)
			{
				if (XMM[op.reg].copyInstruction)
				{
					XMM[op.reg].copyInstruction->retain();
					XMM[op.reg].copyInstruction = 0;
				}

				if (XMM[op.reg].loadInstruction)
				{
					XMM[op.reg].loadInstruction->retain();
					XMM[op.reg].loadInstruction = 0;
				}

				if (XMM[op.reg].spillInstruction)
				{
					XMM[op.reg].spillInstruction->retain();
					XMM[op.reg].spillInstruction = 0;

					XMM[op.reg].spill.free();
				}
			}
			else if (op.isSubtypeOf(Operand::OPERAND_FPUREG))
			{
			}
			else
			{
				throw INTERNAL_ERROR;
			}
		}
		else if (Operand::isMem(op))
		{
			if (op.baseReg != Encoding::REG_UNKNOWN)
			{
				markReferenced(OperandREG32(op.baseReg));
			}

			if (op.indexReg != Encoding::REG_UNKNOWN)
			{
				markReferenced(OperandREG32(op.indexReg));
			}
		}
	}

	void RegisterAllocator::swap32(int i, int j)
	{
		Allocation* source = &GPR[j];
		Allocation* destination = &GPR[i];

		// Swap references, priorities, etc.
		OperandREF swapRef = source->reference;
		source->reference = destination->reference;
		destination->reference = swapRef;

		int swapPriority = source->priority;
		source->priority = destination->priority;
		destination->priority = swapPriority;

		int swapPartial = source->partial;
		source->partial = destination->partial;
		destination->partial = swapPartial;

		bool swapModified = source->modified;
		source->modified = destination->modified;
		destination->modified = swapModified;
	}

	void RegisterAllocator::swap64(int i, int j)
	{
		Allocation* source = &MMX[j];
		Allocation* destination = &MMX[i];

		// Swap references, priorities, etc.
		OperandREF swapRef = source->reference;
		source->reference = destination->reference;
		destination->reference = swapRef;

		int swapPriority = source->priority;
		source->priority = destination->priority;
		destination->priority = swapPriority;

		int swapPartial = source->partial;
		source->partial = destination->partial;
		destination->partial = swapPartial;

		bool swapModified = source->modified;
		source->modified = destination->modified;
		destination->modified = swapModified;
	}

	void RegisterAllocator::swap128(int i, int j)
	{
		Allocation* source = &XMM[j];
		Allocation* destination = &XMM[i];

		// Swap references, priorities, etc.
		OperandREF swapRef = source->reference;
		source->reference = destination->reference;
		destination->reference = swapRef;

		int swapPriority = source->priority;
		source->priority = destination->priority;
		destination->priority = swapPriority;

		int swapPartial = source->partial;
		source->partial = destination->partial;
		destination->partial = swapPartial;

		bool swapModified = source->modified;
		source->modified = destination->modified;
		destination->modified = swapModified;
	}
}


namespace SoftWire
{
	Synthesizer::Synthesizer(bool x64) : encoding(0), x64(x64)
	{
		reset();
	}

	Synthesizer::~Synthesizer()
	{
	}

	void Synthesizer::reset()
	{
		encoding.reset();

		firstType = Operand::OPERAND_UNKNOWN;
		secondType = Operand::OPERAND_UNKNOWN;

		firstReg = Encoding::REG_UNKNOWN;
		secondReg = Encoding::REG_UNKNOWN;
		baseReg = Encoding::REG_UNKNOWN;
		indexReg = Encoding::REG_UNKNOWN;

		scale = 0;
	}

	void Synthesizer::defineLabel(const char* label)
	{
		if (encoding.label != 0)
		{
			throw INTERNAL_ERROR;   // Parser error
		}

		encoding.setLabel(label);
	}

	void Synthesizer::referenceLabel(const char* label)
	{
		if (label)
		{
			if (encoding.reference != (intptr_t)0)
			{
				throw Error("Instruction can't have multiple references");
			}

			encoding.setReference(label);
		}
	}

	void Synthesizer::encodeFirstOperand(const Operand& firstOperand)
	{
		if (firstType != Operand::OPERAND_UNKNOWN)
		{
			throw INTERNAL_ERROR;   // Instruction destination already set
		}

		firstType = firstOperand.type;

		if (Operand::isReg(firstType))
		{
			firstReg = firstOperand.reg;
		}
		else if (Operand::isMem(firstType))
		{
			encodeBase(firstOperand);
			encodeIndex(firstOperand);

			setScale(firstOperand.scale);
			setDisplacement(firstOperand.displacement);

			referenceLabel(firstOperand.reference);
		}
		else if (Operand::isImm(firstType))
		{
			encodeImmediate(firstOperand.value);
			referenceLabel(firstOperand.reference);
		}
		else if (!Operand::isVoid(firstType))
		{
			throw INTERNAL_ERROR;
		}
	}

	void Synthesizer::encodeSecondOperand(const Operand& secondOperand)
	{
		if (secondType != Operand::OPERAND_UNKNOWN)
		{
			throw INTERNAL_ERROR;   // Instruction source already set
		}

		secondType = secondOperand.type;

		if (Operand::isReg(secondType))
		{
			secondReg = secondOperand.reg;
		}
		else if (Operand::isMem(secondType))
		{
			encodeBase(secondOperand);
			encodeIndex(secondOperand);

			setScale(secondOperand.scale);
			setDisplacement(secondOperand.displacement);

			referenceLabel(secondOperand.reference);
		}
		else if (Operand::isImm(secondType))
		{
			encodeImmediate(secondOperand.value);
			referenceLabel(secondOperand.reference);
		}
		else if (!Operand::isVoid(secondType))
		{
			throw INTERNAL_ERROR;
		}
	}

	void Synthesizer::encodeThirdOperand(const Operand& thirdOperand)
	{
		if (Operand::isImm(thirdOperand))
		{
			encodeImmediate(thirdOperand.value);
			referenceLabel(thirdOperand.reference);
		}
		else if (!Operand::isVoid(thirdOperand))
		{
			throw INTERNAL_ERROR;
		}
	}

	void Synthesizer::encodeBase(const Operand& base)
	{
		if (baseReg != Encoding::REG_UNKNOWN)
		{
			// Base already set, use as index with scale = 1
			encodeIndex(base);
			setScale(1);
			return;
		}

		baseReg = base.baseReg;
	}

	void Synthesizer::encodeIndex(const Operand& index)
	{
		if (indexReg != Encoding::REG_UNKNOWN)
		{
			throw Error("Memory reference can't have multiple index registers");
		}

		indexReg = index.indexReg;
	}

	void Synthesizer::setScale(int scale)
	{
		if (this->scale != 0)
		{
			throw Error("Memory reference can't have multiple scale factors");
		}

		if (scale != 0 && scale != 1 && scale != 2 && scale != 4 && scale != 8)
		{
			throw Error("Invalid scale value '%d'", scale);
		}

		this->scale = scale;
	}

	void Synthesizer::encodeImmediate(int immediate)
	{
		if (encoding.immediate != 0xCCCCCCCC)
		{
			throw Error("Instruction can't have multiple immediate operands");
		}

		encoding.immediate = immediate;
	}

	void Synthesizer::setDisplacement(intptr_t displacement)
	{
		encoding.setDisplacement(displacement);
	}

	void Synthesizer::encodeLiteral(const char* string)
	{
		encoding.literal = strdup(string);
		encoding.format.O1 = false;   // Indicates that this is data
	}

	const Encoding& Synthesizer::encodeInstruction(const Instruction* instruction)
	{
		if (!instruction) return encoding;
		encoding.instruction = instruction;

		if (x64 && instruction->isInvalid64())
		{
			throw Error("Invalid instruction for x86-64 long mode");
		}

		const char* format = instruction->getEncoding();

		if (!format) throw INTERNAL_ERROR;

		while (*format)
		{
			switch ((format[0] << 8) | format[1])
			{
			case LOCK_PRE:
				encoding.addPrefix(0xF0);
				break;
			case CONST_PRE:
				encoding.addPrefix(0xF1);
				break;
			case REPNE_PRE:
				encoding.addPrefix(0xF2);
				break;
			case REP_PRE:
				encoding.addPrefix(0xF3);
				break;
			case OFF_PRE:
				if (!instruction->is32Bit())
				{
					encoding.addPrefix(0x66);
				}
				break;
			case ADDR_PRE:
				if (!instruction->is32Bit())
				{
					encoding.addPrefix(0x67);
				}
				break;
			case ADD_REG:
				encodeRexByte(instruction);
				if (encoding.format.O1)
				{
					if (Operand::isReg(firstType) && firstType != Operand::OPERAND_ST0)
					{
						encoding.O1 += firstReg & 0x7;
						encoding.REX.B = (firstReg & 0x8) >> 3;
					}
					else if (Operand::isReg(secondType))
					{
						encoding.O1 += secondReg & 0x7;
						encoding.REX.B = (secondReg & 0x8) >> 3;
					}
					else if (Operand::isReg(firstType) && firstType == Operand::OPERAND_ST0)
					{
						encoding.O1 += firstReg & 0x7;
						encoding.REX.B = (firstReg & 0x8) >> 3;
					}
					else
					{
						throw INTERNAL_ERROR;   // '+r' not compatible with operands
					}
				}
				else
				{
					throw INTERNAL_ERROR;   // '+r' needs first opcode byte
				}
				break;
			case EFF_ADDR:
				encodeRexByte(instruction);
				encodeModField();
				encodeRegField(instruction);
				encodeR_MField(instruction);
				encodeSibByte(instruction);
				break;
			case MOD_RM_0:
			case MOD_RM_1:
			case MOD_RM_2:
			case MOD_RM_3:
			case MOD_RM_4:
			case MOD_RM_5:
			case MOD_RM_6:
			case MOD_RM_7:
				encodeRexByte(instruction);
				encodeModField();
				encoding.modRM.reg = format[1] - '0';
				encodeR_MField(instruction);
				encodeSibByte(instruction);
				break;
			case QWORD_IMM:
				throw INTERNAL_ERROR;   // FIXME: Unimplemented
				break;
			case DWORD_IMM:
				encoding.format.I1 = true;
				encoding.format.I2 = true;
				encoding.format.I3 = true;
				encoding.format.I4 = true;
				break;
			case WORD_IMM:
				encoding.format.I1 = true;
				encoding.format.I2 = true;
				break;
			case BYTE_IMM:
				encoding.format.I1 = true;
				break;
			case BYTE_REL:
				encoding.format.I1 = true;
				encoding.relative = true;
				break;
			case DWORD_REL:
				encoding.format.I1 = true;
				encoding.format.I2 = true;
				encoding.format.I3 = true;
				encoding.format.I4 = true;
				encoding.relative = true;
				break;
			default:
				unsigned int opcode = strtoul(format, 0, 16);

				if (opcode > 0xFF)
				{
					throw INTERNAL_ERROR;
				}

				if (!encoding.format.O1)
				{
					encoding.O1 = (unsigned char)opcode;

					encoding.format.O1 = true;
				}
				else if ((!encoding.format.O2)/* &&
						(encoding.O1 == 0x0F ||
						 encoding.O1 == 0xD8 ||
						 encoding.O1 == 0xD9 ||
						 encoding.O1 == 0xDA ||
						 encoding.O1 == 0xDB ||
						 encoding.O1 == 0xDC ||
						 encoding.O1 == 0xDD ||
						 encoding.O1 == 0xDE ||
						 encoding.O1 == 0xDF || encoding.O1 == 0x66)*/)
				{
					encoding.O2 = encoding.O1;
					encoding.O1 = opcode;

					encoding.format.O2 = true;
				}
				else if (encoding.format.O2 && !encoding.format.O3) {
					encoding.O3 = encoding.O2;
					encoding.O2 = encoding.O1;
					encoding.O1 = opcode;

					encoding.format.O3 = true;
				}
				else if (encoding.format.O3 && !encoding.format.O4) {
					encoding.O4 = encoding.O3;
					encoding.O3 = encoding.O2;
					encoding.O2 = encoding.O1;
					encoding.O1 = opcode;

					encoding.format.O4 = true;
				}
				/*				else if(encoding.O1 == 0x66)   // Operand size prefix for SSE2
								{
									encoding.addPrefix(0x66);   // HACK: Might not be valid for later instruction sets

									encoding.O1 = opcode;
								}
								else if(encoding.O1 == 0x9B)   // FWAIT
								{
									encoding.addPrefix(0x9B);   // HACK: Might not be valid for later instruction sets

									encoding.O1 = opcode;
								}
				*/
				else   // 3DNow!, SSE or SSE2 instruction, opcode as immediate
				{
					encoding.format.I1 = true;

					encoding.I1 = opcode;
				}
			}

			format += 2;

			if (*format == ' ')
			{
				format++;
			}
			else if (*format == '\0')
			{
				break;
			}
			else
			{
				throw INTERNAL_ERROR;
			}
		}

		return encoding;
	}

	void Synthesizer::encodeRexByte(const Instruction* instruction)
	{
		if (instruction->is64Bit() || firstReg > 0x07 || secondReg > 0x07 || baseReg > 0x07 || indexReg > 0x07)
		{
			encoding.format.REX = true;
			encoding.REX.prefix = 0x4;
			encoding.REX.W = 0;
			encoding.REX.R = 0;
			encoding.REX.X = 0;
			encoding.REX.B = 0;
		}

		if (instruction->is64Bit())
		{
			encoding.REX.W = true;
		}
	}

	void Synthesizer::encodeModField()
	{
		encoding.format.modRM = true;

		if (Operand::isReg(firstType) &&
			(Operand::isReg(secondType) || Operand::isImm(secondType) || Operand::isVoid(secondType)))
		{
			encoding.modRM.mod = Encoding::MOD_REG;
		}
		else
		{
			if (baseReg == Encoding::REG_UNKNOWN)   // Static address
			{
				encoding.modRM.mod = Encoding::MOD_NO_DISP;
				encoding.format.D1 = true;
				encoding.format.D2 = true;
				encoding.format.D3 = true;
				encoding.format.D4 = true;
			}
			else if (encoding.reference && !encoding.displacement)
			{
				encoding.modRM.mod = Encoding::MOD_DWORD_DISP;
				encoding.format.D1 = true;
				encoding.format.D2 = true;
				encoding.format.D3 = true;
				encoding.format.D4 = true;
			}
			else if (!encoding.displacement)
			{
				if (baseReg == Encoding::EBP)
				{
					encoding.modRM.mod = Encoding::MOD_BYTE_DISP;
					encoding.format.D1 = true;
				}
				else
				{
					encoding.modRM.mod = Encoding::MOD_NO_DISP;
				}
			}
			else if ((char)encoding.displacement == encoding.displacement)
			{
				encoding.modRM.mod = Encoding::MOD_BYTE_DISP;
				encoding.format.D1 = true;
			}
			else
			{
				encoding.modRM.mod = Encoding::MOD_DWORD_DISP;
				encoding.format.D1 = true;
				encoding.format.D2 = true;
				encoding.format.D3 = true;
				encoding.format.D4 = true;
			}
		}
	}

	void Synthesizer::encodeR_MField(const Instruction* instruction)
	{
		int r_m;

		if (Operand::isReg(instruction->getFirstOperand()) &&
			Operand::isR_M(instruction->getSecondOperand()))
		{
			if (Operand::isMem(secondType))
			{
				if (baseReg == Encoding::REG_UNKNOWN)
				{
					r_m = Encoding::EBP;   // Static address
				}
				else
				{
					r_m = baseReg;
				}
			}
			else if (Operand::isReg(secondType))
			{
				r_m = secondReg;
			}
			else
			{
				throw INTERNAL_ERROR;   // Syntax error should be detected by parser
			}
		}
		else if (Operand::isR_M(instruction->getFirstOperand()) &&
			Operand::isReg(instruction->getSecondOperand()))
		{
			if (Operand::isMem(firstType))
			{
				if (baseReg == Encoding::REG_UNKNOWN)
				{
					r_m = Encoding::EBP;   // Static address
				}
				else
				{
					r_m = baseReg;
				}
			}
			else if (Operand::isReg(firstType))
			{
				r_m = firstReg;
			}
			else
			{
				throw INTERNAL_ERROR;   // Syntax error should be detected by parser
			}
		}
		else
		{
			if (Operand::isMem(firstType))
			{
				if (baseReg != Encoding::REG_UNKNOWN)
				{
					r_m = baseReg;
				}
				else
				{
					r_m = Encoding::EBP;   // Displacement only
				}
			}
			else if (Operand::isReg(firstType))
			{
				r_m = firstReg;
			}
			else
			{
				throw INTERNAL_ERROR;   // Syntax error should be caught by parser
			}
		}

		encoding.modRM.r_m = r_m & 0x07;
		encoding.REX.B = (r_m & 0x8) >> 3;
	}

	void Synthesizer::encodeRegField(const Instruction* instruction)
	{
		int reg;

		if (Operand::isReg(instruction->getFirstOperand()) &&
			Operand::isR_M(instruction->getSecondOperand()))
		{
			reg = firstReg;
		}
		else if (Operand::isR_M(instruction->getFirstOperand()) &&
			Operand::isReg(instruction->getSecondOperand()))
		{
			reg = secondReg;
		}
		else if (Operand::isReg(instruction->getFirstOperand()) &&
			Operand::isImm(instruction->getSecondOperand()))   // IMUL working on the same register
		{
			reg = firstReg;
		}
		else
		{
			throw INTERNAL_ERROR;
		}

		encoding.modRM.reg = reg & 0x07;
		encoding.REX.R = (reg & 0x8) >> 3;
	}

	void Synthesizer::encodeSibByte(const Instruction* instruction)
	{
		if (scale == 0 && indexReg == Encoding::REG_UNKNOWN)
		{
			if (baseReg == Encoding::REG_UNKNOWN || encoding.modRM.r_m != Encoding::ESP)
			{
				if (encoding.format.SIB)
				{
					throw INTERNAL_ERROR;
				}

				return;   // No SIB byte needed
			}
		}

		encoding.format.SIB = true;

		encoding.modRM.r_m = Encoding::ESP;   // Indicates use of SIB in mod R/M

		if (baseReg == Encoding::EBP && encoding.modRM.mod == Encoding::MOD_NO_DISP)
		{
			encoding.modRM.mod = Encoding::MOD_BYTE_DISP;

			encoding.format.D1 = true;
		}

		if (indexReg == Encoding::ESP)
		{
			if (scale != 1)
			{
				throw Error("ESP can't be scaled index in memory reference");
			}
			else   // Switch base and index
			{
				int tempReg;

				tempReg = indexReg;
				indexReg = baseReg;
				baseReg = tempReg;
			}
		}

		if (baseReg == Encoding::REG_UNKNOWN)
		{
			encoding.SIB.base = Encoding::EBP;   // No Base

			encoding.modRM.mod = Encoding::MOD_NO_DISP;
			encoding.format.D1 = true;
			encoding.format.D2 = true;
			encoding.format.D3 = true;
			encoding.format.D4 = true;
		}
		else
		{
			encoding.SIB.base = baseReg & 0x7;
			encoding.REX.X = (baseReg & 0x8) >> 3;
		}

		if (indexReg != Encoding::REG_UNKNOWN)
		{
			encoding.SIB.index = indexReg & 0x7;
			encoding.REX.X = (indexReg & 0x8) >> 3;
		}
		else
		{
			encoding.SIB.index = Encoding::ESP;
		}

		switch (scale)
		{
		case 0:
		case 1:
			encoding.SIB.scale = Encoding::SCALE_1;
			break;
		case 2:
			encoding.SIB.scale = Encoding::SCALE_2;
			break;
		case 4:
			encoding.SIB.scale = Encoding::SCALE_4;
			break;
		case 8:
			encoding.SIB.scale = Encoding::SCALE_8;
			break;
		default:
			throw INTERNAL_ERROR;
		}
	}
}


namespace SoftWire
{
	int CodeGenerator::stack = -128;
	int CodeGenerator::stackTop = -128;
	Encoding* CodeGenerator::stackUpdate = 0;

	CodeGenerator* CodeGenerator::cg = 0;

	CodeGenerator::Variable::Variable(int size) : size(size)
	{
		previous = stack;
		reference = (stack + size - 1) & ~(size - 1);
		stack = reference + size;

		// Grow stack when required
		if (stack > stackTop)
		{
			if (stackUpdate)
			{
				stackTop += 16;

				if (!cg->x64)
				{
					stackUpdate->setImmediate(stackTop);
				}
				else
				{
					stackUpdate->setImmediate(32 + stackTop + 128);
				}
			}
			else if (stackTop != -128)   // Skip arg
			{
				throw Error("Stack used without prologue");
			}
		}
	}

	CodeGenerator::Variable::~Variable()
	{
		if (reference == 0xDEADC0DE) return;   // Already freed

		cg->free((OperandREF)(ebp + reference));
		reference = 0xDEADC0DE;

		for (int i = 0; i < 8; i++)
		{
			if (GPR[i].reference.baseReg == Encoding::EBP && GPR[i].reference.displacement > previous) return;
			if (MMX[i].reference.baseReg == Encoding::EBP && MMX[i].reference.displacement > previous) return;
			if (XMM[i].reference.baseReg == Encoding::EBP && XMM[i].reference.displacement > previous) return;
		}

		stack = previous;   // Free stack space when allocated at top of stack
	}

	void CodeGenerator::Variable::free()
	{
		// Explicitely destruct
		this->~Variable();
	}

	intptr_t CodeGenerator::Variable::ref() const
	{
		if (reference == 0xDEADC0DE)
		{
			throw Error("Freed variables can no longer be accessed!");
		}

		return reference;
	}

	CodeGenerator::Byte::Byte() : Variable(1)
	{
	}

	CodeGenerator::Byte::operator OperandREG8() const
	{
		return cg->r8(ebp + ref());
	}

	CodeGenerator::Char::Char()
	{
	}

	CodeGenerator::Char::Char(unsigned char c)
	{
		cg->mov(*this, c);
	}

	CodeGenerator::Char::Char(const Char& c)
	{
		cg->mov(*this, c);
	}

	CodeGenerator::Char& CodeGenerator::Char::operator=(const Char& c)
	{
		cg->mov(*this, cg->m8(ebp + c.ref()));
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator+=(const Char& c)
	{
		cg->add(*this, cg->m8(ebp + c.ref()));
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator-=(const Char& c)
	{
		cg->sub(*this, cg->m8(ebp + c.ref()));
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator*=(const Char& c)
	{
		cg->exclude(eax);
		cg->mov(al, cg->m8(ebp + ref()));
		cg->imul(c);
		cg->mov(*this, al);
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator/=(const Char& c)
	{
		cg->exclude(eax);
		cg->exclude(edx);
		cg->mov(al, cg->m8(ebp + ref()));
		cg->mov(dl, cg->m8(ebp + c.ref()));
		cg->idiv(dl);
		cg->mov(*this, al);
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator%=(const Char& c)
	{
		cg->exclude(eax);
		cg->exclude(edx);
		cg->mov(al, cg->m8(ebp + ref()));
		cg->mov(dl, cg->m8(ebp + c.ref()));
		cg->idiv(dl);
		cg->mov(*this, dl);
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator<<=(const Char& c)
	{
		cg->exclude(ecx);
		cg->mov(cl, cg->m8(ebp + c.ref()));
		cg->shl(*this, cl);
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator>>=(const Char& c)
	{
		cg->exclude(ecx);
		cg->mov(cl, cg->m8(ebp + c.ref()));
		cg->shr(*this, cl);
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator&=(const Char& c)
	{
		cg-> and (*this, cg->m8(ebp + c.ref()));
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator^=(const Char& c)
	{
		cg-> xor (*this, cg->m8(ebp + c.ref()));
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator|=(const Char& c)
	{
		cg-> or (*this, cg->m8(ebp + c.ref()));
		return *this;
	}

	CodeGenerator::Char CodeGenerator::Char::operator+(const Char& c)
	{
		Char temp;
		temp = *this;
		temp += c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator-(const Char& c)
	{
		Char temp;
		temp = *this;
		temp -= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator*(const Char& c)
	{
		Char temp;
		temp = *this;
		temp *= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator/(const Char& c)
	{
		Char temp;
		temp = *this;
		temp /= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator%(const Char& c)
	{
		Char temp;
		temp = *this;
		temp %= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator<<(const Char& c)
	{
		Char temp;
		temp = *this;
		temp <<= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator>>(const Char& c)
	{
		Char temp;
		temp = *this;
		temp >>= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator&(const Char& c)
	{
		Char temp;
		temp = *this;
		temp &= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator^(const Char& c)
	{
		Char temp;
		temp = *this;
		temp ^= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator|(const Char& c)
	{
		Char temp;
		temp = *this;
		temp |= c;
		return temp;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator+=(unsigned char c)
	{
		cg->add(*this, c);
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator-=(unsigned char c)
	{
		cg->sub(*this, c);
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator*=(unsigned char c)
	{
		cg->exclude(eax);
		cg->mov(al, *this);
		cg->imul(eax, c);
		cg->mov(*this, al);
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator/=(unsigned char c)
	{
		cg->exclude(eax);
		cg->exclude(edx);
		cg->mov(al, *this);
		cg->mov(dl, c);
		cg->idiv(dl);
		cg->mov(*this, al);
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator%=(unsigned char c)
	{
		cg->exclude(eax);
		cg->exclude(edx);
		cg->mov(al, *this);
		cg->mov(dl, c);
		cg->idiv(dl);
		cg->mov(*this, dl);
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator<<=(unsigned char c)
	{
		cg->exclude(ecx);
		cg->exclude(edx);
		cg->mov(cl, c);
		cg->mov(cl, *this);
		cg->shl(*this, cl);
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator>>=(unsigned char c)
	{
		cg->exclude(ecx);
		cg->exclude(edx);
		cg->mov(cl, c);
		cg->mov(cl, *this);
		cg->shr(*this, cl);
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator&=(unsigned char c)
	{
		cg-> and (*this, c);
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator^=(unsigned char c)
	{
		cg-> xor (*this, c);
		return *this;
	}

	CodeGenerator::Char& CodeGenerator::Char::operator|=(unsigned char c)
	{
		cg-> or (*this, c);
		return *this;
	}

	CodeGenerator::Char CodeGenerator::Char::operator+(unsigned char c)
	{
		Char temp;
		temp = *this;
		temp += c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator-(unsigned char c)
	{
		Char temp;
		temp = *this;
		temp -= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator*(unsigned char c)
	{
		Char temp;
		temp = *this;
		temp *= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator/(unsigned char c)
	{
		Char temp;
		temp = *this;
		temp /= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator%(unsigned char c)
	{
		Char temp;
		temp = *this;
		temp %= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator<<(unsigned char c)
	{
		Char temp;
		temp = *this;
		temp <<= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator>>(unsigned char c)
	{
		Char temp;
		temp = *this;
		temp >>= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator&(unsigned char c)
	{
		Char temp;
		temp = *this;
		temp &= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator^(unsigned char c)
	{
		Char temp;
		temp = *this;
		temp ^= c;
		return temp;
	}

	CodeGenerator::Char CodeGenerator::Char::operator|(unsigned char c)
	{
		Char temp;
		temp = *this;
		temp |= c;
		return temp;
	}

	CodeGenerator::Word::Word() : Variable(2)
	{
	}

	CodeGenerator::Word::operator OperandREG16() const
	{
		return cg->r16(ebp + ref());
	}

	CodeGenerator::Short::Short()
	{
	}

	CodeGenerator::Short::Short(unsigned short s)
	{
		cg->mov(*this, s);
	}

	CodeGenerator::Short::Short(const Short& s)
	{
		cg->mov(*this, s);
	}

	CodeGenerator::Short& CodeGenerator::Short::operator=(const Short& s)
	{
		cg->mov(*this, cg->m16(ebp + s.ref()));
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator+=(const Short& s)
	{
		cg->add(*this, cg->m16(ebp + s.ref()));
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator-=(const Short& s)
	{
		cg->sub(*this, cg->m16(ebp + s.ref()));
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator*=(const Short& s)
	{
		cg->imul(*this, cg->m16(ebp + s.ref()));
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator/=(const Short& s)
	{
		cg->exclude(eax);
		cg->exclude(edx);
		cg->mov(ax, cg->m16(ebp + ref()));
		cg->mov(dx, cg->m16(ebp + s.ref()));
		cg->idiv(dx);
		cg->mov(*this, ax);
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator%=(const Short& s)
	{
		cg->exclude(eax);
		cg->exclude(edx);
		cg->mov(ax, cg->m16(ebp + ref()));
		cg->mov(dx, cg->m16(ebp + s.ref()));
		cg->idiv(dx);
		cg->mov(*this, dx);
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator<<=(const Short& s)
	{
		cg->exclude(ecx);
		cg->mov(cx, cg->m16(ebp + s.ref()));
		cg->shl(*this, cl);
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator>>=(const Short& s)
	{
		cg->exclude(ecx);
		cg->mov(cx, cg->m16(ebp + s.ref()));
		cg->shr(*this, cl);
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator&=(const Short& s)
	{
		cg-> and (*this, cg->m16(ebp + s.ref()));
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator^=(const Short& s)
	{
		cg-> xor (*this, cg->m16(ebp + s.ref()));
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator|=(const Short& s)
	{
		cg-> or (*this, cg->m16(ebp + s.ref()));
		return *this;
	}

	CodeGenerator::Short CodeGenerator::Short::operator+(const Short& s)
	{
		Short temp;
		temp = *this;
		temp += s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator-(const Short& s)
	{
		Short temp;
		temp = *this;
		temp -= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator*(const Short& s)
	{
		Short temp;
		temp = *this;
		temp *= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator/(const Short& s)
	{
		Short temp;
		temp = *this;
		temp /= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator%(const Short& s)
	{
		Short temp;
		temp = *this;
		temp %= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator<<(const Short& s)
	{
		Short temp;
		temp = *this;
		temp <<= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator>>(const Short& s)
	{
		Short temp;
		temp = *this;
		temp >>= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator&(const Short& s)
	{
		Short temp;
		temp = *this;
		temp &= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator^(const Short& s)
	{
		Short temp;
		temp = *this;
		temp ^= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator|(const Short& s)
	{
		Short temp;
		temp = *this;
		temp |= s;
		return temp;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator+=(unsigned short s)
	{
		cg->add(*this, s);
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator-=(unsigned short s)
	{
		cg->sub(*this, s);
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator*=(unsigned short s)
	{
		cg->imul(*this, s);
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator/=(unsigned short s)
	{
		cg->exclude(eax);
		cg->exclude(edx);
		cg->mov(ax, *this);
		cg->mov(dx, s);
		cg->idiv(dx);
		cg->mov(*this, ax);
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator%=(unsigned short s)
	{
		cg->exclude(eax);
		cg->exclude(edx);
		cg->mov(ax, *this);
		cg->mov(dx, s);
		cg->idiv(dx);
		cg->mov(*this, dx);
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator<<=(unsigned short s)
	{
		cg->exclude(ecx);
		cg->exclude(edx);
		cg->mov(cx, s);
		cg->mov(dx, *this);
		cg->shl(*this, cl);
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator>>=(unsigned short s)
	{
		cg->exclude(ecx);
		cg->exclude(edx);
		cg->mov(cx, s);
		cg->mov(dx, *this);
		cg->shr(*this, cl);
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator&=(unsigned short s)
	{
		cg-> and (*this, s);
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator^=(unsigned short s)
	{
		cg-> xor (*this, s);
		return *this;
	}

	CodeGenerator::Short& CodeGenerator::Short::operator|=(unsigned short s)
	{
		cg-> or (*this, s);
		return *this;
	}

	CodeGenerator::Short CodeGenerator::Short::operator+(unsigned short s)
	{
		Short temp;
		temp = *this;
		temp += s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator-(unsigned short s)
	{
		Short temp;
		temp = *this;
		temp -= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator*(unsigned short s)
	{
		Short temp;
		temp = *this;
		temp *= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator/(unsigned short s)
	{
		Short temp;
		temp = *this;
		temp /= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator%(unsigned short s)
	{
		Short temp;
		temp = *this;
		temp %= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator<<(unsigned short s)
	{
		Short temp;
		temp = *this;
		temp <<= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator>>(unsigned short s)
	{
		Short temp;
		temp = *this;
		temp >>= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator&(unsigned short s)
	{
		Short temp;
		temp = *this;
		temp &= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator^(unsigned short s)
	{
		Short temp;
		temp = *this;
		temp ^= s;
		return temp;
	}

	CodeGenerator::Short CodeGenerator::Short::operator|(unsigned short s)
	{
		Short temp;
		temp = *this;
		temp |= s;
		return temp;
	}

	CodeGenerator::Dword::Dword() : Variable(4)
	{
	}

	CodeGenerator::Dword::operator OperandREG32() const
	{
		return cg->r32(ebp + ref());
	}

	CodeGenerator::Int::Int()
	{
	}

	CodeGenerator::Int::Int(unsigned int i)
	{
		cg->mov(*this, i);
	}

	CodeGenerator::Int::Int(const Int& i)
	{
		cg->mov(*this, i);
	}

	CodeGenerator::Int& CodeGenerator::Int::operator=(const Int& i)
	{
		cg->mov(*this, cg->m32(ebp + i.ref()));
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator+=(const Int& i)
	{
		cg->add(*this, cg->m32(ebp + i.ref()));
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator-=(const Int& i)
	{
		cg->sub(*this, cg->m32(ebp + i.ref()));
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator*=(const Int& i)
	{
		cg->imul(*this, cg->m32(ebp + i.ref()));
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator/=(const Int& i)
	{
		cg->exclude(eax);
		cg->exclude(edx);
		cg->mov(eax, cg->m32(ebp + ref()));
		cg->mov(edx, cg->m32(ebp + i.ref()));
		cg->idiv(edx);
		cg->mov(*this, eax);
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator%=(const Int& i)
	{
		cg->exclude(eax);
		cg->exclude(edx);
		cg->mov(eax, cg->m32(ebp + ref()));
		cg->mov(edx, cg->m32(ebp + i.ref()));
		cg->idiv(edx);
		cg->mov(*this, edx);
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator<<=(const Int& i)
	{
		cg->exclude(ecx);
		cg->mov(ecx, cg->m32(ebp + ref()));
		cg->shl(*this, cl);
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator>>=(const Int& i)
	{
		cg->exclude(ecx);
		cg->mov(ecx, cg->m32(ebp + ref()));
		cg->shr(*this, cl);
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator&=(const Int& i)
	{
		cg-> and (*this, cg->m32(ebp + i.ref()));
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator^=(const Int& i)
	{
		cg-> xor (*this, cg->m32(ebp + i.ref()));
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator|=(const Int& i)
	{
		cg-> or (*this, cg->m32(ebp + i.ref()));
		return *this;
	}

	CodeGenerator::Int CodeGenerator::Int::operator+(const Int& i)
	{
		Int temp;
		temp = *this;
		temp += i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator-(const Int& i)
	{
		Int temp;
		temp = *this;
		temp -= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator*(const Int& i)
	{
		Int temp;
		temp = *this;
		temp *= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator/(const Int& i)
	{
		Int temp;
		temp = *this;
		temp /= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator%(const Int& i)
	{
		Int temp;
		temp = *this;
		temp %= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator<<(const Int& i)
	{
		Int temp;
		temp = *this;
		temp <<= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator>>(const Int& i)
	{
		Int temp;
		temp = *this;
		temp >>= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator&(const Int& i)
	{
		Int temp;
		temp = *this;
		temp &= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator^(const Int& i)
	{
		Int temp;
		temp = *this;
		temp ^= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator|(const Int& i)
	{
		Int temp;
		temp = *this;
		temp |= i;
		return temp;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator+=(unsigned int i)
	{
		cg->add(*this, i);
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator-=(unsigned int i)
	{
		cg->sub(*this, i);
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator*=(unsigned int i)
	{
		cg->imul(*this, i);
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator/=(unsigned int i)
	{
		cg->exclude(eax);
		cg->exclude(edx);
		cg->mov(eax, *this);
		cg->mov(edx, i);
		cg->idiv(edx);
		cg->mov(*this, eax);
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator%=(unsigned int i)
	{
		cg->exclude(eax);
		cg->exclude(edx);
		cg->mov(eax, *this);
		cg->mov(edx, i);
		cg->idiv(edx);
		cg->mov(*this, edx);
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator<<=(unsigned int i)
	{
		cg->exclude(ecx);
		cg->exclude(edx);
		cg->mov(ecx, i);
		cg->mov(edx, *this);
		cg->shl(*this, cl);
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator>>=(unsigned int i)
	{
		cg->exclude(ecx);
		cg->exclude(edx);
		cg->mov(ecx, i);
		cg->mov(edx, *this);
		cg->shr(*this, cl);
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator&=(unsigned int i)
	{
		cg-> and (*this, i);
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator^=(unsigned int i)
	{
		cg-> xor (*this, i);
		return *this;
	}

	CodeGenerator::Int& CodeGenerator::Int::operator|=(unsigned int i)
	{
		cg-> or (*this, i);
		return *this;
	}

	CodeGenerator::Int CodeGenerator::Int::operator+(unsigned int i)
	{
		Int temp;
		temp = *this;
		temp += i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator-(unsigned int i)
	{
		Int temp;
		temp = *this;
		temp -= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator*(unsigned int i)
	{
		Int temp;
		temp = *this;
		temp *= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator/(unsigned int i)
	{
		Int temp;
		temp = *this;
		temp /= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator%(unsigned int i)
	{
		Int temp;
		temp = *this;
		temp %= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator<<(unsigned int i)
	{
		Int temp;
		temp = *this;
		temp <<= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator>>(unsigned int i)
	{
		Int temp;
		temp = *this;
		temp >>= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator&(unsigned int i)
	{
		Int temp;
		temp = *this;
		temp &= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator^(unsigned int i)
	{
		Int temp;
		temp = *this;
		temp ^= i;
		return temp;
	}

	CodeGenerator::Int CodeGenerator::Int::operator|(unsigned int i)
	{
		Int temp;
		temp = *this;
		temp |= i;
		return temp;
	}

	CodeGenerator::Qword::Qword() : Variable(8)
	{
	}

	CodeGenerator::Qword::Qword(const Qword& qword) : Variable(8)
	{
		cg->movq(*this, qword);
	}

	CodeGenerator::Qword::operator OperandMMREG() const
	{
		return cg->r64(ebp + ref());
	}

	CodeGenerator::Qword& CodeGenerator::Qword::operator=(const Qword& qword)
	{
		cg->movq(*this, cg->m64(ebp + qword.ref()));
		return *this;
	}

	CodeGenerator::Qword& CodeGenerator::Qword::operator+=(const Qword& qword)
	{
		cg->paddq(*this, cg->m64(ebp + qword.ref()));
		return *this;
	}

	CodeGenerator::Qword& CodeGenerator::Qword::operator-=(const Qword& qword)
	{
		cg->psubq(*this, cg->m64(ebp + qword.ref()));
		return *this;
	}

	CodeGenerator::Qword& CodeGenerator::Qword::operator<<=(const Qword& qword)
	{
		cg->psllq(*this, cg->m64(ebp + qword.ref()));
		return *this;
	}

	CodeGenerator::Qword& CodeGenerator::Qword::operator&=(const Qword& qword)
	{
		cg->pand(*this, cg->m64(ebp + qword.ref()));
		return *this;
	}

	CodeGenerator::Qword& CodeGenerator::Qword::operator^=(const Qword& qword)
	{
		cg->pxor(*this, cg->m64(ebp + qword.ref()));
		return *this;
	}

	CodeGenerator::Qword& CodeGenerator::Qword::operator|=(const Qword& qword)
	{
		cg->por(*this, cg->m64(ebp + qword.ref()));
		return *this;
	}

	CodeGenerator::Qword CodeGenerator::Qword::operator+(const Qword& qword)
	{
		Qword temp;
		temp = *this;
		temp += qword;
		return temp;
	}

	CodeGenerator::Qword CodeGenerator::Qword::operator-(const Qword& qword)
	{
		Qword temp;
		temp = *this;
		temp -= qword;
		return temp;
	}

	CodeGenerator::Qword CodeGenerator::Qword::operator<<(const Qword& qword)
	{
		Qword temp;
		temp = *this;
		temp <<= qword;
		return temp;
	}

	CodeGenerator::Qword CodeGenerator::Qword::operator&(const Qword& qword)
	{
		Qword temp;
		temp = *this;
		temp &= qword;
		return temp;
	}

	CodeGenerator::Qword CodeGenerator::Qword::operator^(const Qword& qword)
	{
		Qword temp;
		temp = *this;
		temp ^= qword;
		return temp;
	}

	CodeGenerator::Qword CodeGenerator::Qword::operator|(const Qword& qword)
	{
		Qword temp;
		temp = *this;
		temp |= qword;
		return temp;
	}

	CodeGenerator::Qword& CodeGenerator::Qword::operator<<=(char imm)
	{
		cg->psllq(*this, imm);
		return *this;
	}

	CodeGenerator::Qword CodeGenerator::Qword::operator<<(char imm)
	{
		Qword temp;
		temp = *this;
		temp <<= imm;
		return temp;
	}

	CodeGenerator::Word4::Word4()
	{
	}

	CodeGenerator::Word4::Word4(const Word4& word4)
	{
		cg->movq(*this, word4);
	}

	CodeGenerator::Word4::operator OperandMMREG() const
	{
		return cg->r64(ebp + ref());
	}

	CodeGenerator::Word4& CodeGenerator::Word4::operator=(const Word4& word4)
	{
		cg->movq(*this, cg->m64(ebp + word4.ref()));
		return *this;
	}

	CodeGenerator::Word4& CodeGenerator::Word4::operator+=(const Word4& word4)
	{
		cg->paddw(*this, cg->m64(ebp + word4.ref()));
		return *this;
	}

	CodeGenerator::Word4& CodeGenerator::Word4::operator-=(const Word4& word4)
	{
		cg->psubw(*this, cg->m64(ebp + word4.ref()));
		return *this;
	}

	CodeGenerator::Word4& CodeGenerator::Word4::operator<<=(const Qword& qword)
	{
		cg->psllw(*this, cg->m64(ebp + qword.ref()));
		return *this;
	}

	CodeGenerator::Word4& CodeGenerator::Word4::operator>>=(const Qword& qword)
	{
		cg->psraw(*this, cg->m64(ebp + qword.ref()));
		return *this;
	}

	CodeGenerator::Word4& CodeGenerator::Word4::operator&=(const Word4& word4)
	{
		cg->pand(*this, cg->m64(ebp + word4.ref()));
		return *this;
	}

	CodeGenerator::Word4& CodeGenerator::Word4::operator^=(const Word4& word4)
	{
		cg->pxor(*this, cg->m64(ebp + word4.ref()));
		return *this;
	}

	CodeGenerator::Word4& CodeGenerator::Word4::operator|=(const Word4& word4)
	{
		cg->por(*this, cg->m64(ebp + word4.ref()));
		return *this;
	}

	CodeGenerator::Word4 CodeGenerator::Word4::operator+(const Word4& word4)
	{
		Word4 temp;
		temp = *this;
		temp += word4;
		return temp;
	}

	CodeGenerator::Word4 CodeGenerator::Word4::operator-(const Word4& word4)
	{
		Word4 temp;
		temp = *this;
		temp -= word4;
		return temp;
	}

	CodeGenerator::Word4 CodeGenerator::Word4::operator<<(const Qword& qword)
	{
		Word4 temp;
		temp = *this;
		temp <<= qword;
		return temp;
	}

	CodeGenerator::Word4 CodeGenerator::Word4::operator>>(const Qword& qword)
	{
		Word4 temp;
		temp = *this;
		temp >>= qword;
		return temp;
	}

	CodeGenerator::Word4 CodeGenerator::Word4::operator&(const Word4& word4)
	{
		Word4 temp;
		temp = *this;
		temp &= word4;
		return temp;
	}

	CodeGenerator::Word4 CodeGenerator::Word4::operator^(const Word4& word4)
	{
		Word4 temp;
		temp = *this;
		temp ^= word4;
		return temp;
	}

	CodeGenerator::Word4 CodeGenerator::Word4::operator|(const Word4& word4)
	{
		Word4 temp;
		temp = *this;
		temp |= word4;
		return temp;
	}

	CodeGenerator::Word4& CodeGenerator::Word4::operator<<=(char imm)
	{
		cg->psllw(*this, imm);
		return *this;
	}

	CodeGenerator::Word4& CodeGenerator::Word4::operator>>=(char imm)
	{
		cg->psraw(*this, imm);
		return *this;
	}

	CodeGenerator::Word4 CodeGenerator::Word4::operator<<(char imm)
	{
		Word4 temp;
		temp = *this;
		temp <<= imm;
		return temp;
	}

	CodeGenerator::Word4 CodeGenerator::Word4::operator>>(char imm)
	{
		Word4 temp;
		temp = *this;
		temp >>= imm;
		return temp;
	}

	CodeGenerator::Dword2::Dword2()
	{
	}

	CodeGenerator::Dword2::Dword2(const Dword2& dword2)
	{
		cg->movq(*this, dword2);
	}

	CodeGenerator::Dword2::operator OperandMMREG() const
	{
		return cg->r64(ebp + ref());
	}

	CodeGenerator::Dword2& CodeGenerator::Dword2::operator=(const Dword2& dword2)
	{
		cg->movq(*this, cg->m64(ebp + dword2.ref()));
		return *this;
	}

	CodeGenerator::Dword2& CodeGenerator::Dword2::operator+=(const Dword2& dword2)
	{
		cg->paddd(*this, cg->m64(ebp + dword2.ref()));
		return *this;
	}

	CodeGenerator::Dword2& CodeGenerator::Dword2::operator-=(const Dword2& dword2)
	{
		cg->psubd(*this, cg->m64(ebp + dword2.ref()));
		return *this;
	}

	CodeGenerator::Dword2& CodeGenerator::Dword2::operator<<=(const Qword& qword)
	{
		cg->pslld(*this, cg->m64(ebp + qword.ref()));
		return *this;
	}

	CodeGenerator::Dword2& CodeGenerator::Dword2::operator>>=(const Qword& qword)
	{
		cg->psrad(*this, cg->m64(ebp + qword.ref()));
		return *this;
	}

	CodeGenerator::Dword2& CodeGenerator::Dword2::operator&=(const Dword2& dword2)
	{
		cg->pand(*this, cg->m64(ebp + dword2.ref()));
		return *this;
	}

	CodeGenerator::Dword2& CodeGenerator::Dword2::operator^=(const Dword2& dword2)
	{
		cg->pxor(*this, cg->m64(ebp + dword2.ref()));
		return *this;
	}

	CodeGenerator::Dword2& CodeGenerator::Dword2::operator|=(const Dword2& dword2)
	{
		cg->por(*this, cg->m64(ebp + dword2.ref()));
		return *this;
	}

	CodeGenerator::Dword2 CodeGenerator::Dword2::operator+(const Dword2& dword2)
	{
		Dword2 temp;
		temp = *this;
		temp += dword2;
		return temp;
	}

	CodeGenerator::Dword2 CodeGenerator::Dword2::operator-(const Dword2& dword2)
	{
		Dword2 temp;
		temp = *this;
		temp -= dword2;
		return temp;
	}

	CodeGenerator::Dword2 CodeGenerator::Dword2::operator<<(const Qword& qword)
	{
		Dword2 temp;
		temp = *this;
		temp <<= qword;
		return temp;
	}

	CodeGenerator::Dword2 CodeGenerator::Dword2::operator>>(const Qword& qword)
	{
		Dword2 temp;
		temp = *this;
		temp >>= qword;
		return temp;
	}

	CodeGenerator::Dword2 CodeGenerator::Dword2::operator&(const Dword2& dword2)
	{
		Dword2 temp;
		temp = *this;
		temp &= dword2;
		return temp;
	}

	CodeGenerator::Dword2 CodeGenerator::Dword2::operator^(const Dword2& dword2)
	{
		Dword2 temp;
		temp = *this;
		temp ^= dword2;
		return temp;
	}

	CodeGenerator::Dword2 CodeGenerator::Dword2::operator|(const Dword2& dword2)
	{
		Dword2 temp;
		temp = *this;
		temp |= dword2;
		return temp;
	}

	CodeGenerator::Dword2& CodeGenerator::Dword2::operator<<=(char imm)
	{
		cg->pslld(*this, imm);
		return *this;
	}

	CodeGenerator::Dword2& CodeGenerator::Dword2::operator>>=(char imm)
	{
		cg->psrad(*this, imm);
		return *this;
	}

	CodeGenerator::Dword2 CodeGenerator::Dword2::operator<<(char imm)
	{
		Dword2 temp;
		temp = *this;
		temp <<= imm;
		return temp;
	}

	CodeGenerator::Dword2 CodeGenerator::Dword2::operator>>(char imm)
	{
		Dword2 temp;
		temp = *this;
		temp >>= imm;
		return temp;
	}

	CodeGenerator::Float::Float() : Variable(4)
	{
	}

	CodeGenerator::Float::Float(const Float& f) : Variable(4)
	{
		cg->movss(*this, f);
	}

	CodeGenerator::Float::operator OperandXMMREG() const
	{
		return cg->rSS(ebp + ref());
	}

	CodeGenerator::Float& CodeGenerator::Float::operator=(const Float& f)
	{
		cg->movss(*this, cg->mSS(ebp + f.ref()));
		return *this;
	}

	CodeGenerator::Float& CodeGenerator::Float::operator+=(const Float& f)
	{
		cg->movss(*this, cg->mSS(ebp + f.ref()));
		return *this;
	}

	CodeGenerator::Float& CodeGenerator::Float::operator-=(const Float& f)
	{
		cg->subss(*this, cg->mSS(ebp + f.ref()));
		return *this;
	}

	CodeGenerator::Float& CodeGenerator::Float::operator*=(const Float& f)
	{
		cg->mulss(*this, cg->mSS(ebp + f.ref()));
		return *this;
	}

	CodeGenerator::Float& CodeGenerator::Float::operator/=(const Float& f)
	{
		cg->divss(*this, cg->mSS(ebp + f.ref()));
		return *this;
	}

	CodeGenerator::Float CodeGenerator::Float::operator+(const Float& f)
	{
		Float temp;
		temp = *this;
		temp += f;
		return temp;
	}

	CodeGenerator::Float CodeGenerator::Float::operator-(const Float& f)
	{
		Float temp;
		temp = *this;
		temp -= f;
		return temp;
	}

	CodeGenerator::Float CodeGenerator::Float::operator*(const Float& f)
	{
		Float temp;
		temp = *this;
		temp *= f;
		return temp;
	}

	CodeGenerator::Float CodeGenerator::Float::operator/(const Float& f)
	{
		Float temp;
		temp = *this;
		temp /= f;
		return temp;
	}

	CodeGenerator::Xword::Xword() : Variable(16)
	{
	}

	CodeGenerator::Xword::operator OperandXMMREG() const
	{
		return cg->r128(ebp + ref());
	}

	CodeGenerator::Float4::Float4()
	{
	}

	CodeGenerator::Float4::Float4(const Float4& float4)
	{
		cg->movaps(*this, float4);
	}

	CodeGenerator::Float4::Float4(const Float& f)
	{
		cg->movss(*this, f);
		cg->shufps(*this, *this, 0x00);
	}

	CodeGenerator::Float4& CodeGenerator::Float4::operator=(const Float4& float4)
	{
		cg->movaps(*this, cg->m128(ebp + float4.ref()));
		return *this;
	}

	CodeGenerator::Float4& CodeGenerator::Float4::operator+=(const Float4& float4)
	{
		cg->addps(*this, cg->m128(ebp + float4.ref()));
		return *this;
	}

	CodeGenerator::Float4& CodeGenerator::Float4::operator-=(const Float4& float4)
	{
		cg->subps(*this, cg->m128(ebp + float4.ref()));
		return *this;
	}

	CodeGenerator::Float4& CodeGenerator::Float4::operator*=(const Float4& float4)
	{
		cg->mulps(*this, cg->m128(ebp + float4.ref()));
		return *this;
	}

	CodeGenerator::Float4& CodeGenerator::Float4::operator/=(const Float4& float4)
	{
		cg->divps(*this, cg->m128(ebp + float4.ref()));
		return *this;
	}

	CodeGenerator::Float4& CodeGenerator::Float4::operator&=(const Float4& float4)
	{
		cg->andps(*this, cg->m128(ebp + float4.ref()));
		return *this;
	}

	CodeGenerator::Float4& CodeGenerator::Float4::operator^=(const Float4& float4)
	{
		cg->xorps(*this, cg->m128(ebp + float4.ref()));
		return *this;
	}

	CodeGenerator::Float4& CodeGenerator::Float4::operator|=(const Float4& float4)
	{
		cg->orps(*this, cg->m128(ebp + float4.ref()));
		return *this;
	}

	CodeGenerator::Float4 CodeGenerator::Float4::operator+(const Float4& float4)
	{
		Float4 temp;
		temp = *this;
		temp += float4;
		return temp;
	}

	CodeGenerator::Float4 CodeGenerator::Float4::operator-(const Float4& float4)
	{
		Float4 temp;
		temp = *this;
		temp -= float4;
		return temp;
	}

	CodeGenerator::Float4 CodeGenerator::Float4::operator*(const Float4& float4)
	{
		Float4 temp;
		temp = *this;
		temp *= float4;
		return temp;
	}

	CodeGenerator::Float4 CodeGenerator::Float4::operator/(const Float4& float4)
	{
		Float4 temp;
		temp = *this;
		temp /= float4;
		return temp;
	}

	CodeGenerator::Float4 CodeGenerator::Float4::operator&(const Float4& float4)
	{
		Float4 temp;
		temp = *this;
		temp &= float4;
		return temp;
	}

	CodeGenerator::Float4 CodeGenerator::Float4::operator^(const Float4& float4)
	{
		Float4 temp;
		temp = *this;
		temp ^= float4;
		return temp;
	}

	CodeGenerator::Float4 CodeGenerator::Float4::operator|(const Float4& float4)
	{
		Float4 temp;
		temp = *this;
		temp |= float4;
		return temp;
	}

	CodeGenerator::CodeGenerator(bool x64) : Emulator(x64)
	{
		cg = this;
	}

	CodeGenerator::~CodeGenerator()
	{
		// Reset stack
		stack = -128;
		stackTop = -128;
		stackUpdate = 0;
	}

	void CodeGenerator::prologue(int functionArguments)
	{
		cg = this;

		if (!x64)
		{
			mov(arg, esp);

			push(edi);
			push(esi);
			push(ebx);

			push(ebp);
			mov(ebp, esp);
			stackUpdate =
				sub(ebp, stackTop);
			lea(esp, dword_ptr[ebp - 128 - 12]);
			and (ebp, 0xFFFFFFF0);
		}
		else
		{
			push(rbp);
			push(rbx);
			push(r12);
			push(r13);
			push(r14);
			push(r15);
			stackUpdate =
				sub(rsp, 32 + stackTop + 128);
		}
	};

	OperandMEM32 CodeGenerator::argument(int i)
	{
		return dword_ptr[arg + 4 * i + 4];
	}

	void CodeGenerator::epilogue()
	{
		cg = this;

		if (!x64)
		{
			add(esp, stackTop + 128 + 12);
			pop(ebp);

			pop(ebx);
			pop(esi);
			pop(edi);
		}
		else
		{
			add(rsp, 32 + stackTop + 128);
			pop(r15);
			pop(r14);
			pop(r13);
			pop(r12);
			pop(rbx);
			pop(rbp);
		}

		ret();
	}

	void CodeGenerator::free(Variable& var1)
	{
		var1.free();
	}

	void CodeGenerator::free(Variable& var1, Variable& var2)
	{
		var1.free();
		var2.free();
	}

	void CodeGenerator::free(Variable& var1, Variable& var2, Variable& var3)
	{
		var1.free();
		var2.free();
		var3.free();
	}

	void CodeGenerator::free(Variable& var1, Variable& var2, Variable& var3, Variable& var4)
	{
		var1.free();
		var2.free();
		var3.free();
		var4.free();
	}

	void CodeGenerator::free(Variable& var1, Variable& var2, Variable& var3, Variable& var4, Variable& var5)
	{
		var1.free();
		var2.free();
		var3.free();
		var4.free();
		var5.free();
	}
}

#endif