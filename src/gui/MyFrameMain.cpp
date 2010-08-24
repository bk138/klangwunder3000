
#include "wx/aboutdlg.h"
#include "res/about.png.h"
#include "MyFrameMain.h"
#include "../KW3KApp.h"
#include "../dfltcfg.h"

#define UPDATE_TIMER_INTERVAL 500
#define UPDATE_TIMER_ID 1


// map recv of events to handler methods
BEGIN_EVENT_TABLE(MyFrameMain, FrameMain)
   EVT_CLOSE (MyFrameMain::onClose)
   EVT_TIMER (UPDATE_TIMER_ID, MyFrameMain::onUpdateTimer)
END_EVENT_TABLE()


/*
  internal file drop class

*/

MyFrameMain::MyFileDropTarget::MyFileDropTarget(MyFrameMain *caller)
{
  owner = caller;
}
    
bool MyFrameMain::MyFileDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
  if(wxFile::Exists(filenames[0]))
    {
      // check if its a KLW or other
      if(wxFileName(filenames[0]).GetExt().IsSameAs(wxT("klw"), false))//case insensitive
	owner->openKlangset(filenames[0]);
      else
	wxLogError(wxT("no klw"));
      return true;
    }
  else
    return false;
}



/*
  constructor /destructor

*/

MyFrameMain::MyFrameMain(wxWindow* parent, int id, const wxString& title, 
			 const wxPoint& pos,
			 const wxSize& size,
			 long style):
  FrameMain(parent, id, title, pos, size, style)	
{
  int x,y, vol;
  // get default config object, created on demand if not exist
  wxConfigBase *pConfig = wxConfigBase::Get();
  pConfig->Read(K_VOLUME, &vol, V_VOLUME);
  pConfig->Read(K_SIZE_X, &x, V_SIZE_X);
  pConfig->Read(K_SIZE_Y, &y, V_SIZE_Y);

  slider_vol->SetValue(vol);

  // window size
  SetMinSize(wxSize(640, 480));
  SetSize(x, y);


  /*
    disable some menu items for a new frame
    unfortunately there seems to be a bug in wxMenu::FindItem(str): it skips '&' characters,
    but GTK uses '_' for accelerators and these are not trimmed...
  */
  // this is "Save"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(2)->Enable(false);
  // "Save as"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(3)->Enable(false);
  // "Close"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(4)->Enable(false);

  // "add klang
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(0)->Enable(false);
  // "remove klang"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(1)->Enable(false);
  // "info on klang"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(2)->Enable(false);
  // "play klang"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(3)->Enable(false);

  
  /*
    and disable buttons
  */
  button_playpause->Enable(false);
  button_stop->Enable(false);
  button_add->Enable(false);
  button_remove->Enable(false);
  button_info->Enable(false);
  button_playklang->Enable(false);



  /*
    setup of the grid
  */

  // and change label font of grid
  wxFont lf = grid_klangs->GetLabelFont();
  lf.SetWeight(wxFONTWEIGHT_NORMAL);
  grid_klangs->SetLabelFont(lf);

  update_timer.SetOwner(this, UPDATE_TIMER_ID);
  update_timer.Start(UPDATE_TIMER_INTERVAL);

  // set current prob column to read-only
  wxGridCellAttr *a = new wxGridCellAttr;
  a->SetReadOnly();
  grid_klangs->SetColAttr(1, a);

  /*
      this is really cool ;-)
  */
  this->SetDropTarget(new MyFileDropTarget(this));
}




MyFrameMain::~MyFrameMain()
{
  wxConfigBase *pConfig = wxConfigBase::Get();
  int x,y;
  GetSize(&x, &y);
  pConfig->Write(K_SIZE_X, x);
  pConfig->Write(K_SIZE_Y, y);
  pConfig->Write(K_VOLUME, slider_vol->GetValue());
}



/*
  private functions
  
*/

void MyFrameMain::onClose(wxCloseEvent& event)
{
  if(wxGetApp().ks_now)
    {
      if(wxGetApp().ks_now_changed)
	{
	  int answer = wxMessageBox(_("Klangset changed. Save it before exiting?"), _("Save changed klangset?"),
				    wxYES_NO|wxCANCEL|wxICON_QUESTION, this);
	  if(answer == wxCANCEL)
	    {
	      if(event.CanVeto()) // i.e. not called from user code
		event.Veto();     // do not close
	      return;
	    }
	  if(answer == wxYES)
	    {
	      wxCommandEvent unused;
	      klangset_save(unused);
	    }
	}

      delete wxGetApp().ks_now; 
      wxGetApp().ks_now = 0; 
    }
  
  Destroy();
}




