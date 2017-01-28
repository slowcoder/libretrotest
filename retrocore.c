#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dlfcn.h>
#include <assert.h>
#include "libretro.h"

#include "retrocore.h"
#include "retrocore_internal.h"
#include "retrocore_env.h"

static struct retrocore *pRunningCore = NULL;

static int populateCore(retrocore_core_t *pCCore,void *dl) {
	pCCore->set_environment = dlsym(dl,"retro_set_environment");
	if( pCCore->set_environment == NULL ) return -1;
	pCCore->init            = dlsym(dl,"retro_init");
	if( pCCore->init            == NULL ) return -1;
	pCCore->deinit          = dlsym(dl,"retro_deinit");
	if( pCCore->deinit          == NULL ) return -1;
	pCCore->api_version     = dlsym(dl,"retro_api_version");
	if( pCCore->api_version     == NULL ) return -1;
	pCCore->get_system_info = dlsym(dl,"retro_get_system_info");
	if( pCCore->get_system_info == NULL ) return -1;
	pCCore->load_game       = dlsym(dl,"retro_load_game");
	if( pCCore->load_game       == NULL ) return -1;
	pCCore->reset           = dlsym(dl,"retro_reset");
	if( pCCore->reset           == NULL ) return -1;
	pCCore->run             = dlsym(dl,"retro_run");
	if( pCCore->run             == NULL ) return -1;
	pCCore->get_system_av_info = dlsym(dl,"retro_get_system_av_info");
	if( pCCore->get_system_av_info == NULL ) return -1;

	pCCore->set_video_refresh      = dlsym(dl,"retro_set_video_refresh");
	if( pCCore->set_video_refresh      == NULL ) return -1;
	pCCore->set_audio_sample       = dlsym(dl,"retro_set_audio_sample");
	if( pCCore->set_audio_sample       == NULL ) return -1;
	pCCore->set_audio_sample_batch = dlsym(dl,"retro_set_audio_sample_batch");
	if( pCCore->set_audio_sample_batch == NULL ) return -1;
	pCCore->set_input_state        = dlsym(dl,"retro_set_input_state");
	if( pCCore->set_input_state        == NULL ) return -1;
	pCCore->set_input_poll         = dlsym(dl,"retro_set_input_poll");
	if( pCCore->set_input_poll         == NULL ) return -1;

	unsigned ver;
	ver = pCCore->api_version();
	if (ver != RETRO_API_VERSION)
		return -2;

	struct retro_system_info sysinfo;
	pCCore->get_system_info(&sysinfo);
	printf("System info: (core=%p)\n",pCCore);
	printf(" Name: %s\n",sysinfo.library_name);
	printf(" Ver : %s\n",sysinfo.library_version);
	printf(" Exts: %s\n",sysinfo.valid_extensions);
	printf(" Need fullpath: %i\n",sysinfo.need_fullpath);
	printf(" Blockextract : %i\n",sysinfo.block_extract);

	pCCore->bNeedFullpath = sysinfo.need_fullpath;

	if( 1 ) {
		int endx = 0;
		char *p = (char*)sysinfo.valid_extensions;
		while(*p) {
			char *n;
			n = strstr(p,"|");
			if(n==NULL) {
				//printf("Ext: \"%s\", len=%i\n",p,strlen(p));
				pCCore->pzExt[endx++] = strndup(p,strlen(p));
				p += strlen(p);
			} else {
				//printf("Ext: \"%s\", len=%i\n",p,n-p);
				pCCore->pzExt[endx++] = strndup(p,n-p);
				p = n + 1;
			}
		}
	}

	pCCore->dl = dl;

	return 0;
}

static void releaseCore(retrocore_core_t *pCCore) {
	int i;

	if (pCCore == NULL)
		return;

	pCCore->deinit();
	dlclose(pCCore->dl);

	for(i=0;i<MAX_EXTS_PER_CORE;i++) {
		if (pCCore->pzExt[i] != NULL) {
			free(pCCore->pzExt[i]);
			pCCore->pzExt[i] = NULL;
		}
	}

	pCCore->dl = NULL;
}

