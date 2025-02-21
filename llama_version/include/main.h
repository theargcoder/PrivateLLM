// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "../HTML.cpp"
#include "../HTML_conversation_block.cpp"

#include <atomic>
#include <regex>
#include <thread>

// markdown parsing (markdown string to HTML string)
#include "../../libcmark/cmark.h"

// CURL for talking to pllam
#include <curl/curl.h>

// to parse json responses str into cpp
// and also generate json to input into ollama
// i wrote it lol check the docks its pretty decent
#include "../../libcjparse/src/cjparse.cpp"
#include "../../libcjparse/src/cjparse_json_generate.cpp"
#include "../../libcjparse/src/cjparse_json_parser.cpp"

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
            this->done = true;
            this->new_data = true;
        }
        this->conditon.notify_one (); // Notify outside the lock

        std::this_thread::sleep_for (std::chrono::milliseconds{ 150 });
    }

  private:
    wxWebView *web;
    int number_of_divs = 0;
    std::string think_id, content_id;

  private:
    std::string curr_model_name;

  private:
    void on_click_send_prompt_button (wxWebViewEvent &event);

  private:
    std::string markdown;
    wxString line_buff;
    wxString JS_buffer;

  private:
    wxString HTML_complete = wxString (FULL_DOC);

  private:
    std::vector<std::thread> writer_thread, send_prompt_thread,
        update_web_thread;
    std::mutex buffer_mutex, update_web_mutex; // for thread safety
    std::condition_variable conditon;          // for thread coordination
    bool new_data = false;
    std::atomic<bool> alive{ true }, done = { false },
                                     inject_thinking = { false };

  private:
    bool is_ollama_running ();
    void start_ollama ();
    std::string execute_command (const char *cmd);
    void call_ollama (const std::string &prompt);
    void send_prompt (std::string prompt);
    void write_response ();

  private:
    static size_t curl_write_callback (void *contents, size_t size,
                                       size_t nmemb, void *user_data);

  private:
    std::vector<std::string> ignore_pattern{ R"(\\()", R"(\\))", R"(\\[)",
                                             R"(\\])", R"(\()",  R"(\))",
                                             R"(\[)",  R"(\])",  R"(\\)" };
};