void MyFrameMain::onUpdateTimer(wxTimerEvent& event)
{
  if(!wxGetApp().ks_now)
    return;
    
  for(size_t row = 0; row < wxGetApp().ks_now->size(); ++row)
    {
      Klang k = wxGetApp().ks_now->at(row);
      grid_klangs->SetCellValue(row, 1, wxString() << k.p_now);
    }
}



void MyFrameMain::klangset2grid(int begin, int end)
{
  if(end < 0)
    end = begin + 1;

  for(int row = begin; row < end; ++row)
    {
      Klang k = wxGetApp().ks_now->at(row);

      grid_klangs->SetCellValue(row, 0, k.name);
      grid_klangs->SetCellValue(row, 1, wxString() << k.p_now);
      grid_klangs->SetCellValue(row, 2, wxString() << k.p_init);
      grid_klangs->SetCellValue(row, 3, wxString() << k.p_incr);
      grid_klangs->SetCellValue(row, 4, wxString() << k.p_decr);
      grid_klangs->SetCellValue(row, 5, wxString() << k.loops_min);
      grid_klangs->SetCellValue(row, 6, wxString() << k.loops_max);
    }

  grid_klangs->AutoSizeColumns();
}




void MyFrameMain::grid2klangset()
{
  // Sets the value of the current grid cell to the current in-place edit control value. 
  // This is called automatically when the grid cursor moves from the current cell to a new cell.
  grid_klangs->SaveEditControlValue();

  long in_long = 0;

  int row = 0;
  for(Klangset::iterator it = wxGetApp().ks_now->begin(); it != wxGetApp().ks_now->end(); ++it)
    {
      it->name = grid_klangs->GetCellValue(row, 0);

      grid_klangs->GetCellValue(row, 2).ToDouble(&it->p_init);
      grid_klangs->GetCellValue(row, 3).ToDouble(&it->p_incr);
      grid_klangs->GetCellValue(row, 4).ToDouble(&it->p_decr);

      grid_klangs->GetCellValue(row, 5).ToLong(&in_long);
      it->loops_min = in_long;
      grid_klangs->GetCellValue(row, 6).ToLong(&in_long);
      it->loops_max = in_long;
	
      ++row;
    }
}






void MyFrameMain::openKlangset(wxString& path)
{
  wxBusyCursor busy;

  // if there's already one open, clear the grid and delete it
  if(wxGetApp().ks_now)
    {
      grid_klangs->DeleteRows(0, wxGetApp().ks_now->size());
      SetTitle(wxT("Klangwunder3000"));
      delete wxGetApp().ks_now; 
    }


  wxGetApp().ks_now = new Klangset;


  /*
    a call to loadFile or saveFile has to be with english locale
    because of otherwise different config file formats :-(
  */
  wxGetApp().setLocale(wxLANGUAGE_ENGLISH);

  if(wxGetApp().ks_now->loadFile(path))
    {
      // set the path based on internal klangset name, not filename
      // so construct our path based on dir file resides in and its klangset name
      wxGetApp().ks_now_path = wxFileName(path).GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR) 
	+ wxGetApp().ks_now->name + wxT(".klw");

      // not modified yet
      wxGetApp().ks_now_changed = false;

      SetTitle(wxT("Klangwunder3000 - ") + wxGetApp().ks_now->name + 
	       wxString(wxT(" version ")) << wxGetApp().ks_now->version);


      /*
	enable menu items
      */
      // this is "Save"
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(2)->Enable(true);
      // "Save as"
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(3)->Enable(true);
      // "Close"
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(4)->Enable(true);
      // "add klang
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(0)->Enable(true);

      /*
	and enable buttons
      */
      button_add->Enable(true);

   
      // enable "remove klang" && "info on klang"
      // and update the grid ...
      if(wxGetApp().ks_now->size() > 0)
	{
	  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))
	    ->FindItemByPosition(1)->Enable(true);
	  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))
	    ->FindItemByPosition(2)->Enable(true);
	  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))
	    ->FindItemByPosition(3)->Enable(true);

	  button_playpause->Enable(true);
	  button_stop->Enable(true);	
	  button_remove->Enable(true);
	  button_info->Enable(true);
	  button_playklang->Enable(true);

	  // show it
	  grid_klangs->AppendRows(wxGetApp().ks_now->size()); 
	  klangset2grid(0, wxGetApp().ks_now->size());        
	}
	      
      // dump to stdout
      wxGetApp().ks_now->print();
    }
  else
    { 
      wxLogError(wxGetApp().ks_now->getErr()); 
      delete wxGetApp().ks_now;
      wxGetApp().ks_now = 0;
    }

  /*
    and set it back
  */
  wxGetApp().setLocale(wxLANGUAGE_DEFAULT);
}







