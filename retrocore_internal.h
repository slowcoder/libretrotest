#pragma once

#include "retrocore.h"

#define MAX_CORES         20
#define MAX_EXTS_PER_CORE 10
#define MAX_AUDIO_FRAMES  48000

typedef struct {
	void *dl;
	char *pzExt[MAX_EXTS_PER_CORE];
	int   bNeedFullpath;

	void (*init)(void);
	void (*deinit)(void);

	unsigned (*api_version)(void);
	void (*get_system_info)(struct retro_system_info *info);
	void (*get_system_av_info)(struct retro_system_av_info *info);

	bool (*load_game)(const struct retro_game_info *game);
	void (*run)(void);
	void (*reset)(void);

	void (*set_environment)(retro_environment_t);
	void (*set_video_refresh)(retro_video_refresh_t);
	void (*set_audio_sample)(retro_audio_sample_t);
	void (*set_audio_sample_batch)(retro_audio_sample_batch_t);
	void (*set_input_poll)(retro_input_poll_t);
	void (*set_input_state)(retro_input_state_t);
} retrocore_core_t;

typedef struct retrocore {
	retrocore_core_t core[MAX_CORES];

	retrocore_core_t *pCurrentCCore;

	struct {
		retroimage_t fb;
		retroaudio_t audio;
	} output;
} retrocore_t;

void core_logger(enum retro_log_level level,const char *fmt, ...);