// -*- C++ -*- 

#ifndef KLANGSET_H
#define KLANGSET_H


#include <deque>
#include <vector>
#include "wx/string.h"
#include "wx/wfstream.h"
#include "wx/zipstrm.h"
#include "AL/alut.h"



struct Klang
{
  wxString err;

  std::vector<char> file_buffer; // the encoded sound file
  std::vector<char> data_buffer; // the decoded raw sound data

public:
  double p_init;
  double p_incr;
  double p_decr;
  double p_now;
  size_t loops_min;
  size_t loops_max;
  wxString filename;
  wxString name;

  ALuint al_buffer;

  Klang();
  ~Klang();
  Klang(const Klang& k);

  bool loadSnd(std::vector<char>& src);
  bool playSnd(); // simply plays the sound for debug purposes
  float getDuration() const;
  int getSampleRate() const;
  int getChannels() const;
  const wxString& getErr() const { const wxString& ref = err; return ref; };
};



class Klangset: public std::deque<Klang>
{
  wxString err;

  bool fileFromZip(wxFileInputStream& filestrm, wxString filename, std::vector<char>* dest);
  bool fileToZip(wxZipOutputStream* zipstrm, wxString filename, std::vector<char>& src);


 public:
  wxString name;
  size_t version; 
  size_t channels;

  Klangset();

  bool loadFile(const wxString& path);
  bool saveFile(const wxString& path);

  void print() const;
  const wxString& getErr() const { const wxString& ref = err; return ref; };
};



#endif //KLANGSET_H