/*
  handler functions
*/

void MyFrameMain::klangset_new(wxCommandEvent &event)
{
  if(wxGetApp().ks_now && wxGetApp().ks_now_changed)
    {
      int answer = wxMessageBox(_("Klangset changed. Save it before continuing?"), _("Save changed klangset?"),
				wxYES_NO|wxCANCEL|wxICON_QUESTION, this);

      if(answer == wxCANCEL)
	return;
      if(answer == wxYES)
	{
	  wxCommandEvent unused;
	  klangset_save(unused);
	}
    }


  wxString newname = wxGetTextFromUser(_("How should the new klangset be named?"),
				       _("Enter klangset name"),
				       wxEmptyString,
				       this);
  if(!newname.empty())
    {
      if(wxGetApp().ks_now)
	delete wxGetApp().ks_now; // if there's already one open, delete it
      
      wxGetApp().ks_now = new Klangset;
      wxGetApp().ks_now->name = newname;

      wxString dir = wxFileName::GetCwd();
      if(dir.empty())
	dir = wxFileName::GetHomeDir();
      wxGetApp().ks_now_path = dir + wxFileName::GetPathSeparator() + wxGetApp().ks_now->name + wxT(".klw");
     
      wxGetApp().ks_now_changed = false;


      /*
	enable menu items
      */
      // this is "Save"
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(2)->Enable(true);
      // "Save as"
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(3)->Enable(true);
      // "Close"
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(4)->Enable(true);
      // "add klang
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(0)->Enable(true);

      /*
	and enable button
      */
      button_add->Enable(true);

      SetTitle(wxT("Klangwunder3000 - ") + wxGetApp().ks_now->name + 
	       wxString(wxT(" v. ")) << wxGetApp().ks_now->version);
    }
}





void MyFrameMain::klangset_open(wxCommandEvent &event)
{	
  if(wxGetApp().ks_now && wxGetApp().ks_now_changed)
    {
      int answer = wxMessageBox(_("Klangset changed. Save it before continuing?"), _("Save changed klangset?"),
				wxYES_NO|wxCANCEL|wxICON_QUESTION, this);

      if(answer == wxCANCEL)
	return;
      if(answer == wxYES)
	{
	  wxCommandEvent unused;
	  klangset_save(unused);
	}
    }


  wxString path = wxFileSelector(_("Choose a file to open"), 
				 wxEmptyString,
				 wxEmptyString,
				 wxEmptyString,
				 _("Klangset (*.klw)|*.klw|All files (*.*)|*.*"),
				 wxFD_OPEN | wxFD_FILE_MUST_EXIST,
				 this);

  if(!path.empty())
    openKlangset(path);
}






void MyFrameMain::klangset_save(wxCommandEvent &event)
{
  if(!wxGetApp().ks_now)
    return;
  
  if(wxFile::Exists(wxGetApp().ks_now_path))
    if(wxMessageBox(_("File already exists. Overwrite it?"), _("Overwrite?"), wxYES_NO|wxICON_QUESTION, this) == wxNO)
      return;

  wxBusyCursor busy;

  // transfer grid values to klangset
  grid2klangset();

  /*
    a call to loadFile or saveFile has to be with english locale
    because of otherwise different config file formats :-(
  */
  wxGetApp().setLocale(wxLANGUAGE_ENGLISH);
  
  if(!wxGetApp().ks_now->saveFile(wxGetApp().ks_now_path))
    wxLogError(wxGetApp().ks_now->getErr()); 

  /*
    and set it back
  */
  wxGetApp().setLocale(wxLANGUAGE_DEFAULT); 

  SetTitle(wxT("Klangwunder3000 - ") + wxGetApp().ks_now->name + 
	   wxString(wxT(" v. ")) << wxGetApp().ks_now->version);

  // now saved state == state in gui
  wxGetApp().ks_now_changed = false;
}





