// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#define NORMAL 0
#define VISUAL 1
#define INSERT 2
#define EXECUTE 3
#define REPLACE 4
#define SEARCH 5

#include <wx/aui/auibook.h>
#include <wx/event.h>
#include <wx/stc/stc.h>

class private_llm_frame : public wxFrame
{
  public:
    private_llm_frame (wxWindow *parent, int id = wxID_ANY,
                       wxString title = "Demo",
                       wxPoint pos = wxDefaultPosition,
                       wxSize size = wxDefaultSize,
                       int style = wxDEFAULT_FRAME_STYLE);

  private:
};

class private_llm_window : public wxAuiNotebook
{
  public:
    private_llm_window (wxWindow *parent, wxWindowID ID, wxPoint &pos,
                        wxSize &size, long style);

  private:
};
