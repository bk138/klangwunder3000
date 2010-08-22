
#include <iostream>
#include <memory>
#include "wx/fileconf.h"
#include "wx/mstream.h"
#include "wx/intl.h"
#include "Klangset.h"
#include "KW3KApp.h"

using namespace std;




/*
  internal constants
*/
#define KLW_CFGFILE "klangwunder.cfg" 
#define PLAY_TIMER_INTERVAL 100
#define PLAY_TIMER_ID 0



// map recv of events to handler methods
BEGIN_EVENT_TABLE(Klangset, wxEvtHandler)
  EVT_TIMER   (PLAY_TIMER_ID, Klangset::onPlayTimer)
END_EVENT_TABLE()



/*
  internal functions
*/
static inline size_t lRand(size_t limit);






// initialise primitive datatypes
Klangset::Klangset()
{
  version = 0;
  channels = 0;
  play_timer.SetOwner(this, PLAY_TIMER_ID);
  play_timer.Start(PLAY_TIMER_INTERVAL);
}




void Klangset::onPlayTimer(wxTimerEvent& event)
{


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
    if(!fileToZip(&outzipstrm, it->filename, it->getFileBuffer()))
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





bool Klangset::fileToZip(wxZipOutputStream* zipstrm, wxString filename, const std::vector<char>& src)
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