void MyFrameMain::klangset_saveas(wxCommandEvent &event)
{
  wxString path = wxFileSelector(_("Choose a file to save to"), 
				 wxEmptyString,
				 wxEmptyString,
				 wxEmptyString,
				 _("Klangset (*.klw)|*.klw|All files (*.*)|*.*"),
				 wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
				 this);

  // back up these in case sth. goes wrong
  wxString oldname = wxGetApp().ks_now->name;
  wxString oldpath = wxGetApp().ks_now_path;
  size_t oldversion = wxGetApp().ks_now->version;

  if(!path.empty())
    {
      wxBusyCursor busy;

      // transfer grid values to klangset
      grid2klangset();

      // construct new internal name from new filename
      wxGetApp().ks_now->name = wxFileName(path).GetName();

      // new name, version
      if(! oldname.IsSameAs(wxGetApp().ks_now->name) )
	wxGetApp().ks_now->version = 0;
	
      // and make sure file is saved with right extension
      wxGetApp().ks_now_path = wxFileName(path).GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR) 
	+ wxFileName(path).GetName() + wxT(".klw");

 
      /*
	a call to loadFile or saveFile has to be with english locale
	because of otherwise different config file formats :-(
      */
      wxGetApp().setLocale(wxLANGUAGE_ENGLISH);

      // reset to old values if sth. went wrong
      if(!wxGetApp().ks_now->saveFile(wxGetApp().ks_now_path))
	{
	  wxLogError(wxGetApp().ks_now->getErr()); 
	  wxGetApp().ks_now->name = oldname;
	  wxGetApp().ks_now_path = oldpath;
	  wxGetApp().ks_now->version = oldversion;
	}
    
      /*
	and set it back
      */
      wxGetApp().setLocale(wxLANGUAGE_DEFAULT); 

      // all well, set title
      SetTitle(wxT("Klangwunder3000 - ") + wxGetApp().ks_now->name + 
	       wxString(wxT(" v. ")) << wxGetApp().ks_now->version);

      // now saved state == state in gui
      wxGetApp().ks_now_changed = false;
    }
}



void MyFrameMain::klangset_close(wxCommandEvent &event)
{
  if(wxGetApp().ks_now && wxGetApp().ks_now_changed)
    {
      int answer = wxMessageBox(_("Klangset changed. Save it before continuing?"), _("Save changed klangset?"),
				wxYES_NO|wxCANCEL|wxICON_QUESTION, this);
      
      if(answer == wxCANCEL)
	return;
      if(answer == wxYES)
	{
	  wxCommandEvent unused;
	  klangset_save(unused);
	}
    }

  if(wxGetApp().ks_now) 
    {
      delete wxGetApp().ks_now; 
      wxGetApp().ks_now = 0; 
      grid_klangs->DeleteRows(0, grid_klangs->GetNumberRows());
    }
  

  // this is "Save"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(2)->Enable(false);
  // "Save as"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(3)->Enable(false);
  // "Close"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(4)->Enable(false);

  // "add klang
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(0)->Enable(false);
  // "remove klang"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(1)->Enable(false);
  // "info on klang"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(2)->Enable(false);
  // "play klang"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(3)->Enable(false);

  
  /*
    and disable buttons
  */
  button_playpause->Enable(false);
  button_stop->Enable(false);
  button_add->Enable(false);
  button_remove->Enable(false);
  button_info->Enable(false);
  button_playklang->Enable(false);

  SetTitle(wxT("Klangwunder3000"));
}



void MyFrameMain::klangset_playpause(wxCommandEvent &event)
{
  if(wxGetApp().ks_now)
    {
      if(wxGetApp().ks_now->getStatus() == KLANGSET_PLAYING)
	{
	  wxGetApp().ks_now->pause();
	  button_playpause->SetBitmapLabel(bitmapFromMem(play_png));
	}
      else
	{
	  wxGetApp().ks_now->play();
	  button_playpause->SetBitmapLabel(bitmapFromMem(pause_png));
	}
    }
}


void MyFrameMain::klangset_stop(wxCommandEvent &event)
{
  if(wxGetApp().ks_now)
    {
      wxGetApp().ks_now->stop();
      button_playpause->SetBitmapLabel(bitmapFromMem(play_png));
    }
}