static retrocore_core_t *getCoreByExt(struct retrocore *pCore,const char *pzExt) {
	int c,e;
	retrocore_core_t *pCCore;

	for(c=0;c<MAX_CORES;c++) {
		pCCore = &pCore->core[c];
		if (pCCore->dl != NULL) {
			for(e=0;e<MAX_EXTS_PER_CORE;e++) {
				if( pCCore->pzExt[e] != NULL ) {
					if( strcmp(pCCore->pzExt[e],pzExt) == 0 ) return pCCore;
				}
			}
		}
	}

	return NULL;
}

int loadFile(const char *pzFile,void **ppData,size_t *pSize) {
	FILE *in;
	size_t s;
	void  *d;

	in = fopen(pzFile,"rb");
	assert(in != NULL);
	fseek(in,0,SEEK_END);
	s = ftell(in);
	fseek(in,0,SEEK_SET);

	d = malloc(s);
	fread(d,1,s,in);
	fclose(in);

	*ppData = d;
	*pSize  = s;

	return 0;
}

void core_logger(enum retro_log_level level,const char *fmt, ...) {
	va_list ap;

	va_start(ap,fmt);
	vprintf(fmt,ap);
	va_end(ap);
}

/*
 * Callbacks
 */
static bool core_retro_environment(unsigned cmd, void *data) {
	return intcore_retro_environment(pRunningCore,cmd,data);
}


static void core_retro_video_refresh(const void *data, unsigned width, unsigned height, size_t pitch) {
//	printf("%s:\n",__func__);
	int bpp = 2;

	if( data == NULL ) return;

	if (pRunningCore->output.fb.fmt == eFMT_XRGB888)
		bpp = 4;

	// Did we change size?
	if ( (pRunningCore->output.fb.w != width) ||
		 (pRunningCore->output.fb.h != height) ||
		 (pRunningCore->output.fb.pitch != pitch) ) {
		if( pRunningCore->output.fb.pData != NULL )
			free(pRunningCore->output.fb.pData);

		pRunningCore->output.fb.pData = malloc(width*height*bpp);
	}

	int sl;
	uint16_t *pSrc,*pDst;

	pSrc = (uint16_t*)data;
	pDst = (uint16_t*)pRunningCore->output.fb.pData;
	for(sl=0;sl<height;sl++) {
		memcpy( &pDst[width*sl*(bpp>>1)], &pSrc[(pitch/2)*sl],width*bpp );
	}

	pRunningCore->output.fb.w     = width;
	pRunningCore->output.fb.h     = height;
	pRunningCore->output.fb.pitch = pitch;
}

void core_retro_audio_sample(int16_t left, int16_t right) {
	pRunningCore->output.audio.pSamples[pRunningCore->output.audio.frames*2+0] = left;
	pRunningCore->output.audio.pSamples[pRunningCore->output.audio.frames*2+1] = right;
	pRunningCore->output.audio.frames++;
//	printf("%s:\n",__func__);
}

size_t core_retro_audio_sample_batch(const int16_t *data,size_t frames) {
//	printf("%s:\n",__func__);

	memcpy( &pRunningCore->output.audio.pSamples[pRunningCore->output.audio.frames*2],
		data,
		frames * 2 * 2);

	pRunningCore->output.audio.frames += frames;
	return 0;
}

int16_t core_retro_input_state(unsigned port, unsigned device,unsigned index, unsigned id) {
//	printf("%s:\n",__func__);
	return 0;
}

void core_retro_input_poll(void) {
//	printf("%s:\n",__func__);
}

/*
 * Public API functions
 */
struct retrocore *retrocore_create(void) {
	retrocore_t *pCore;

	pCore = (retrocore_t*)calloc(1,sizeof(retrocore_t));

	pCore->output.audio.pSamples = (int16_t*)malloc(MAX_AUDIO_FRAMES * 2 * 2);
	printf("Internal audio buffer: %p\n",pCore->output.audio.pSamples);

