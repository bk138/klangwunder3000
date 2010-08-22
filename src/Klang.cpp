#include <cerrno>
extern "C" {
#include "avcodec.h"
#include "avformat.h"
}
#include "wx/intl.h"
#include "Klang.h"


using namespace std;


// initialise primitive datatypes
Klang::Klang()
{
  p_init = 0;
  p_incr= 0;
  p_decr = 0;
  p_now = p_init; 
  loops_min = 0;
  loops_max = 0;
  
  alGenBuffers(1, &al_buffer);
  alGenSources(1, &static_source);
  alGenSources(1, &dynamic_source);
}


Klang::~Klang()
{
  if(alIsBuffer(al_buffer))
    alDeleteBuffers(1, &al_buffer);
  if(alIsSource(static_source))
    alDeleteSources(1, &static_source);
  if(alIsSource(dynamic_source))
    alDeleteSources(1, &dynamic_source);
}


Klang::Klang(const Klang& k)
{
  p_init = k.p_init;
  p_incr= k.p_incr;
  p_decr = k.p_decr;
  p_now = k.p_now;
  loops_min = k.loops_min;
  loops_max = k.loops_max;

  name = k.name;
  filename = k.filename;
  err = k.err;
  
  file_buffer = k.file_buffer;
  data_buffer = k.data_buffer;

  alGenBuffers(1, &al_buffer);
  alGenSources(1, &static_source);
  alGenSources(1, &dynamic_source);

  if(k.data_buffer.size() && alIsBuffer(k.al_buffer))
    {
      int bits, freq, channels;
      alGetBufferi(k.al_buffer, AL_BITS, &bits);
      alGetBufferi(k.al_buffer, AL_CHANNELS, &channels);
      alGetBufferi(k.al_buffer, AL_FREQUENCY, &freq);

      ALenum format;
      if(channels == 1 && bits == 8)
	format = AL_FORMAT_MONO8;
      if(channels == 1 && bits == 16)
	format = AL_FORMAT_MONO16;
      if(channels == 2 && bits == 8)
	format = AL_FORMAT_STEREO8;
      if(channels == 2 && bits == 16)
	format = AL_FORMAT_STEREO16;

      alBufferData(al_buffer, format, &data_buffer[0], data_buffer.size(), freq);
    }
}




