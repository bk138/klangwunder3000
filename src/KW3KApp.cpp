/*


*/

#include <iostream>
extern "C" {
#include "avcodec.h"
#include "avformat.h"
}
#include "AL/alut.h"
#include "KW3KApp.h"
#include "gui/MyFrameMain.h"



using namespace std;



// this also sets up main()
IMPLEMENT_APP(KW3KApp);




bool KW3KApp::OnInit()
{
  locale = 0;
  ks_now = 0;
  
  setLocale(wxLANGUAGE_DEFAULT);

  // greetings to anyone who made it...
  cout << "\n:::  klangwunder3000 welcomes you!  :::\n\n";
  cout << COPYRIGHT << ".\n";
  cout << "klangwunder3000 is free software, licensed unter the GPL.\n\n";

  // register all libavformat formats and codecs
  av_register_all();

  // Initialize OpenAL and clear the error bit.
  alutInit(&argc, (char**)argv);
  alGetError();

  // wx stuff
  wxInitAllImageHandlers();
  MyFrameMain* frame_main = new MyFrameMain(NULL, wxID_ANY, wxEmptyString);
  SetTopWindow(frame_main);
  frame_main->Show();
  return true;
}



int KW3KApp::OnExit()
{
  alutExit();

  // clean up: Set() returns the active config object as Get() does, but unlike
  // Get() it doesn't try to create one if there is none (definitely not what
  // we want here!)
  delete wxConfigBase::Set((wxConfigBase *) NULL);

  return 0;
}




bool KW3KApp::setLocale(int language)
{
  delete locale;
     
  locale = new wxLocale;

  // don't use wxLOCALE_LOAD_DEFAULT flag so that Init() doesn't return                                          
  // false just because it failed to load wxstd catalog                                                          
  if(! locale->Init(language, wxLOCALE_CONV_ENCODING) )                                                    
    {                                                                                                              
      wxLogError(_("This language is not supported by the system.")); 
      return false;                                                                                              
    }            

  // normally this wouldn't be necessary as the catalog files would be found                                         
  // in the default locations, but when the program is not installed the                                             
  // catalogs are in the build directory where we wouldn't find them by                                              
  // default                                                                                                         
  wxLocale::AddCatalogLookupPathPrefix(wxT("."));                                                                    
                                                                                                                       
  // Initialize the catalogs we'll be using                                                                          
  locale->AddCatalog(wxT("klangwunder3000"));  
  
  return true;
}
