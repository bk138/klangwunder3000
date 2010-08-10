
#include "wx/aboutdlg.h"

#include "res/about.png.h"

#include "MyFrameMain.h"
#include "../KW3KApp.h"





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






MyFrameMain::MyFrameMain(wxWindow* parent, int id, const wxString& title, 
			 const wxPoint& pos,
			 const wxSize& size,
			 long style):
  FrameMain(parent, id, title, pos, size, style)	
{
  /*
    disable some menu items for a new frame
    unfortunately there seems to be a bug in wxMenu::FindItem(str): it skips '&' characters,
    but GTK uses '_' for accelerators and these are not trimmed...
  */
  // this is "Save"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(2)->Enable(false);
  // "Save as"
  frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(3)->Enable(false);

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
  button_play->Enable(false);
  button_pause->Enable(false);
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

  // set current prob column to read-only
  wxGridCellAttr *a = new wxGridCellAttr;
  a->SetReadOnly();
  grid_klangs->SetColAttr(1, a);

  /*
      this is really cool ;-)
  */
  this->SetDropTarget(new MyFileDropTarget(this));
}



/*
  handler functions
*/

void MyFrameMain::klangset_new(wxCommandEvent &event)
{
  wxString newname = wxGetTextFromUser(_("How should the new klangset be named?"),
				       _("Enter klangset name"),
				       wxEmptyString,
				       this);
 if(!newname.empty())
   {
      delete wxGetApp().ks_now; // if there's already one open, delete it
      
      wxGetApp().ks_now = new Klangset;
      wxGetApp().ks_now->name = newname;

      wxString dir = wxFileName::GetCwd();
      if(dir.empty())
	dir = wxFileName::GetHomeDir();
      wxGetApp().ks_now_path = dir + wxFileName::GetPathSeparator() + wxGetApp().ks_now->name + wxT(".klw");

      /*
	enable menu items
      */
      // this is "Save"
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(2)->Enable(true);
      // "Save as"
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(3)->Enable(true);
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

      SetTitle(wxT("Klangwunder3000 - ") + wxGetApp().ks_now->name + 
	       wxString(wxT(" version ")) << wxGetApp().ks_now->version);


      /*
	enable menu items
      */
      // this is "Save"
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(2)->Enable(true);
      // "Save as"
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("File")))->FindItemByPosition(3)->Enable(true);
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

	  button_play->Enable(true);
	  button_pause->Enable(true);	
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
      Klang k;
      
      int pos = grid_klangs->GetGridCursorRow();
      pos =  pos < 0 ? 0 : pos;
      
      // insert into klangset and grid
      wxGetApp().ks_now->insert(wxGetApp().ks_now->begin() + pos, k);
      grid_klangs->InsertRows(pos); // new row
      klangset2grid(pos);           // fill it

      // there's sth. to remove or get info on or to play now
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(1)->Enable(true);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(2)->Enable(true);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(3)->Enable(true);

      button_play->Enable(true);
      button_pause->Enable(true);
      button_remove->Enable(true);
      button_info->Enable(true);
      button_playklang->Enable(true);
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

  // disable "remove", "info" and "play" if nothing left
  if(wxGetApp().ks_now->size() < 1)
    {
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(1)->Enable(false);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(2)->Enable(false);
      frame_main_menubar->GetMenu(frame_main_menubar->FindMenu(wxT("Edit")))->FindItemByPosition(3)->Enable(false);
      button_play->Enable(false);
      button_pause->Enable(false);
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
	       _("Duration: ") + duration + wxT("\n") +
	       _("Channels: ") + channels + wxT("\n") +
	       _("Sample Rate: ") + samplerate,
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

  if(!wxGetApp().ks_now->at(pos).playSnd())
    wxLogError(wxGetApp().ks_now->at(pos).getErr());
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
