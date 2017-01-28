#include <stdio.h>
#include <assert.h>

#include "libretro.h"

#include "retrocore.h"
#include "retrocore_internal.h"
#include "retrocore_env.h"

bool intcore_retro_environment(retrocore_t *pCore,unsigned cmd, void *data) {
	//printf("%s: cmd=%u, data=%p\n",__func__,cmd,data);

	switch(cmd) {
		case RETRO_ENVIRONMENT_GET_OVERSCAN: // 2
			{
				bool *b = (bool*)data;
				*b = false;
				return true;
			}
			break;
		case RETRO_ENVIRONMENT_GET_CAN_DUPE: // 3
			{
				bool *b = (bool*)data;
				*b = true;
				return true;
			}
			break;
		case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: // 27
			{
				struct retro_log_callback *rlc = (struct retro_log_callback*)data;
				rlc->log = core_logger;
				return true;
			}
			break;
		case RETRO_ENVIRONMENT_SET_VARIABLES: // 16
			{
				struct retro_variable *var = (struct retro_variable*)data;
				int i;

				for(i=0;var[i].key!=NULL;i++) {
					printf("SetVar \"%s\" -> \"%s\"\n",var[i].key,var[i].value);
				}
			}
			break;
		case RETRO_ENVIRONMENT_GET_VARIABLE: // 15
			{
				struct retro_variable *var = (struct retro_variable*)data;

				printf("GetVar Key=\"%s\"\n",var->key);
				var->value = NULL;
				//var->value = "";
				return true;
			}

			break;
		case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
			{
				bool *b = (bool*)data;
				*b = false;
				return false;
			}
			break;
		case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: // 9
		case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: // 31
			{
				char **ppDir = (char**)data;
				*ppDir = NULL;
				return true;
			}
			break;
		case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: // 10
			{
				const enum retro_pixel_format *pFmt = (const enum retro_pixel_format *)data;
				switch(*pFmt) {
					case RETRO_PIXEL_FORMAT_0RGB1555:
						pCore->output.fb.fmt = eFMT_XRGB555;
						break;
					case RETRO_PIXEL_FORMAT_XRGB8888:
						pCore->output.fb.fmt = eFMT_XRGB888;
						break;
					case RETRO_PIXEL_FORMAT_RGB565:
						pCore->output.fb.fmt = eFMT_RGB565;
						break;
					default:
						assert(!"Unknown pixel format");
				}
				return true;
			}
			break;
		case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL: // 8
		case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS: // 11
		case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO:    // 34
		case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:   // 35
		case 44: // ??
			break;
		default:
			printf("%s: Unknown cmd %u\n",__func__,cmd);
			//assert(0);
			break;
	}
	return false;
}
