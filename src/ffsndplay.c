// tutorial03.c
// A pedagogical video player that will stream through every video frame as fast as it can
// and play audio (out of sync).
//
// Code based on FFplay, Copyright (c) 2003 Fabrice Bellard, 
// and a tutorial by Martin Bohme (boehme@inb.uni-luebeckREMOVETHIS.de)
// Tested on Gentoo, CVS version 5/01/07 compiled with GCC 4.1.1
// Use
//
// gcc -o tutorial03 tutorial03.c -lavformat -lavcodec -lz -lm `sdl-config --cflags --libs`
// to build (assuming libavformat and libavcodec are correctly installed, 
// and assuming you have sdl-config. Please refer to SDL docs for your installation.)
//
// Run using
// tutorial03 myvideofile.mpg
//
// to play the stream on your screen.


#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>

#include <SDL.h>
#include <SDL_thread.h>



#define INBUF_SIZE 4096


#ifdef __MINGW32__
#undef main /* Prevents SDL from overriding main() */
#endif

#include <stdio.h>

#define SDL_AUDIO_BUFFER_SIZE 1024

typedef struct PacketQueue {
  AVPacketList *first_pkt, *last_pkt;
  int nb_packets;
  int size;
  SDL_mutex *mutex;
  SDL_cond *cond;
} PacketQueue;

PacketQueue audioq;

int quit = 0;

void packet_queue_init(PacketQueue *q) {
  memset(q, 0, sizeof(PacketQueue));
  q->mutex = SDL_CreateMutex();
  q->cond = SDL_CreateCond();
}
int packet_queue_put(PacketQueue *q, AVPacket *pkt) {

  AVPacketList *pkt1;
  if(av_dup_packet(pkt) < 0) {
    return -1;
  }
  pkt1 = av_malloc(sizeof(AVPacketList));
  if (!pkt1)
    return -1;
  pkt1->pkt = *pkt;
  pkt1->next = NULL;
  
  
  SDL_LockMutex(q->mutex);
  
  if (!q->last_pkt)
    q->first_pkt = pkt1;
  else
    q->last_pkt->next = pkt1;
  q->last_pkt = pkt1;
  q->nb_packets++;
  q->size += pkt1->pkt.size;
  SDL_CondSignal(q->cond);
  
  SDL_UnlockMutex(q->mutex);
  return 0;
}
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
{
  AVPacketList *pkt1;
  int ret;
  
  SDL_LockMutex(q->mutex);
  
  for(;;) {
    
    if(quit) {
      ret = -1;
      break;
    }

    pkt1 = q->first_pkt;
    if (pkt1) {
      q->first_pkt = pkt1->next;
      if (!q->first_pkt)
	q->last_pkt = NULL;
      q->nb_packets--;
      q->size -= pkt1->pkt.size;
      *pkt = pkt1->pkt;
      av_free(pkt1);
      ret = 1;
      break;
    } else if (!block) {
      ret = 0;
      break;
    } else {
      SDL_CondWait(q->cond, q->mutex);
    }
  }
  SDL_UnlockMutex(q->mutex);
  return ret;
}

int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size) {

  static AVPacket pkt;
  static uint8_t *audio_pkt_data = NULL;
  static int audio_pkt_size = 0;

  int len1, data_size;

  for(;;) {
    while(audio_pkt_size > 0) {
      data_size = buf_size;
      len1 = avcodec_decode_audio2(aCodecCtx, (int16_t *)audio_buf, &data_size, 
				   audio_pkt_data, audio_pkt_size);
      if(len1 < 0) {
	/* if error, skip frame */
	audio_pkt_size = 0;
	break;
      }
      audio_pkt_data += len1;
      audio_pkt_size -= len1;
      if(data_size <= 0) {
	/* No data yet, get more frames */
	continue;
      }
      /* We have data, return it and come back for more later */
      return data_size;
    }
    if(pkt.data)
      av_free_packet(&pkt);

    if(quit) {
      return -1;
    }

    if(packet_queue_get(&audioq, &pkt, 1) < 0) {
      return -1;
    }
    audio_pkt_data = pkt.data;
    audio_pkt_size = pkt.size;
  }
}



