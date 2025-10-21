#pragma once

#include <libfrei0r/frei0r.h>

typedef struct Frei0rContext {
	f0r_init_f init ;
	f0r_get_plugin_info_f get_plugin_info;
	f0r_update_f update;
	f0r_update2_f update2;
	f0r_instance_t instance;
	f0r_plugin_info_t plugin_info;
	f0r_get_param_info_f  get_param_info;
	f0r_get_param_value_f get_param_value;
	f0r_set_param_value_f set_param_value;
	f0r_construct_f       construct;
	f0r_destruct_f        destruct;
	f0r_deinit_f          deinit;

	char *dl_name;
	char *params;
	int w, h;
	uint64_t pts;
} Frei0rContext;

int frei0r_init(Frei0rContext* context);
int frei0r_set_param(Frei0rContext* s, f0r_param_info_t info, int index, char* param);
int frei0r_set_params(Frei0rContext* s, const char* params);