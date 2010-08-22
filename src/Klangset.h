// -*- C++ -*- 

#ifndef KLANGSET_H
#define KLANGSET_H

#include <deque>
#include <vector>
#include "wx/string.h"
#include "wx/wfstream.h"
#include "wx/zipstrm.h"
#include "Klang.h"


class Klangset: public std::deque<Klang>
{
  wxString err;

  bool fileFromZip(wxFileInputStream& filestrm, wxString filename, std::vector<char>* dest);
  bool fileToZip(wxZipOutputStream* zipstrm, wxString filename, const std::vector<char>& src);


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












