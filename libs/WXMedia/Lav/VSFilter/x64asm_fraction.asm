%if  __BITS__ == 64


		global	VDFractionScale64
VDFractionScale64:
		mov		rax, rcx
		mul		rdx
		div		r8
		mov		[r9], edx
		ret

;--------------------------------------------------------------------------
; VDUMulDiv64x32(
;		[rcx] uint64 a,
;		[rdx] uint64 b,
;		[r8]  uint64 c);
;					
;
		global	VDUMulDiv64x32
VDUMulDiv64x32:
		mov		rax, rcx
		mul		rdx
		div		r8
		ret

		end


%endif