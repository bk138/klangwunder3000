
#ifndef KW3KAPP
#define KW3KAPP


#include <wx/wxprec.h>
#ifndef WX_PRECOMP 
#include "wx/wx.h"       
#endif  

#include "Klangset.h"


#define VERSION "0.1"
#define COPYRIGHT "Copyright (C) 2007-2010 Christian Beier <dontmind@freeshell.org>"


class KW3KApp: public wxApp 
{
  wxLocale *locale;
public:
  Klangset *ks_now;
  wxString ks_now_path;

  bool OnInit();
  bool setLocale(int language);
};


DECLARE_APP(KW3KApp);





#endif // KW3KAPP