/*
 * Audio decoding.
 */
int audio_decode_example(const char *outfilename, const char *filename)
{
  int out_size, len;
  FILE *outfile;
  uint8_t *inbuf_ptr;
  uint8_t* buf;
  uint8_t* bufptr;

  AVFormatContext *pFormatCtx;
  int             i, audioStream;
  AVPacket        packet;
  AVCodecContext  *aCodecCtx;
  AVCodec         *aCodec;


  // Register all formats and codecs
  av_register_all();


  // Open file
  if(av_open_input_file(&pFormatCtx, filename, NULL, 0, NULL)!=0)
    return -1; // Couldn't open file
  
  // Retrieve stream information
  if(av_find_stream_info(pFormatCtx)<0)
    return -1; // Couldn't find stream information
  
  // Dump information about file onto standard error
  dump_format(pFormatCtx, 0, filename, 0);
  
  // Find the first audio stream
  audioStream=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++) 
    if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_AUDIO)
      {
	audioStream=i;
	break;
      }
  if(audioStream==-1)
    return -1;
   
  aCodecCtx=pFormatCtx->streams[audioStream]->codec;

  aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
  if(!aCodec) {
    fprintf(stderr, "Unsupported codec!\n");
    return -1;
  }
  avcodec_open(aCodecCtx, aCodec);



  printf("sample rate %d\n", aCodecCtx->sample_rate);
  printf("channels %d\n", aCodecCtx->channels);
  printf("duration %ld\n", pFormatCtx->duration/AV_TIME_BASE + 1);


  // sample_rate x seconds x channels
  size_t bufsz = sizeof(uint16_t) * aCodecCtx->sample_rate * (pFormatCtx->duration/AV_TIME_BASE + 2) * aCodecCtx->channels;
  if(bufsz < AVCODEC_MAX_AUDIO_FRAME_SIZE)
    bufsz = AVCODEC_MAX_AUDIO_FRAME_SIZE;

  size_t final_len=0;

  buf = av_mallocz(bufsz);
  bufptr = buf;



  outfile = fopen(outfilename, "wb");
  if (!outfile) {
    return -1;
  }



  /* decode until eof */
  while(av_read_frame(pFormatCtx, &packet)>=0 || !quit) {
  
    if(packet.stream_index==audioStream) 
      {
	//fprintf(stderr, "got audio\n");
        if (packet.size == 0)
	  break;

        inbuf_ptr = packet.data;
        while (packet.size > 0) {

	  out_size = bufsz; // reset this as it gets sets by avcodec_decode_audio2()
	  len = avcodec_decode_audio2(aCodecCtx, (short*)bufptr, &out_size,
				      inbuf_ptr, packet.size);
	  if (len < 0) {
	    fprintf(stderr, "Error while decoding\n");
	    exit(1);
	  }
	  if (out_size > 0) {
	    bufptr += out_size;
	    final_len += out_size;
	  }
	  packet.size -= len;
	  inbuf_ptr += len;
        }

	av_free_packet(&packet);
      }
    else 
      {
	fprintf(stderr, "got sth else\n");
	av_free_packet(&packet);
      }
  }

  printf("final len %ld\n", final_len);

  fwrite(buf, 1, bufsz, outfile);
  fclose(outfile);

  // Close the codec
  avcodec_close(aCodecCtx);
   
  // Close the video file
  av_close_input_file(pFormatCtx);

  return 0;
}



