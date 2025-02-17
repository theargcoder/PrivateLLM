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
    private_llm_frame (wxWindow *parent);
};

class private_llm_notebook : public wxAuiNotebook
{
  public:
    private_llm_notebook (wxWindow *parent, long style);
};

class private_llm_window : public wxWindow
{
  public:
    private_llm_window (wxWindow *parent);

    ~private_llm_window ()
    {
        {
            std::lock_guard<std::mutex> lock (this->buffer_mutex);
            this->alive = false;
            this->new_data = true;
        }
        this->conditon.notify_one (); // Notify outside the lock

        if (this->writer.joinable ())
            {
                this->writer.join ();
            }
    }

  private:
    std::string markdown, HTML_data;

  private:
    wxString HTML_complete = wxString (FULL_DOC);

  private:
    std::thread writer;
    std::mutex buffer_mutex;          // for thread safety
    std::condition_variable conditon; // for thread coordination
    bool new_data = false;
    bool done = false;
    std::atomic<bool> alive{ true };

  private:
    bool isOllamaRunning ();
    void startOllama ();
    std::string execCommand (const char *cmd);
    void callOllama (const std::string &model_name, const std::string &prompt);

  private:
    static size_t WriteCallback (void *contents, size_t size, size_t nmemb,
                                 void *user_data);

  private:
    std::vector<std::string> ignore_pattern{ R"(\\()", R"(\\))", R"(\\[)",
                                             R"(\\])", R"(\()",  R"(\))",
                                             R"(\[)",  R"(\])",  R"(\\)" };
};
