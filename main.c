#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <pthread.h> 
#include <dirent.h>
#include "picture_t.h"
#include "simplerecorder.h"
#include "log.h"
#define MAX_SIZE 1024*1024*10


static int recording;
static char osd_string[20];
static char mkv_filename[100];
static char dirname[20];
static char tempname[20];
static struct picture_t pic;

static void gen_osd_info()
{
	time_t t = time(0);
	strftime(osd_string, 20, "%Y-%m-%d %H:%M:%S",localtime(&t));
}
static void get_filename()
{
	time_t t = time(0);
	strcpy(mkv_filename,"./Record/");
	strftime(dirname, 20, "%Y%m%d",localtime(&t));
	strftime(tempname, 20, "%H%M%S",localtime(&t));
	strcat(mkv_filename,dirname);
	if(NULL==opendir(mkv_filename))
		mkdir(mkv_filename,0775);
	strcat(mkv_filename,"/");
	strcat(mkv_filename,tempname);
	strcat(mkv_filename,".mkv");
}
void stop_recording(int param)
{
	recording = 0;
}


int main()
{
	int i,FileSize;
	struct timeval tpstart,tpend;
	float timeuse;
	struct encoded_pic_t encoded_pic,header_pic;
	errno = 0;
	if(NULL==opendir("./Record"))
		mkdir("./Record",0775);
	if(!camera_init(&pic))
		goto error_cam; 
	if(!encoder_init(&pic)){
		fprintf(stderr,"failed to initialize encoder\n");
		goto error_encoder;
	}
	if(!preview_init(&pic))
		goto error_preview;
	get_filename();
	printf("file:%s\n",mkv_filename);
	if(!output_init(&pic,mkv_filename))
		goto error_output;
	if(!encoder_encode_headers(&encoded_pic))
		goto error_output;
	memcpy(&header_pic,&encoded_pic,sizeof(encoded_pic));
	header_pic.buffer=malloc(encoded_pic.length);
	memcpy(header_pic.buffer,encoded_pic.buffer,encoded_pic.length);
	if(!output_write_headers(&header_pic))
		goto error_output;
	encoder_release(&encoded_pic);
	if(!camera_on())
		goto error_cam_on;
	if(signal(SIGINT, stop_recording) == SIG_ERR){
		fprintf(stderr,"signal() failed\n");
		goto error_signal;
	}
	printf("Press ctrl-c to stop recording...\n");
	recording = 1;
	FileSize =0;
	for(i=0; recording; i++){
		if(!camera_get_frame(&pic))
			break;
		gen_osd_info();
		osd_print(&pic, osd_string);
		//if((i&7)==0) // i%8==0 
		gettimeofday(&tpstart,NULL);
		preview_display(&pic);
		gettimeofday(&tpend,NULL);
		timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+
		tpend.tv_usec-tpstart.tv_usec;
		timeuse/=1000000;
		//printf("usetime:%f\n",timeuse);
		FileSize+=encoded_pic.length;
		if(!encoder_encode_frame(&pic, &encoded_pic))
			break;
		applog_flush();	
		if ((FileSize>MAX_SIZE) && (encoded_pic.frame_type ==FRAME_TYPE_I)) {
			output_close();	
			get_filename();
			printf("file:%s\n",mkv_filename);
			if(!output_init(&pic,mkv_filename))
				goto error_output;
			if(!output_write_headers(&header_pic))
				goto error_output;
			FileSize=0;
			ResetTime(&pic,&encoded_pic);
			if(!output_write_frame(&encoded_pic))
				break;
			encoder_release(&encoded_pic);
		} else {
			if(!output_write_frame(&encoded_pic))
				break;
			encoder_release(&encoded_pic);
		}
	}
	printf("\n%d frames recorded\n", i);

error_signal:
	printf("error signal\n");
	camera_off();
error_cam_on:
	printf("error cam\n");
	output_close();
error_output:
	printf("error output\n");
	preview_close();
error_preview:
    printf("error encode\n");
	encoder_close();
error_encoder:
    printf("error cam\n");
	camera_close();
error_cam:
    printf("error over\n");
	return 0;
}
