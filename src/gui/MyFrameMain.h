// -*- C++ -*- 

#ifndef MYFRAMEMAIN_H
#define MYFRAMEMAIN_H

#include "wx/dnd.h"

#include "../Klangset.h"
#include "FrameMain.h"



class MyFrameMain: public FrameMain
{
  class MyFileDropTarget;
  friend class MyFileDropTarget;

  class MyFileDropTarget: public wxFileDropTarget 
  {
    MyFrameMain *owner;
  public:
    MyFileDropTarget(MyFrameMain *caller);
    bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);
  };

  // internal real open functions
  void openKlangset(wxString& path);

  // grid <-> data structure conversion
  void klangset2grid(int begin, int end = -1);
  void grid2klangset();

  // private handlers
  void onClose(wxCloseEvent& event);
  
protected:
  DECLARE_EVENT_TABLE();

public:
  MyFrameMain(wxWindow* parent, int id, const wxString& title, 
	      const wxPoint& pos=wxDefaultPosition, 
	      const wxSize& size=wxDefaultSize, 
	      long style=wxDEFAULT_FRAME_STYLE);
  ~MyFrameMain();

 
  // handlers
  void klangset_new(wxCommandEvent &event); 
  void klangset_open(wxCommandEvent &event);
  void klangset_save(wxCommandEvent &event);
  void klangset_saveas(wxCommandEvent &event);

  void klang_add(wxCommandEvent &event);
  void klang_remove(wxCommandEvent &event); 
  void klang_info(wxCommandEvent &event); 
  void klang_play(wxCommandEvent &event); 

  void vol_change(wxScrollEvent &event);

  void grid_change(wxGridEvent &event);

  void help_about(wxCommandEvent &event);

  void app_exit(wxCommandEvent &event);

};



#endif
