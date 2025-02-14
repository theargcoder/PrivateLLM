// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "../HTML.cpp"

#include <atomic>
#include <regex>
#include <thread>

// markdown parsing (markdown string to HTML string)
#include "../libcmark/cmark.h"

// CURL for talking to pllam
#include <curl/curl.h>

// to parse json responses str into cpp
// and also generate json to input into ollama
// i wrote it lol check the docks its pretty decent
#include "../libcjparse/src/cjparse.cpp"
#include "../libcjparse/src/cjparse_json_generate.cpp"
#include "../libcjparse/src/cjparse_json_parser.cpp"

#include <wx/app.h>
#include <wx/aui/auibook.h>
#include <wx/event.h>
#include <wx/stc/stc.h>
#include <wx/webview.h>

class private_llm_frame : public wxFrame
{
  public:
    private_llm_frame (wxWindow *parent, int id = wxID_ANY,
                       wxString title = "Demo",
                       wxPoint pos = wxDefaultPosition,
                       wxSize size = wxDefaultSize,
                       int style = wxDEFAULT_FRAME_STYLE);

  private:
    std::string markdown, HTML_data;
    std::atomic<bool> cool_bruh = true;

  private:
    wxString HTML_complete = wxString (FULL_DOC);
};

class private_llm_window : public wxAuiNotebook
{
  public:
    private_llm_window (wxWindow *parent, wxWindowID ID, wxPoint &pos,
                        wxSize &size, long style);

  private:
};
