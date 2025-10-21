
//#include <cmath>
#include "fre.h"

#include <Windows.h>
#include <libavutil/avutil.h>
#include <libavutil/avstring.h>
#include <wxlog.h>

#ifndef _HUGE_ENUF
#define _HUGE_ENUF  1e+300  // _HUGE_ENUF*_HUGE_ENUF must overflow
#endif

#define INFINITY   ((float)(_HUGE_ENUF * _HUGE_ENUF))
#define HUGE_VAL   ((double)INFINITY)

int frei0r_set_param(Frei0rContext *s, f0r_param_info_t info, int index, char *param)
{
	union {
		double d;
		f0r_param_color_t col;
		f0r_param_position_t pos;
	} val;
	char *tail;
	//uint8_t rgba[4];

	switch (info.type) {
	case F0R_PARAM_BOOL:
		if (!strcmp(param, "y")) val.d = 1.0;
		else if (!strcmp(param, "n")) val.d = 0.0;
		else goto fail;
		break;

	case F0R_PARAM_DOUBLE:
		val.d = strtod(param, &tail);
		if (*tail || val.d == HUGE_VAL)
			goto fail;
		break;

		//case F0R_PARAM_COLOR:
		//	if (sscanf(param, "%f/%f/%f", &val.col.r, &val.col.g, &val.col.b) != 3) {
		//		
		//		if (av_parse_color(rgba, param, -1, ctx) < 0)
		//			goto fail;
		//		val.col.r = rgba[0] / 255.0;
		//		val.col.g = rgba[1] / 255.0;
		//		val.col.b = rgba[2] / 255.0;
		//	}
		//	break;

	case F0R_PARAM_POSITION:
		if (sscanf(param, "%lf/%lf", &val.pos.x, &val.pos.y) != 2)
			goto fail;
		break;
	}

	s->set_param_value(s->instance, &val, index);
	return 0;

fail:
	WXLogA("Invalid value '%s' for parameter '%s'.\n",
		param, info.name);
	return AVERROR(EINVAL);
}

int frei0r_set_params(Frei0rContext *s, const char *params)
{
	int i;

	if (!params)
		return 0;

	for (i = 0; i < s->plugin_info.num_params; i++) {
		f0r_param_info_t info;
		char *param = NULL;
		int ret;

		s->get_param_info(&info, i);

		if (*params) {
			if (!(param = av_get_token(&params, "|")))
				return AVERROR(ENOMEM);
			if (*params)
				params++;               /* skip ':' */
			ret = frei0r_set_param(s, info, i, param);
			av_free(param);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int frei0r_init_impl(Frei0rContext *s, const char *filter_name, int type)
{
	if (!filter_name) {
		//空名字
		WXLogA("frei0r_init_impl No filter name provided.\n");
		return AVERROR(EINVAL);
	}

	f0r_plugin_info_t* pi = Feri0r_Get(filter_name);
	if (pi == NULL) {
		//没有对应名字滤镜
		WXLogA("Could not find module '%s'.\n", filter_name);
		return -1;
	}
	s->init = pi->f_init;
	s->get_plugin_info = pi->f_get_plugin_info;
	s->get_param_info = pi->f_get_param_info;
	s->get_param_value = pi->f_get_param_value;
	s->set_param_value = pi->f_set_param_value;
	s->construct = pi->f_construct;
	s->destruct = pi->f_destruct;
	s->deinit = pi->f_deinit;
	s->update2 = pi->f_update2;
	s->update = pi->f_update;
	if (s->init) {
		if (s->init() < 0) {
			//滤镜初始化失败
			WXLogA("Could not init the frei0r module.\n");
			return AVERROR(EINVAL);
		}
	}
	else {
		WXLogA("Could not init the frei0r module.\n");
		return AVERROR(EINVAL);
	}

	if (pi->plugin_type != type) {
		//滤镜类型不对
		WXLogA("Invalid type '%s' for this plugin\n",
			pi->plugin_type == F0R_PLUGIN_TYPE_FILTER ? "filter" :
			pi->plugin_type == F0R_PLUGIN_TYPE_SOURCE ? "source" :
			pi->plugin_type == F0R_PLUGIN_TYPE_MIXER2 ? "mixer2" :
			pi->plugin_type == F0R_PLUGIN_TYPE_MIXER3 ? "mixer3" : "unknown");
		return AVERROR(EINVAL);
	}

	////滤镜信息
	//Log_debug(
	//	"name:%s author:'%s' explanation:'%s' color_model:%s "
	//	"frei0r_version:%d version:%d.%d num_params:%d\n",
	//	pi->name, pi->author, pi->explanation,
	//	pi->color_model == F0R_COLOR_MODEL_BGRA8888 ? "bgra8888" :
	//	pi->color_model == F0R_COLOR_MODEL_RGBA8888 ? "rgba8888" :
	//	pi->color_model == F0R_COLOR_MODEL_PACKED32 ? "packed32" : "unknown",
	//	pi->frei0r_version, pi->major_version, pi->minor_version, pi->num_params);

	return 0;
}

int frei0r_init(Frei0rContext* context)
{
	return frei0r_init_impl(context, context->dl_name, F0R_PLUGIN_TYPE_FILTER);
}