void MyFrameMain::klang_add(wxCommandEvent &event)
{
  if(!wxGetApp().ks_now)
    return;

  wxString path = wxFileSelector(_("Choose a file to open"), 
				 wxEmptyString,
				 wxEmptyString,
				 wxEmptyString,
				 _("All files (*.*)|*.*"),
				 wxFD_OPEN | wxFD_FILE_MUST_EXIST,
				 this);

  if(!path.empty())
    {
      wxBusyCursor busy;
      wxLogStatus(_("Loading"));

      Klang k;
      
      // load sound file into klang
      k.filename = path.AfterLast(wxT('/'));
      wxFileInputStream sndfile(path);
      if(!sndfile.IsOk())
	{
	  wxLogError(_("Could not open file!"));
	  return;
	}
      std::vector<char> sndfilebuf;
      while(!sndfile.Eof())
	sndfilebuf.push_back(sndfile.GetC());
      if(!k.loadSnd(sndfilebuf))
	{
	  wxLogError(_("Could not load file!"));
	  return;
	} 
      

      // insert into klangset and grid
      int pos = grid_klangs->GetGridCursorRow();
      pos =  pos < 0 ? 0 : pos;
      wxGetApp().ks_now->insert(wxGetApp().ks_now->begin() + pos, k);
      grid_klangs->InsertRows(pos); // new row
      klangset2grid(pos);           // fill it

      // now saved state != state in gui
      wxGetApp().ks_now_changed = true;

      // there's sth. to remove or get info on or to play now
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(1)->Enable(true);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(2)->Enable(true);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(3)->Enable(true);

      button_playpause->Enable(true);
      button_stop->Enable(true);
      button_remove->Enable(true);
      button_info->Enable(true);
      button_playklang->Enable(true);

      wxLogStatus(_("Loaded successfully"));
    }
}





void MyFrameMain::klang_remove(wxCommandEvent &event)
{
  if(!wxGetApp().ks_now)
    return;

  if(!wxGetApp().ks_now->size())
    return;

  int pos = grid_klangs->GetGridCursorRow();
  if(pos < 0) // invalid position
    return;
      
  // remove klangset and grid row
  wxGetApp().ks_now->erase(wxGetApp().ks_now->begin() + pos);
  grid_klangs->DeleteRows(pos);   

  // now saved state != state in gui
  wxGetApp().ks_now_changed = true;

  // disable "remove", "info" and "play" if nothing left
  if(wxGetApp().ks_now->size() < 1)
    {
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(1)->Enable(false);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(2)->Enable(false);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(3)->Enable(false);
      button_playpause->Enable(false);
      button_stop->Enable(false);
      button_remove->Enable(false);
      button_info->Enable(false);
      button_playklang->Enable(false);
    }
}




void MyFrameMain::klang_info(wxCommandEvent &event)
{
  if(!wxGetApp().ks_now)
    return;

  wxGetApp().ks_now->print();

  if(!wxGetApp().ks_now->size())
    return;

  int pos = grid_klangs->GetGridCursorRow();
  if(pos < 0) // invalid position
    return;

  wxString name = wxGetApp().ks_now->at(pos).name;
  wxString filename = wxGetApp().ks_now->at(pos).filename;
  wxString duration = wxString::Format(wxT("%4.2f"), wxGetApp().ks_now->at(pos).getDuration()) + wxT(" s");
  wxString channels = wxString()<< wxGetApp().ks_now->at(pos).getChannels();
  wxString samplerate = wxString() << wxGetApp().ks_now->at(pos).getSampleRate();

  wxMessageBox(_("Name:              ") + name + wxT("\n") +
	       _("Internal name: ") + filename + wxT("\n") +
	       _("Duration:         ") + duration + wxT("\n") +
	       _("Channels:        ") + channels + wxT("\n") +
	       _("Sample Rate:   ") + samplerate,
	       _("Info on Klang"),
	       wxOK | wxICON_INFORMATION,
	       this);
}



void MyFrameMain::klang_play(wxCommandEvent &event)
{
  if(!wxGetApp().ks_now)
    return;

  if(!wxGetApp().ks_now->size())
    return;

  int pos = grid_klangs->GetGridCursorRow();
  if(pos < 0) // invalid position
    return;

  if(!wxGetApp().ks_now->at(pos).playStatic())
    wxLogError(wxGetApp().ks_now->at(pos).getErr());
}



void MyFrameMain::vol_change(wxScrollEvent &event)
{
  alListenerf(AL_GAIN, slider_vol->GetValue()/100.0);
}



void MyFrameMain::grid_change(wxGridEvent &event)
{
  // now saved state != state in gui
  wxGetApp().ks_now_changed = true;
}


  
  


void MyFrameMain::help_about(wxCommandEvent &event)
{	
  wxAboutDialogInfo info;
  wxIcon icon;
  icon.CopyFromBitmap(bitmapFromMem(about_png));

  info.SetIcon(icon);
  info.SetName(wxT("Klangwunder3000"));
  info.SetVersion(wxT(VERSION));
  info.SetDescription(_("This is Klangwunder3000, the good-natured ambient audio generator.\n"));
  info.SetCopyright(wxT(COPYRIGHT));
  
  wxAboutBox(info); 
}







void MyFrameMain::app_exit(wxCommandEvent &event)
{
  Close(true);
}



