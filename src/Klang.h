// -*- C++ -*- 

#ifndef KLANG_H
#define KLANG_H

#include <vector>
#include "wx/string.h"
#include "AL/alut.h"


class Klang
{
  wxString err;

  std::vector<char> file_buffer; // the encoded sound file
  std::vector<char> data_buffer; // the decoded raw sound data

  ALuint static_source;
  ALuint dynamic_source;

public:
  uint8_t p_init;
  uint8_t p_incr;
  uint8_t p_decr;
  uint8_t p_now;
  size_t loops_min;
  size_t loops_max;
  wxString filename;
  wxString name;

  ALuint al_buffer;

  Klang();
  ~Klang();
  Klang(const Klang& k);

  bool loadSnd(std::vector<char>& src);
  bool playStatic(bool play); // simply plays the sound for debug purposes
  bool isPlayingStatic() const;
  bool playDynamic(bool play, ALfloat x=0, ALfloat y=0, ALfloat z=0); // plays sound at given position
  bool isPlayingDynamic() const;
  float getDuration() const;
  int getSampleRate() const;
  int getChannels() const;
  const std::vector<char>& getFileBuffer() const { const std::vector<char>& ref = file_buffer; return ref; };
  const wxString& getErr() const { const wxString& ref = err; return ref; };
};


#endif // KLANG_H