bool Klang::loadSnd(vector<char>& src)
{
  /*
    first, save encoded sound file data for later saving
  */
  file_buffer = src;


  /*
    then, decode sound file data into raw sound data
  */
  int fd = -1;
  FILE *tmpfile;

#ifdef __WIN32__
  char* tmpfilename = tmpnam(NULL);
  fd = open(tmpfilename, "w+");
#else
  char tmpfilename[] = "/tmp/klangwunder3000.XXXXXX";
  fd = mkstemp(tmpfilename);
#endif

  if(fd == -1 || (tmpfile = fdopen(fd, "w+")) == NULL) 
    {
      if(fd != -1) 
	{
	  unlink(tmpfilename);
	  close(fd);
	}
      err.Printf(wxT("%s: %s\n"), tmpfilename, strerror(errno));
      return false;
    }


  // save buffer contents into tmpfile
  for(vector<char>::iterator it = src.begin(); it != src.end(); ++it)
    fputc(*it, tmpfile);
  close(fd);
  
  int out_size, len;
  uint8_t *inbuf_ptr;
  uint8_t *outbuf_ptr;

  AVFormatContext *pFormatCtx;
  int             audioStream;
  AVCodecContext  *aCodecCtx;
  AVCodec         *aCodec;


  // Open temp file again
  if(av_open_input_file(&pFormatCtx, tmpfilename, NULL, 0, NULL)!=0)
    {
      err.Printf(_("Opening temporary file failed."));
      unlink(tmpfilename);
      return false;
    }

  // Retrieve stream information
  if(av_find_stream_info(pFormatCtx)<0)
    {
      err.Printf(_("Retrieving stream info failed."));
      av_close_input_file(pFormatCtx);
      unlink(tmpfilename);
      return false;
    }
  
  // Dump information about file onto standard error
  dump_format(pFormatCtx, 0, tmpfilename, 0);
  
  // Find the first audio stream
  audioStream=-1;
  for(size_t i=0; i<pFormatCtx->nb_streams; i++) 
    if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_AUDIO)
      {
	audioStream=i;
	break;
      }
  if(audioStream==-1)
    {
      err.Printf(_("Could not find a audio stream."));
      av_close_input_file(pFormatCtx);
      unlink(tmpfilename);
      return false;
    }
    
  aCodecCtx=pFormatCtx->streams[audioStream]->codec;

  aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
  if(!aCodec) 
    {
      err.Printf(_("Did not find a suitable decoder."));
      av_close_input_file(pFormatCtx);
      unlink(tmpfilename);
      return false;
    }
  avcodec_open(aCodecCtx, aCodec);

  // sample_rate x seconds x channels
  size_t data_buffer_len = sizeof(uint16_t) * aCodecCtx->sample_rate * (pFormatCtx->duration/AV_TIME_BASE + 2) * aCodecCtx->channels;
  if(data_buffer_len < AVCODEC_MAX_AUDIO_FRAME_SIZE)
    data_buffer_len = AVCODEC_MAX_AUDIO_FRAME_SIZE;

  size_t final_len=0;
  data_buffer.resize(data_buffer_len);
  outbuf_ptr = (uint8_t*)&data_buffer[0];

  // decode until eof 
  AVPacket        packet;
  while(av_read_frame(pFormatCtx, &packet)>=0) 
    {
      if(packet.stream_index==audioStream) 
	{
	  if (packet.size == 0)
	    break;

	  inbuf_ptr = packet.data;
	  while (packet.size > 0) 
	    {
	      out_size = data_buffer_len; // reset this as it gets sets by avcodec_decode_audio2()
	      len = avcodec_decode_audio2(aCodecCtx, (short*)outbuf_ptr, &out_size,
					  inbuf_ptr, packet.size);
	      if (len < 0) 
		{
		  err.Printf(_("Error while decoding."));
		  avcodec_close(aCodecCtx);
		  av_close_input_file(pFormatCtx);
		  unlink(tmpfilename);
		  return false;
		}
	      if (out_size > 0) 
		{
		  outbuf_ptr += out_size;
		  final_len += out_size;
		}
	      packet.size -= len;
	      inbuf_ptr += len;
	    }

	  av_free_packet(&packet);
	}
      else 
	av_free_packet(&packet);

    }
  
  // copy the data into an openal buffer
  ALenum format;
  if(aCodecCtx->channels == 1)
    format = AL_FORMAT_MONO16;
  if(aCodecCtx->channels == 2)
    format = AL_FORMAT_STEREO16;

  alBufferData(al_buffer, format, &data_buffer[0], data_buffer_len, aCodecCtx->sample_rate);

  ALenum error = alGetError();
  if(error != AL_NO_ERROR)
    {
      err.Printf(_("Error creating AL buffer: "));
      err += wxString(alGetString(error), wxConvUTF8);
      avcodec_close(aCodecCtx);
      av_close_input_file(pFormatCtx);
      unlink(tmpfilename);
      return false;
    }

  // Close the codec
  avcodec_close(aCodecCtx);
  // Close the input file
  av_close_input_file(pFormatCtx);
  // and delete it
  unlink(tmpfilename);

  return true;
}


bool Klang::playStatic()
{
  bool status = true;

  if(al_buffer == AL_NONE)
    return false;

  alSourcei (static_source, AL_BUFFER, al_buffer);
  alSourcePlay (static_source);

  // Normally nothing should go wrong above, but one never knows...
  ALenum error = alGetError();
  if (error != ALUT_ERROR_NO_ERROR)
    {
      err.Printf(_("Error playing AL buffer: "));
      err += wxString(alGetString(error), wxConvUTF8);
      status = false;
    }

  return status;
}


bool Klang::playDynamic(ALfloat x, ALfloat y, ALfloat z)
{
  bool status = true;

  if(al_buffer == AL_NONE)
    return false;

  alSourcei (dynamic_source, AL_BUFFER, al_buffer);
  alSourcePlay (dynamic_source);

  // Normally nothing should go wrong above, but one never knows...
  ALenum error = alGetError();
  if (error != ALUT_ERROR_NO_ERROR)
    {
      err.Printf(_("Error playing AL buffer: "));
      err += wxString(alGetString(error), wxConvUTF8);
      status = false;
    }

  return status;
}



float Klang::getDuration() const
{
  if(alIsBuffer(al_buffer))
    {
      int channels, bits, freq, size;
      alGetBufferi(al_buffer, AL_CHANNELS, &channels);
      alGetBufferi(al_buffer, AL_BITS, &bits);
      alGetBufferi(al_buffer, AL_FREQUENCY, &freq);
      alGetBufferi(al_buffer, AL_SIZE, &size);

      float duration = (((float)size / ((float)bits/8.0)) / (float)channels) / (float)freq;
      return duration;
    }
  else
    return 0;
}



int Klang::getSampleRate() const
{
  if(alIsBuffer(al_buffer))
    {
      int freq;
      alGetBufferi(al_buffer, AL_FREQUENCY, &freq);
      return freq;
    }
  else
    return 0;
}


int Klang::getChannels() const
{
  if(alIsBuffer(al_buffer))
    {
      int channels;
      alGetBufferi(al_buffer, AL_CHANNELS, &channels);
      return channels;
    }
  else
    return 0;
}