void audio_callback(void *userdata, Uint8 *stream, int len) {

  AVCodecContext *aCodecCtx = (AVCodecContext *)userdata;
  int len1, audio_size;

  static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
  static unsigned int audio_buf_size = 0;
  static unsigned int audio_buf_index = 0;

  while(len > 0) {
    if(audio_buf_index >= audio_buf_size) {
      /* We have already sent all our data; get more */
      audio_size = audio_decode_frame(aCodecCtx, audio_buf, sizeof(audio_buf));
      if(audio_size < 0) {
	/* If error, output silence */
	audio_buf_size = 1024; // arbitrary?
	memset(audio_buf, 0, audio_buf_size);
      } else {
	audio_buf_size = audio_size;
      }
      audio_buf_index = 0;
    }
    len1 = audio_buf_size - audio_buf_index;
    if(len1 > len)
      len1 = len;
    memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
    //
    //memcpy(bufptr, (uint8_t *)audio_buf + audio_buf_index, len1);
    //bufptr += len1;
    //
    len -= len1;
    stream += len1;
    audio_buf_index += len1;
  }
}







int main(int argc, char *argv[]) {
  AVFormatContext *pFormatCtx;
  //  int             i, videoStream, audioStream;
  int             i, audioStream;
  AVCodecContext  *pCodecCtx;
  AVCodec         *pCodec;
  AVFrame         *pFrame; 
  AVPacket        packet;
  int             frameFinished;
  float           aspect_ratio;
  
  AVCodecContext  *aCodecCtx;
  AVCodec         *aCodec;

  SDL_Overlay     *bmp;
  SDL_Surface     *screen;
  SDL_Rect        rect;
  SDL_Event       event;
  SDL_AudioSpec   wanted_spec, spec;

  if(argc < 2) {
    fprintf(stderr, "Usage: test <file>\n");
    exit(1);
  }


  audio_decode_example("out.raw", argv[1]);
  exit(0);


  // Register all formats and codecs
  av_register_all();
  
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    //  if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
    exit(1);
  }

  // Open video file
  if(av_open_input_file(&pFormatCtx, argv[1], NULL, 0, NULL)!=0)
    return -1; // Couldn't open file
  
  // Retrieve stream information
  if(av_find_stream_info(pFormatCtx)<0)
    return -1; // Couldn't find stream information
  
  // Dump information about file onto standard error
  dump_format(pFormatCtx, 0, argv[1], 0);
  
  // Find the first video stream
  //videoStream=-1;
  audioStream=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++) {
    /*if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO &&
      videoStream < 0) {
      videoStream=i;
      }*/
    if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_AUDIO &&
       audioStream < 0) {
      audioStream=i;
    }
  }
  //  if(videoStream==-1)
  // return -1; // Didn't find a video stream
  if(audioStream==-1)
    return -1;
   
  aCodecCtx=pFormatCtx->streams[audioStream]->codec;
  // Set audio settings from codec info
  wanted_spec.freq = aCodecCtx->sample_rate;
  wanted_spec.format = AUDIO_S16SYS;
  wanted_spec.channels = aCodecCtx->channels;
  wanted_spec.silence = 0;
  wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
  //wanted_spec.callback = audio_callback;
  wanted_spec.userdata = aCodecCtx;


  
  if(SDL_OpenAudio(&wanted_spec, &spec) < 0) {
    fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
    return -1;
  }
  aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
  if(!aCodec) {
    fprintf(stderr, "Unsupported codec!\n");
    return -1;
  }
  avcodec_open(aCodecCtx, aCodec);

  // audio_st = pFormatCtx->streams[index]
  packet_queue_init(&audioq);
  SDL_PauseAudio(0);

  /* // Get a pointer to the codec context for the video stream
     pCodecCtx=pFormatCtx->streams[videoStream]->codec;
  
     // Find the decoder for the video stream
     pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
     if(pCodec==NULL) {
     fprintf(stderr, "Unsupported codec!\n");
     return -1; // Codec not found
     }
     // Open codec
     if(avcodec_open(pCodecCtx, pCodec)<0)
     return -1; // Could not open codec
  
     // Allocate video frame
     pFrame=avcodec_alloc_frame();*/

  // Make a screen to put our video
  /*
    #ifndef __DARWIN__
    screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
    #else
    screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 24, 0);
    #endif
    if(!screen) {
    fprintf(stderr, "SDL: could not set video mode - exiting\n");
    exit(1);
    }
  
    // Allocate a place to put our YUV image on that screen
    bmp = SDL_CreateYUVOverlay(pCodecCtx->width,
    pCodecCtx->height,
    SDL_YV12_OVERLAY,
    screen);
  */



  printf("sample rate %d\n", aCodecCtx->sample_rate);
  printf("channels %d\n", aCodecCtx->channels);
  printf("duration %d\n", pFormatCtx->duration/AV_TIME_BASE + 1);

  // sample_rate x seconds x channels
  size_t bufsz = 10* aCodecCtx->sample_rate * (pFormatCtx->duration/AV_TIME_BASE + 1) * aCodecCtx->channels;

  //buf = av_mallocz(bufsz);
  //bufptr = buf;

  // Read frames and save first five frames to disk
  i=0;
  while(av_read_frame(pFormatCtx, &packet)>=0 || !quit) {
    // Is this a packet from the video stream?
    
    /*if(packet.stream_index==videoStream) 
      {
      fprintf(stderr, "got video\n");

      // Decode video frame
      avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, 
      packet.data, packet.size);

      
      // Did we get a video frame?
      if(frameFinished) {
      SDL_LockYUVOverlay(bmp);

      AVPicture pict;
      pict.data[0] = bmp->pixels[0];
      pict.data[1] = bmp->pixels[2];
      pict.data[2] = bmp->pixels[1];

      pict.linesize[0] = bmp->pitches[0];
      pict.linesize[1] = bmp->pitches[2];
      pict.linesize[2] = bmp->pitches[1];

      // Convert the image into YUV format that SDL uses
      img_convert(&pict, PIX_FMT_YUV420P,
      (AVPicture *)pFrame, pCodecCtx->pix_fmt, 
      pCodecCtx->width, pCodecCtx->height);
	
      SDL_UnlockYUVOverlay(bmp);
	
      rect.x = 0;
      rect.y = 0;
      rect.w = pCodecCtx->width;
      rect.h = pCodecCtx->height;
      SDL_DisplayYUVOverlay(bmp, &rect);
      av_free_packet(&packet);
      }
	
      } 
      else*/
    if(packet.stream_index==audioStream) 
      {
	fprintf(stderr, "got audio\n");

	packet_queue_put(&audioq, &packet);


      }
    else 
      {
	fprintf(stderr, "got sth else\n");
	av_free_packet(&packet);
      }
   
    SDL_PollEvent(&event);
    switch(event.type) 
      {
      case SDL_QUIT:
	fprintf(stderr, "got SDL_QUIT\n");


	FILE *outfile;
	outfile = fopen("out.raw", "wb");
	if (!outfile) {
	  fprintf(stderr, "opening outfile failed \n");
	}
	//fwrite(buf, 1, bufsz, outfile);
	fclose(outfile);
  
	//av_free(buf);


	quit = 1;
	SDL_Quit();
	exit(0);
	break;
      default:
	break;
      }

  } // while

  FILE *outfile;
  outfile = fopen("out.raw", "wb");
  if (!outfile) {
    fprintf(stderr, "opening outfile failed \n");
  }
  //fwrite(buf, 1, bufsz, outfile);
  fclose(outfile);
  
  //av_free(buf);


  fprintf(stderr, "bye, quit was %d \n", quit);

  // Free the YUV frame
  //av_free(pFrame);
  
  // Close the codec
  avcodec_close(pCodecCtx);
  
  // Close the video file
  av_close_input_file(pFormatCtx);

  quit =1 ;
  SDL_Quit();
  
  return 0;
}
