#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <SDL/SDL.h>
#include "picture_t.h"
#include "simplerecorder.h"

static	SDL_Surface *pscreen;
static 	SDL_Overlay *overlay;
static 	SDL_Rect drect;
static Uint32 SDL_VIDEO_Flags = SDL_SWSURFACE|SDL_RESIZABLE|SDL_ANYFORMAT;
int y_Size;

int preview_init(struct picture_t *info)
{
	y_Size= info->width*info->height;
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return 0;
    }
    pscreen =SDL_SetVideoMode(info->width, info->height, 0,
			 SDL_VIDEO_Flags);
    overlay =SDL_CreateYUVOverlay(info->width, info->height,
			     SDL_YV12_OVERLAY, pscreen);
	drect.x=0;
	drect.y=0;
	drect.w=info->width;
	drect.h=info->height;
	if (overlay->hw_overlay) 
		printf("Run in Hardware!\n");
	else
		printf("Run in Software!\n");
	SDL_WM_SetCaption ("Camera H264 DEMO By Ashwing", NULL);
	return 1;
}

int preview_display(struct picture_t *pic)
{
	auto y_video_data = pic->buffer;
	auto u_video_data = pic->buffer + y_Size;
	auto v_video_data = u_video_data + (y_Size / 4);
	SDL_LockSurface(pscreen);
	SDL_LockYUVOverlay(overlay);
	//memcpy(overlay->pixels[0],y_video_data, y_Size);
	overlay->pixels[0]=y_video_data;
	overlay->pixels[1]=v_video_data;
	overlay->pixels[2]=u_video_data;
	//memcpy(overlay->pixels[1],v_video_data, y_Size/4);
	//memcpy(overlay->pixels[2],u_video_data, y_Size/4);
    SDL_UnlockYUVOverlay(overlay);  
    SDL_UnlockSurface(pscreen);   	
    SDL_DisplayYUVOverlay(overlay, &drect); 
	return 1;
}
void preview_close()
{
	SDL_FreeYUVOverlay(overlay);
	free(pscreen);
	SDL_Quit();
}