	return pCore;
}

void              retrocore_destroy(struct retrocore *pCore) {
	int c;

	if (pCore == NULL)
		return;

	for(c=0;c<MAX_CORES;c++) {
		if (pCore->core[c].dl != NULL) {
			releaseCore(&pCore->core[c]);
		}
	}

	if( pCore->output.audio.pSamples != NULL ) {
		free(pCore->output.audio.pSamples);
		pCore->output.audio.pSamples = NULL;
	}

	free(pCore);
}

int               retrocore_add_core(struct retrocore *pCore,const char *pzCoreFile) {
	int c;

	for(c=0;c<MAX_CORES;c++) {
		if (pCore->core[c].dl == NULL) { // Free slot?
			void *dl;

			dl = dlopen(pzCoreFile,RTLD_LAZY);
			if( dl == NULL ) {
				printf("Failed to dlopen(\"%s\",..) : %s\n",pzCoreFile,dlerror());
				return -2;
			}

			populateCore(&pCore->core[c],dl);

			return 0;
		}
	}
	return -1;
}

int               retrocore_load_game(struct retrocore *pCore,const char *pzGameFile) {

	char *pzLastDot = NULL,*p;

	if ((pCore == NULL) || (pzGameFile==NULL) )
		return -1;

	p = (char*)pzGameFile;
	while(*p) {
		if (*p == '.') pzLastDot = p + 1;
		p++;
	}
	if (pzLastDot == NULL)
		return -2;
	if (*pzLastDot == 0)
		return -3;

	retrocore_core_t *pCCore;

	pCCore = getCoreByExt(pCore,pzLastDot);
	if( pCCore == NULL ) {
		printf("No core for ext \"%s\" found..\n",pzLastDot);
		return -4;
	} else {
		printf("Found core %p for ext \"%s\"\n",pCCore,pzLastDot);
	}

	pRunningCore = pCore;

	pCCore->set_environment( core_retro_environment );
	pCCore->init();

	struct retro_system_av_info avinfo;
	pCCore->get_system_av_info(&avinfo);
	printf("Requested AV info:\n");
	printf(" FPS: %.2f\n",avinfo.timing.fps);
	printf(" Samplerate: %.2f\n",avinfo.timing.sample_rate);
	pCore->output.audio.rate = avinfo.timing.sample_rate;

	struct retro_game_info gameinfo = {0};
	if( pCCore->bNeedFullpath ) {
		gameinfo.path = pzGameFile;
	} else {
		loadFile(pzGameFile,(void**)&gameinfo.data,&gameinfo.size);
	}

	bool r;
	r = pCCore->load_game(&gameinfo);
	assert(r == true);

	pCCore->set_video_refresh( core_retro_video_refresh );
	pCCore->set_audio_sample( core_retro_audio_sample );
	pCCore->set_audio_sample_batch( core_retro_audio_sample_batch );
	pCCore->set_input_state( core_retro_input_state );
	pCCore->set_input_poll( core_retro_input_poll );

	pCore->pCurrentCCore = pCCore;

	return 0;
}

int               retrocore_run(struct retrocore *pCore,retroimage_t *pFB,retroaudio_t *pAudio) {

	if (pCore == NULL)
		return -1;

	pRunningCore = pCore;

	// Reset audio buffer
	pRunningCore->output.audio.frames = 0;

	pCore->pCurrentCCore->run();

	if( pAudio != NULL ) {
		pAudio->frames = pRunningCore->output.audio.frames;
		pAudio->rate   = pRunningCore->output.audio.rate;
		if( pAudio->pSamples != NULL) {
			memcpy(	pAudio->pSamples,
					pRunningCore->output.audio.pSamples,
					pRunningCore->output.audio.frames * 2 * 2);
		}
	}

//	printf("Generated %i frames of audio\n",pRunningCore->output.audio.frames);

	if (pFB != NULL) {
		memcpy(pFB,&pCore->output.fb,sizeof(retroimage_t));
	}

	return 0;
}

