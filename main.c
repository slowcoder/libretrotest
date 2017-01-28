#include <stdio.h>
#include <SDL/SDL.h>

#include "retrocore.h"

static int16_t *pAudioBuffer = NULL;
static int audioPtrWrite = 0,audioPtrRead = 0;

static void fill_audio(void *udata, Uint8 *stream, int len) {
	int16_t *pDst = (int16_t*)stream;
	int i;

	//printf("%s\n",__func__);

	for(i=0;i<(len/4);i++) {
		pDst[i*2+0] = pAudioBuffer[(audioPtrRead&0xFFFF)*2+0];
		pDst[i*2+1] = pAudioBuffer[(audioPtrRead&0xFFFF)*2+1];
		audioPtrRead++;
	}
}


static int open_audio(void) {
    SDL_AudioSpec wanted;
    extern void fill_audio(void *udata, Uint8 *stream, int len);

    /* Set the audio format */
    wanted.freq = 32000;
    wanted.format = AUDIO_S16;
    wanted.channels = 2;    /* 1 = mono, 2 = stereo */
    wanted.samples = 1024;  /* Good low-latency value for callback */
    wanted.callback = fill_audio;
    wanted.userdata = NULL;

    /* Open the audio device, forcing the desired format */
    if ( SDL_OpenAudio(&wanted, NULL) < 0 ) {
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        return -1;
    }
    return 0;
}

int main(int argc,char **argv) {
	struct retrocore *pCore;
	SDL_Surface *pScreen = NULL;
	int bDone = 0;

	if( argc < 2 ) {
		printf("Usage: %s romfile\n",argv[0]);
		return -10;
	}

	pCore = retrocore_create();

	if (retrocore_add_core(pCore,"cores/bnes_libretro.so") != 0) {
		printf("Failed to add core..\n");
		return -1;
	}
	if (retrocore_add_core(pCore,"cores/bsnes_performance_libretro.so") != 0) {
		printf("Failed to add core..\n");
		return -1;
	}
	if (retrocore_add_core(pCore,"cores/picodrive_libretro.so") != 0) {
		printf("Failed to add core..\n");
		return -1;
	}
	if (retrocore_add_core(pCore,"cores/gambatte_libretro.so") != 0) {
		printf("Failed to add core..\n");
		return -1;
	}

	if( retrocore_load_game(pCore,argv[1]) != 0 ) {
		printf("Failed to load game..\n");
		return -2;
	}

	SDL_Init(SDL_INIT_VIDEO);
	pScreen = SDL_SetVideoMode(640,480,32,SDL_HWSURFACE | SDL_DOUBLEBUF);
	SDL_WM_SetCaption(argv[0],argv[0]);

	pAudioBuffer = (int16_t*)calloc(0x10000,2*2);

	open_audio();
	SDL_PauseAudio(0);

//return 0;
	while(!bDone) {
		retroimage_t FBimg;
		retroaudio_t audio;

		audio.pSamples = malloc(2048 * 10 * 2 * 2);

		SDL_Delay(16);
		if( retrocore_run(pCore,&FBimg,&audio) == 0 ) {
			SDL_Surface *pSrc;

			// Color format conversion, SDL style
			switch(FBimg.fmt) {
				case eFMT_XRGB555:
					pSrc = SDL_CreateRGBSurfaceFrom(FBimg.pData, // pixels
								FBimg.w,FBimg.h,
								16, // bpp
								FBimg.w*2, // pitch (bytes)
								0x7C00,  // R-Mask
								0x03E0,  // G-Mask
								0x001F,  // B-Mask
								0x0000); // A-Mask
					break;
				case eFMT_RGB565:
					pSrc = SDL_CreateRGBSurfaceFrom(FBimg.pData, // pixels
								FBimg.w,FBimg.h,
								16, // bpp
								FBimg.w*2, // pitch (bytes)
								0xF800,  // R-Mask
								0x07E0,  // G-Mask
								0x001F,  // B-Mask
								0x0000); // A-Mask
					break;
				case eFMT_XRGB888:
					pSrc = SDL_CreateRGBSurfaceFrom(FBimg.pData, // pixels
								FBimg.w,FBimg.h,
								32, // bpp
								FBimg.w*4, // pitch (bytes)
								0xFF0000,  // R-Mask
								0x00FF00,  // G-Mask
								0x0000FF,  // B-Mask
								0x00); // A-Mask
					break;
			}

			SDL_BlitSurface(pSrc,NULL,pScreen,NULL);
			SDL_FreeSurface(pSrc);

	        SDL_Flip(pScreen);

	        // Audio
	        int i,off;
	        off = audioPtrWrite;
	        for(i=0;i<audio.frames;i++) {
	        	pAudioBuffer[ ((off+i)&0xFFFF)*2+0 ] = audio.pSamples[i*2+0];
	        	pAudioBuffer[ ((off+i)&0xFFFF)*2+1 ] = audio.pSamples[i*2+1];
	        	//pAudioBuffer[ ((off+i)*2+0) & 0xFFFF ] = audio.pSamples[i*2+0];
	        	//pAudioBuffer[ ((off+i)*2+1) & 0xFFFF ] = audio.pSamples[i*2+1];

	        	off++;
	        }
	        audioPtrWrite += audio.frames;
		}
		free(audio.pSamples);
		audio.pSamples = NULL;

		SDL_Event ev;
        while( SDL_PollEvent(&ev) != 0 ) {
            switch(ev.type) {
                case SDL_KEYDOWN:
                if( ev.key.keysym.sym == SDLK_ESCAPE )
					bDone = 1;
            }
        }
	}

	retrocore_destroy(pCore);

	SDL_CloseAudio();
	return 0;
}
