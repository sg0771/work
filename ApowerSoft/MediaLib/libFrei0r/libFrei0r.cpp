#include <libfrei0r/frei0r.h>
#include <string.h>
#include <vector>

static std::vector<f0r_plugin_info_t*>s_arrFeri0r;
extern "C" void Feri0r_Set(f0r_plugin_info_t * info) {
	s_arrFeri0r.push_back(info);
}


extern "C" void _distort0r_init();
extern "C" void _G_init();
extern "C" void _glow_init();
extern "C" void _IIRblur_init();
extern "C" void _nervous_init();
extern "C" void _rgbnoise_init();
extern "C" void _vertigo_init();
extern "C" void _vignette_init();

static void Feri0r_Init() {
	static int s_bInit = 0;
	if (s_bInit == 0) {
		s_bInit = 1;
		//Filter Init
		_distort0r_init();
		_G_init();
		_glow_init();
		_IIRblur_init();
		_nervous_init();
		_rgbnoise_init();
		_vertigo_init();
		_vignette_init();
	}
}

extern "C"  f0r_plugin_info_t* Feri0r_Get(const char* name) {
	Feri0r_Init();
	for (int i = 0; i < s_arrFeri0r.size(); i++) {
		if (stricmp(s_arrFeri0r[i]->name, name) == 0) {
			return s_arrFeri0r[i];
		}
	}
	return NULL;
}