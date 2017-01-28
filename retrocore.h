#pragma once

enum retrocore_pixelformat {
	eFMT_XRGB555 = 0,
	eFMT_RGB565  = 1,
	eFMT_XRGB888 = 2,
};

typedef struct retroimage {
	unsigned int w,h;
	size_t pitch;

	void *pData;

	enum retrocore_pixelformat fmt;
} retroimage_t;

typedef struct retroaudio {
	unsigned int rate; // Sample-rate

	uint16_t frames;   // Number of frames available in pSamples
	int16_t *pSamples; // Interleaved stereo, native-endian 16bit signed
} retroaudio_t;

struct retrocore *retrocore_create(void);
void              retrocore_destroy(struct retrocore *pCore);
int               retrocore_add_core(struct retrocore *pCore,const char *pzCoreFile);

int               retrocore_load_game(struct retrocore *pCore,const char *pzGameFile);
int               retrocore_run(struct retrocore *pCore,retroimage_t *pFB,retroaudio_t *pAudio);
