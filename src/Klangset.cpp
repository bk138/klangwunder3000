
#include <iostream>
#include <cstdio>
#include <cerrno>
#include <memory>

#include "wx/fileconf.h"
#include "wx/mstream.h"
#include "wx/intl.h"
extern "C" {
#include "avcodec.h"
#include "avformat.h"
}

#include "Klangset.h"
#include "KW3KApp.h"

using namespace std;




/*
  internal constants
*/
#define KLW_CFGFILE "klangwunder.cfg" 




/*
  internal functions
*/
static inline size_t lRand(size_t limit);


/*******************************************
 *  Klang
 *******************************************/


// initialise primitive datatypes
Klang::Klang()
{
  p_init = 0;
  p_incr= 0;
  p_decr = 0;
  p_now = 0;
  loops_min = 0;
  loops_max = 0;
  
  data_buffer = 0;
  data_buffer_len = 0;
  alGenBuffers(1, &al_buffer);
}


Klang::~Klang()
{
  if(alIsBuffer(al_buffer))
    alDeleteBuffers(1, &al_buffer);
  if(data_buffer)
    av_free(data_buffer);
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

  data_buffer = 0;
  data_buffer_len = 0;
  alGenBuffers(1, &al_buffer);

  if(k.data_buffer_len && alIsBuffer(k.al_buffer))
    {
      data_buffer_len = k.data_buffer_len;
      data_buffer = (uint8_t*)av_mallocz(data_buffer_len);
      memcpy(data_buffer, k.data_buffer, data_buffer_len);

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

      alBufferData(al_buffer, format, data_buffer, data_buffer_len, freq);
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
  data_buffer_len = sizeof(uint16_t) * aCodecCtx->sample_rate * (pFormatCtx->duration/AV_TIME_BASE + 2) * aCodecCtx->channels;
  if(data_buffer_len < AVCODEC_MAX_AUDIO_FRAME_SIZE)
    data_buffer_len = AVCODEC_MAX_AUDIO_FRAME_SIZE;

  size_t final_len=0;
  data_buffer = (uint8_t*)av_mallocz(data_buffer_len);
  outbuf_ptr = data_buffer;

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

  alBufferData(al_buffer, format, data_buffer, data_buffer_len, aCodecCtx->sample_rate);

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


bool Klang::playSnd()
{
  bool status = true;

  if(al_buffer == AL_NONE)
    return false;

  // Generate a single source, attach the buffer to it and start playing. 
  ALuint source;
  alGenSources (1, &source);
  alSourcei (source, AL_BUFFER, al_buffer);
  alSourcePlay (source);

  // Normally nothing should go wrong above, but one never knows...
  ALenum error = alGetError();
  if (error != ALUT_ERROR_NO_ERROR)
    {
      err.Printf(_("Error playing AL buffer: "));
      err += wxString(alGetString(error), wxConvUTF8);
      status = false;
    }

  //  alDeleteSources (1, &source);

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





/*******************************************
 *  Klangset
 *******************************************/


// initialise primitive datatypes
Klangset::Klangset()
{
  version = 0;
  channels = 0;
}




bool Klangset::loadFile(const wxString& path)
{
  // open file
  wxFileInputStream archive(path);
  if(! archive.IsOk())
    {
      err.Printf(_("Could not open klangset '%s'.\n"), path.c_str());
      return false;
    }

  vector<char> cfgfile_buf;

  if(! fileFromZip(archive, wxT(KLW_CFGFILE), &cfgfile_buf))
    return false;

  // create a memory input stream out of the buffer
  // and use it to create a wxFileConfig object
  wxMemoryInputStream cfgstrm(&cfgfile_buf[0], cfgfile_buf.size());
  wxFileConfig cfg(cfgstrm);

  
  /*
    start reading
  */
  long in_long;
 
  if (! cfg.Read(wxT("Name"), &name))
    {
      err.Printf(_("Could not read name of klangset.\n"));
      return false;
    }
 
  if (! cfg.Read(wxT("Version"), &in_long))
    {
      err.Printf(_("Could not read version of klangset.\n"));
      return false;
    }
  else
    version = in_long;
 
  if (! cfg.Read(wxT("Channels"), &in_long))
    {
      err.Printf(_("Could not read number of channels of klangset.\n"));
      return false;
    }
  else
    channels = in_long;
 
   
  // get list of klangs
  cfg.SetPath(wxT("/Klangs/"));
  wxArrayString klangs;

  // enumeration variables
  wxString str; 
  long dummy;

  bool cont = cfg.GetFirstGroup(str, dummy);
  while ( cont ) 
    {
      klangs.Add(str);
      cont = cfg.GetNextGroup(str, dummy);
    }

    
  // read in each klang
  for(size_t i=0; i < klangs.GetCount(); ++i)
    {
      Klang k;

      k.name = klangs[i];

      cfg.SetPath(wxT("/Klangs/") + klangs[i]);

      if(!cfg.Read(wxT("P_init"), &k.p_init))
	{
	  err.Printf(_("Could not read initial propability of klang '%s'.\n"), klangs[i].c_str());
	  return false;
	}

      if(!cfg.Read(wxT("P_incr"), &k.p_incr))
	{
	  err.Printf(_("Could not read propability increment of klang '%s'.\n"), klangs[i].c_str());
	  return false;
	}
        
      if(!cfg.Read(wxT("P_decr"), &k.p_decr))
	{
	  err.Printf(_("Could not read propability decrement of klang '%s'.\n"), klangs[i].c_str());
	  return false;
	}

      if(!cfg.Read(wxT("Loops_min"), &in_long))
	{
	  err.Printf(_("Could not read minimum loop count of klang '%s'.\n"), klangs[i].c_str());
	  return false;
	}
      else
	k.loops_min = in_long;

      if(!cfg.Read(wxT("Loops_max"), &in_long))
	{
	  err.Printf(_("Could not read maximum loop count of klang '%s'.\n"), klangs[i].c_str());
	  return false;
	}
      else
	k.loops_max = in_long;
      
      if(!cfg.Read(wxT("Filename"), &k.filename))
	{
	  err.Printf(_("Could not read internal filename of klang '%s'.\n"), klangs[i].c_str());
	  return false;
	}

      // load associated file into buffer
      vector<char> sndfile_buf;
      if(! fileFromZip(archive, k.filename, &sndfile_buf))
	{
	  err.Printf(_("Could not read associated sound file of klang '%s'.\n"), klangs[i].c_str());
	  return false;
	}

      if(! k.loadSnd(sndfile_buf))
	{
	  err.Printf(_("Could not decode sound file of klang '%s'.\n"), klangs[i].c_str());
	  err += k.getErr();
	  return false;
	}
      
      // all fine, add it
      push_back(k);
    }

  return true;
}





bool Klangset::saveFile(const wxString& path)
{
  wxFileConfig cfg;
  
  cfg.Write(wxT("Name"), name);
  cfg.Write(wxT("Version"), (long)++version);
  cfg.Write(wxT("Channels"), (long)channels);
 

  // write config data for each klang
  for(Klangset::iterator it = begin(); it != end(); ++it)
    {
      cfg.SetPath(wxT("/Klangs/") + it->name);

      cfg.Write(wxT("Filename"), it->filename);
      cfg.Write(wxT("P_init"), it->p_init);
      cfg.Write(wxT("P_incr"), it->p_incr);
      cfg.Write(wxT("P_decr"), it->p_decr);
      cfg.Write(wxT("Loops_min"), (long) it->loops_min);
      cfg.Write(wxT("Loops_max"), (long) it->loops_max);
    }

  // write config into stream
  wxMemoryOutputStream cfgstrm;
  cfg.Save(cfgstrm);
  
  // get config stream's size
  cfgstrm.SeekO(0, wxFromEnd);
  size_t sz = cfgstrm.TellO();
  
  // create data block
  vector<char> buf(sz);
  cfgstrm.CopyTo(&buf[0], sz);

  // write config data into file
  wxFileOutputStream archive(path);
  if(! archive.IsOk())
    {
      err.Printf(_("Could not write klangset '%s' .\n"), path.c_str());
      return false;
    }

  wxZipOutputStream outzipstrm(archive);
  outzipstrm.SetComment(wxT("KLW archive created by Klangwunder3000 version "VERSION"."));
  
  if(!fileToZip(&outzipstrm, wxT(KLW_CFGFILE), buf))
    return false;

  // write file data for each klang
  for(Klangset::iterator it = begin(); it != end(); ++it)
    if(!fileToZip(&outzipstrm, it->filename, it->file_buffer))
      return false;
  
  // all well
  return true;
}









bool Klangset::fileFromZip(wxFileInputStream& filestrm, wxString filename, std::vector<char>* dest)
{
  // we have to create a new wxZipInputStream each time because 
  // we cannot rewind a wxZipInputStream
  wxZipInputStream zipstrm(filestrm);

  // convert the local name we are looking for into the internal format
  wxString name = wxZipEntry::GetInternalName(filename);

  auto_ptr<wxZipEntry> entry;
  // call GetNextEntry() until the required internal name is found
  do 
    entry.reset(zipstrm.GetNextEntry());
  while (entry.get() != 0 && entry->GetInternalName() != name);

  // we found it
  if(entry.get() != 0) 
    {
      size_t sz_entry = entry->GetSize();
      if(sz_entry == 0)
	{
	  err.Printf(_("File '%s' in klangset is empty.\n"), filename.c_str());
	  return false;
	}

      dest->resize(sz_entry);

      size_t i=0;
      while(i < sz_entry && !zipstrm.Eof())
	 {
	   dest->at(i) = zipstrm.GetC();
	   ++i;
	 }

      // check if read was ok
      if(i == sz_entry)
	return true;
      else
	{
	  err.Printf(_("Error reading '%s' in klangset.\nCorrupt file?\n"), filename.c_str());
	  return false;
	}
    }
  else 
    {
      err.Printf(_("Could not find '%s' in klangset.\n"), filename.c_str());
      return false;
    }
}





bool Klangset::fileToZip(wxZipOutputStream* zipstrm, wxString filename, std::vector<char>& src)
{
  wxZipEntry *entry =  new wxZipEntry(filename);
  entry->SetComment(wxT("Added by Klangwunder3000 version "VERSION"."));

  bool good=true;
  if(!zipstrm->PutNextEntry(entry))
    good=false;

  zipstrm->Write(&src[0], src.size());
  if(zipstrm->LastWrite() != src.size())
    good=false;

  if(!good)
    err.Printf(_("Could not add '%s' to klangset.\n"), filename.c_str());

  return good;
}






void Klangset::print() const
{
  cout << "\nCurrent Klangset: " << name.utf8_str() << endl;
  cout << "Version: " << version << endl;
  cout << "Channels: " << channels << endl;

  size_t i=0;
  for(Klangset::const_iterator it = begin(); it != end(); ++it)
    {
      ++i;
      cout <<  endl << i  << " - " <<  it->name.utf8_str() << endl;
      cout << "P now:  " << it->p_now << endl;
      cout << "P init: " << it->p_init << endl;
      cout << "P incr: " << it->p_incr << endl;
      cout << "P decr: " << it->p_decr << endl;
      cout << "Lp min: " << it->loops_min << endl;
      cout << "Lp max: " << it->loops_max << endl;
    }
  cout << endl;
}











// return random number between 0 and limit, EXCLUDING limit ...  
static inline size_t lRand(size_t limit)
{
  return limit != 0 ? rand() % limit : 0; 
}

