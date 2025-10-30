
#include "Utils.hpp"
#include <avisynth/avisynth.h>

EXTERN_C{
#include "fre.h"
}

using base = GenericVideoFilter;

class frewrapper : public GenericVideoFilter {
public:
	explicit frewrapper(const PClip& _child, const char* filter_name, const char* filter_params, int _startframe, int _endframe, IScriptEnvironment* env);

	virtual ~frewrapper() override;
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;


	std::string m_instancekey = "";//查找Key
	std::string m_filter_name = "";//滤镜名
	std::string m_filter_params = "";//参数
	int m_StartFrame = 0;
	int m_EndFrame = 0;
};



class FreInstance
{
	Frei0rContext* m_pFrei0rContext = nullptr;
public:
	BOOL IsInit() {
		return !!m_pFrei0rContext;
	}
	FreInstance(const char* filter_name,
		const char* filter_params, 
		int width, int height){
		m_pFrei0rContext = new  Frei0rContext();
		m_pFrei0rContext->dl_name = _strdup(filter_name);
		m_pFrei0rContext->params = _strdup(filter_params);
		frei0r_init(m_pFrei0rContext);
		if (m_pFrei0rContext->construct == NULL) { //库加载失败!
			delete m_pFrei0rContext;
			m_pFrei0rContext = NULL;
			return;
		}
		m_pFrei0rContext->instance = m_pFrei0rContext->construct(width, height);
		frei0r_set_params(m_pFrei0rContext, m_pFrei0rContext->params);
	}
	virtual ~FreInstance() {
		if (m_pFrei0rContext) {
			if(m_pFrei0rContext->instance)
				m_pFrei0rContext->destruct(m_pFrei0rContext->instance);
			delete m_pFrei0rContext;
			m_pFrei0rContext = NULL;
		}
	}

	void Update(int n, const uint8_t* src, uint8_t* dst) {
		if(m_pFrei0rContext)
		m_pFrei0rContext->update(m_pFrei0rContext->instance, (double)(n + 1), (uint32_t*)src, (uint32_t*)dst);
	}
};

static cache::lru_cache<std::string, std::shared_ptr<FreInstance>>g_cacheFreInst(3);

AVSValue __cdecl Create_Frei0r(AVSValue args, void* user_data, IScriptEnvironment* env) {

	if (strcmp(args[1].AsString(""), "nervous") == 0
		|| strcmp(args[1].AsString(""), "IIRblur") == 0
		)
	{
		return args[0];
	}
	return new frewrapper(args[0].AsClip(),
		args[1].AsString(""), args[2].AsString(""), args[3].AsInt(), args[4].AsInt(),
		env);
}
AVSValue __cdecl Create_Frei0r1(AVSValue args, void* user_data, IScriptEnvironment* env) {
	if (strcmp(args[1].AsString(""), "nervous") == 0
		|| strcmp(args[1].AsString(""), "IIRblur") == 0)
	{
		return args[0];
	}
	return new frewrapper(args[0].AsClip(),
		args[1].AsString(""), args[2].AsString(""), 0, 0,
		env);
}

frewrapper::frewrapper(const PClip &_child, 
	const char* filter_name, 
	const char* filter_params, 
	int _startframe, 
	int _endframe, 
	IScriptEnvironment *env) :
	base{ _child,__FUNCTION__ }, 
	m_StartFrame{ _startframe },
	m_EndFrame{_endframe}
{
	memset(&vi, 0, sizeof vi);
	m_filter_name = strdup( filter_name);
	m_filter_params = filter_params;
	vi=_child->GetVideoInfo();
	m_instancekey = m_filter_name + std::to_string(vi.width) + "x" + std::to_string(vi.height);
}

frewrapper::~frewrapper() {}

PVideoFrame frewrapper::GetFrame(int n, IScriptEnvironment *env) {
	PVideoFrame FilterFrame = child->GetFrame(n, env);

	if (m_EndFrame>0){
		if (n> m_EndFrame|| n< m_StartFrame){
			return FilterFrame;
		}
	}
	
	if (!g_cacheFreInst.exists(m_instancekey)){
		FreInstance* inst = new FreInstance(m_filter_name.c_str(), m_filter_params.c_str(), vi.width, vi.height);
		if (inst->IsInit()) {
			std::shared_ptr<FreInstance>obj = std::shared_ptr<FreInstance>(inst);
			g_cacheFreInst.put(m_instancekey, obj);
		}
		else {
			return NULL;//滤镜添加失败
		}
	}
	FreInstance* instt = g_cacheFreInst.get(m_instancekey).get();
	PVideoFrame dst = env->NewVideoFrame(vi);
	if (dst == nullptr || dst.m_ptr == nullptr) {
		return nullptr;
	}
	instt->Update(n+1, FilterFrame->GetReadPtr(), dst->GetWritePtr()); //滤镜处理
	return dst;
}
