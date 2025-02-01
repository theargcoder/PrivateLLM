#include "../include/main.h"

private_llm_frame::private_llm_frame (wxWindow *parent, int id, wxString title,
                                      wxPoint pos, wxSize size, int style)
    : wxFrame (parent, id, title, pos, size, style)
{
}

class myApp : public wxApp
{
  public:
    virtual bool
    OnInit ()
    {
        private_llm_frame *frame = new private_llm_frame (NULL);
        frame->Show ();
        frame->SetSize (wxSize (500, 500));
        frame->SetMinClientSize (wxSize (500, 500));
        return true;
    }
};

wxIMPLEMENT_APP (myApp);
