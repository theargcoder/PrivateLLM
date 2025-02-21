#include "../include/main.h"

class myApp : public wxApp
{
  public:
    virtual bool
    OnInit ()
    {
        private_llm_frame *frame = new private_llm_frame (NULL);
        frame->Show ();
        frame->SetSize (wxSize (900, 500));
        frame->SetMinClientSize (wxSize (500, 500));
        return true;
    }

    ~myApp ()
    {
        // to do
        //  implemet a system to destroy all threads and proprly close the app
    }
};

wxDECLARE_APP (myApp);
wxIMPLEMENT_APP (myApp);

size_t
private_llm_window::curl_write_callback (void *contents, size_t size,
                                         size_t nmemb, void *user_data)
{
    size_t totalSize = size * nmemb;

    private_llm_window *window = static_cast<private_llm_window *> (user_data);
    {
        std::lock_guard<std::mutex> lock (window->buffer_mutex);
        std::string str_temp;
        str_temp.assign ((char *)contents, totalSize);
        cjparse json (str_temp, window->ignore_pattern);
        if (json.is_container_neither_object_or_array ())
            {
                std::cout << "we got an NEITHER OBJ OR ARR as response.... "
                          << '\n';
                return totalSize; // ollama responded with non
                // json we are beyond
                // fucked;
            }
        else if (json.is_container_an_object ())
            {
                cjparse::json_value response
                    = json.return_the_value ("response");
                if (std::holds_alternative<std::string> (response))
                    {
                        window->markdown += std::get<std::string> (response);
                        window->new_data = true;
                        window->conditon.notify_one ();
                    }
                else
                    {
                        // JSON object named 'response' didn't have value
                        // string so ignore
                    }
                cjparse::json_value done_value
                    = json.return_the_value ("done");
                if (std::holds_alternative<bool> (done_value))
                    {
                        bool done_bool = std::get<bool> (done_value);
                        if (done_bool)
                            {
                                std::cout << "we are done here " << '\n';
                                window->new_data = true;
                                window->done = true;
                                window->conditon.notify_one ();
                            }
                    }
            }
        else if (json.is_container_an_array ())
            {
                std::cout << "we got an array as a response.... " << '\n';
                // process JSON array formatted response (shouldn't happen)
            }
    }

    return totalSize;
}

bool
private_llm_window::is_ollama_running ()
{
    CURL *curl = curl_easy_init ();
    if (!curl)
        return false;

    curl_easy_setopt (curl, CURLOPT_URL,
                      "http://localhost:11434/api/generate");
    curl_easy_setopt (curl, CURLOPT_NOBODY, 1L); // HEAD request
    CURLcode res = curl_easy_perform (curl);
    curl_easy_cleanup (curl);

    return (res == CURLE_OK); // Returns true if server responds
}

void
private_llm_window::start_ollama ()
{
    std::cout << "Starting Ollama server..." << std::endl;
    std::system ("ollama serve &"); // Run in the background
    std::this_thread::sleep_for (std::chrono::seconds (2)); // Wait for startup
}

// Function to call Ollama API
void
private_llm_window::call_ollama (const std::string &prompt)
{
    if (!is_ollama_running ())
        {
            start_ollama (); // Start Ollama if it's not running
        }

    CURL *curl;
    CURLcode res;

    curl_global_init (CURL_GLOBAL_ALL);
    curl = curl_easy_init ();

    if (curl)
        {
            std::string url = "http://localhost:11434/api/generate";
            std::string jsonData = R"({"model": ")" + curr_model_name
                                   + R"(", "prompt": ")" + prompt + R"("})";

            struct curl_slist *headers = nullptr;
            headers = curl_slist_append (headers,
                                         "Content-Type: application/json");

            curl_easy_setopt (curl, CURLOPT_URL, url.c_str ());
            curl_easy_setopt (curl, CURLOPT_POST, 1L);
            curl_easy_setopt (curl, CURLOPT_POSTFIELDS, jsonData.c_str ());
            curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION,
                              private_llm_window::curl_write_callback);
            curl_easy_setopt (curl, CURLOPT_WRITEDATA, this);

            res = curl_easy_perform (curl);
            if (res != CURLE_OK)
                {
                    std::cerr << "cURL error: " << curl_easy_strerror (res)
                              << std::endl;
                }

            curl_slist_free_all (headers);
            curl_easy_cleanup (curl);
        }

    curl_global_cleanup ();
}

std::string
private_llm_window::execute_command (const char *cmd)
{
    std::array<char, 128> buffer;
    std::string result;

    // Open a pipe to run the command
    std::unique_ptr<FILE, decltype (&pclose)> pipe (popen (cmd, "r"), pclose);
    if (!pipe)
        {
            throw std::runtime_error ("popen() failed!");
        }

    // Read the command output
    while (fgets (buffer.data (), buffer.size (), pipe.get ()) != nullptr)
        {
            result += buffer.data ();
        }

    return result;
}

void
private_llm_window::send_prompt (std::string prompt)
{
    std::string output = execute_command ("ollama list");
    std::cout << output << '\n';
    std::vector<std::array<std::string, 4> > models;
    // Updated regex to handle spacing and alignment
    std::regex pattern (R"(^(\S+)\s+(\S+)\s+(\d+\.\d+\s+GB)\s+(.+?)\s*$)");
    std::smatch match;

    // Split the output into lines
    std::istringstream stream (output);
    std::string line;
    while (std::getline (stream, line))
        {
            std::smatch match;
            if (std::regex_match (line, match, pattern))
                {
                    if (match.size () >= 5)
                        {
                            models.push_back (
                                { match[1].str (), match[2].str (),
                                  match[3].str (), match[4].str () });
                        }
                }
        }

    if (models.size () == 0)
        {
            // bewond fucked
        }
    else
        {
            for (auto pp : models)
                std::cout << pp[0] << '\n';

            curr_model_name = models[0][0];

            call_ollama (prompt);
        }
}

void
private_llm_window::on_click_send_prompt_button (wxWebViewEvent &event)
{
    if (!this->done && writer_thread.size () != 0)
        return;

    if (writer_thread.size () != 0)
        {
            writer_thread.pop_back ();
            send_prompt_thread.pop_back ();
            update_web_thread.pop_back ();
        }

    std::cout << "user just sent prompt: " << event.GetString () << '\n';

    wxString js = pre_prompt + event.GetString () + post_prompt;

    js.Replace ("\\", "\\\\");
    js.Replace ("\"", "\\\"");
    js.Replace ("\n", "\\n");
    js.Replace ("\r", "\\r");
    js.Replace ("\t", "\\t");
    js.Replace ("\f", "\\f");
    js.Replace ("\b", "\\b");

    js = "\"" + js + "\"";

    wxString javasript = "let client = "
                         "document.getElementById('client');"
                         "if (client) { "
                         "  let newContent = "
                         + js
                         + ";"
                           "  client.innerHTML += newContent; "
                           "}";

    wxGetApp ().CallAfter (
        [this, javasript] () { web->RunScriptAsync (javasript, NULL); });

    send_prompt_thread.emplace_back (
        std::thread (&private_llm_window::send_prompt, this,
                     std::string (event.GetString ())));
    writer_thread.emplace_back (
        std::thread (&private_llm_window::write_response, this));

    update_web_thread.emplace_back (std::thread ([this] () {
        while (!this->done)
            {
                wxString JS_cpy;
                {
                    std::unique_lock<std::mutex> lock (this->update_web_mutex);
                    JS_cpy = JS_buffer;
                    JS_buffer.clear ();
                }
                if (JS_cpy.empty ())
                    {
                        std::this_thread::sleep_for (
                            std::chrono::milliseconds{ 100 });
                    }
                else
                    {
                        if (inject_thinking)
                            {
                                JS_cpy
                                    += "let parentElement = "
                                       "document.getElementById(\'"
                                       + this->think_id
                                       + "\');parentElement.appendChild("
                                         "latestThinkFragment);"
                                         "window.latestThinkFragment= "
                                         "document.createDocumentFragment();";
                            }
                        else
                            {
                                JS_cpy
                                    += "let parentElement = "
                                       "document.getElementById(\'"
                                       + this->content_id
                                       + "\');parentElement.appendChild("
                                         "window.latestContentFragment);"
                                         "window.latestContentFragment= "
                                         "document.createDocumentFragment();";
                            }
                        std::cout << JS_cpy << "\n\n\n\n\\n\n\n";
                        wxGetApp ().CallAfter ([this, JS_cpy] () {
                            this->web->RunScriptAsync (JS_cpy, NULL);
                        });
                        std::this_thread::sleep_for (
                            std::chrono::milliseconds{ 100 });
                    }
            }
    }));

    writer_thread[0].detach ();
    send_prompt_thread[0].detach ();
    update_web_thread[0].detach ();
}

void
private_llm_window::write_response ()
{
    think_id = "think" + std::to_string (number_of_divs);
    content_id = "content" + std::to_string (number_of_divs);

    number_of_divs++;

    wxString crafted_injection = pre_think + think_id + post_think_pre_content
                                 + content_id + post_content;

    std::cout << crafted_injection << '\n';

    crafted_injection.Replace ("\\", "\\\\");
    crafted_injection.Replace ("\"", "\\\"");
    crafted_injection.Replace ("\n", "\\n");
    crafted_injection.Replace ("\r", "\\r");
    crafted_injection.Replace ("\t", "\\t");
    crafted_injection.Replace ("\f", "\\f");
    crafted_injection.Replace ("\b", "\\b");

    crafted_injection = "\"" + crafted_injection + "\"";

    wxString javasript = "let client = document.getElementById('client');"
                         "if (client) {"
                         "  let newContent = "
                         + crafted_injection
                         + ";"
                           "  client.innerHTML += newContent;"
                           "  let thinkElement = document.getElementById('"
                         + think_id
                         + "');"
                           "  let contentElement = document.getElementById('"
                         + content_id
                         + "');"
                           "  if (thinkElement) {"
                           "    window.latestThinkFragment = "
                           "document.createDocumentFragment();"
                           "    "
                           "window.latestThinkFragment.appendChild("
                           "thinkElement.cloneNode(true));"
                           "  }"
                           "  if (contentElement) {"
                           "    window.latestContentFragment = "
                           "document.createDocumentFragment();"
                           "    "
                           "window.latestContentFragment.appendChild("
                           "contentElement.cloneNode(true));"
                           "  }"
                           "}";

    wxGetApp ().CallAfter (
        [this, javasript] () { web->RunScriptAsync (javasript, NULL); });

    while (this->alive)
        {
            wxString latest_token;
            {
                std::unique_lock<std::mutex> lock (this->buffer_mutex);
                this->conditon.wait (lock, [this] { return new_data; });
                if (!this->alive)
                    break;
                if (markdown.find ("<think>") != std::string::npos)
                    {
                        inject_thinking = true;
                        wxString new_paragraph
                            = R"('<p class=\"ba94db8a\"></p>')";
                        this->update_web_mutex.lock ();
                        JS_buffer
                            += "if (window.latestThinkFragment) {"
                               "  let newFragment = "
                               "document.createRange()."
                               "createContextualFragment("
                               + new_paragraph
                               + ");"
                                 "  if (newFragment.firstElementChild) {"
                                 "    "
                                 "window.latestThinkFragment.appendChild("
                                 "newFragment.firstElementChild);"
                                 "    if "
                                 "(window.latestThinkFragment."
                                 "lastElementChild) {"
                                 "      window.lastParagraph = "
                                 "window.latestThinkFragment.lastElementChild;"
                                 "    }"
                                 "  }"
                                 "}";
                        this->update_web_mutex.unlock ();
                        markdown.clear ();
                        continue;
                    }
                else if (markdown.find ("</think>") != std::string::npos)
                    {
                        inject_thinking = false;
                        wxString new_paragraph = R"('<p></p>')";
                        this->update_web_mutex.lock ();
                        JS_buffer
                            += "if (window.latestContentFragment) {"
                               "  let newFragment = "
                               "document.createRange()."
                               "createContextualFragment("
                               + new_paragraph
                               + ");"
                                 "  if (newFragment.firstElementChild) {"
                                 "    "
                                 "window.latestContentFragment.appendChild("
                                 "newFragment.firstElementChild);"
                                 "    if "
                                 "(window.latestContentFragment."
                                 "lastElementChild) {"
                                 "      window.lastParagraph = "
                                 "window.latestContentFragment."
                                 "lastElementChild;"
                                 "    }"
                                 "  }"
                                 "}";
                        this->update_web_mutex.unlock ();
                        markdown.clear ();
                        continue;
                    }

                latest_token = wxString (markdown);
                markdown.clear ();
            }

            line_buff += latest_token;

            if (!this->alive)
                break;
            if (line_buff.find ("\n") != wxNOT_FOUND)
                {
                    if (!inject_thinking)
                        {
                            char *html = cmark_markdown_to_html (
                                line_buff.c_str (), line_buff.size (),
                                CMARK_OPT_VALIDATE_UTF8 | CMARK_OPT_UNSAFE);
                            line_buff = wxString (html, strlen (html));
                            line_buff.Replace ("\\", "\\\\");
                            line_buff.Replace ("\"", "\\\"");
                            line_buff.Replace ("\n", "\\n");
                            line_buff.Replace ("\r", "\\r");
                            line_buff.Replace ("\t", "\\t");
                            line_buff.Replace ("\f", "\\f");
                            line_buff.Replace ("\b", "\\b");

                            line_buff = '\'' + line_buff + '\'';
                            if (line_buff != "''")
                                {
                                    this->update_web_mutex.lock ();

                                    JS_buffer += "if (window.lastParagraph) {"
                                                 "  let newElement = "
                                                 "document.createRange()."
                                                 "createContextualFragment("
                                                 + line_buff
                                                 + ").firstElementChild;"
                                                   "  if (newElement) {"
                                                   "    "
                                                   "window.lastParagraph."
                                                   "replaceWith(newElement);"
                                                   "    window.lastParagraph "
                                                   "= newElement;"
                                                   "  }"
                                                   "}";

                                    this->update_web_mutex.unlock ();
                                }

                            wxString new_paragraph = "'<p></p>'";
                            this->update_web_mutex.lock ();
                            JS_buffer
                                += "if (window.latestContentFragment) {"
                                   "  let newFragment = "
                                   "document.createRange()."
                                   "createContextualFragment("
                                   + new_paragraph
                                   + ");"
                                     "  if (newFragment.firstElementChild) {"
                                     "    "
                                     "window.latestContentFragment."
                                     "appendChild("
                                     "newFragment.firstElementChild);"
                                     "    if "
                                     "(window.latestContentFragment."
                                     "lastElementChild) {"
                                     "      window.lastParagraph = "
                                     "window.latestContentFragment."
                                     "lastElementChild;"
                                     "    }"
                                     "  }"
                                     "}";
                            this->update_web_mutex.unlock ();
                        }
                    else
                        {
                            wxString new_paragraph
                                = "'<p class=\\\"ba94db8a\\\"></p>'";
                            line_buff = "<p class=\\\"ba94db8a\\\">"
                                        + line_buff + "</p>";

                            line_buff.Replace ("\\", "\\\\");
                            line_buff.Replace ("\"", "\\\"");
                            line_buff.Replace ("\n", "\\n");
                            line_buff.Replace ("\r", "\\r");
                            line_buff.Replace ("\t", "\\t");
                            line_buff.Replace ("\f", "\\f");
                            line_buff.Replace ("\b", "\\b");

                            line_buff = '\'' + line_buff + '\'';
                            if (line_buff != "''")
                                {
                                    this->update_web_mutex.lock ();

                                    JS_buffer += "if (window.lastParagraph) {"
                                                 "  let newElement = "
                                                 "document.createRange()."
                                                 "createContextualFragment("
                                                 + line_buff
                                                 + ").firstElementChild;"
                                                   "  if (newElement) {"
                                                   "    "
                                                   "window.lastParagraph."
                                                   "replaceWith(newElement);"
                                                   "    window.lastParagraph "
                                                   "= newElement;"
                                                   "  }"
                                                   "}";

                                    this->update_web_mutex.unlock ();
                                }
                            this->update_web_mutex.lock ();
                            JS_buffer
                                += "if (window.latestThinkFragment) {"
                                   "  let newFragment = "
                                   "document.createRange()."
                                   "createContextualFragment("
                                   + new_paragraph
                                   + ");"
                                     "  if (newFragment.firstElementChild) {"
                                     "    "
                                     "window.latestThinkFragment.appendChild("
                                     "newFragment.firstElementChild);"
                                     "    if "
                                     "(window.latestThinkFragment."
                                     "lastElementChild) {"
                                     "      window.lastParagraph = "
                                     "window.latestThinkFragment."
                                     "lastElementChild;"
                                     "    }"
                                     "  }"
                                     "}";
                            this->update_web_mutex.unlock ();
                        }
                    line_buff = "";
                    continue;
                }

            latest_token.Replace ("\\", "\\\\");
            latest_token.Replace ("\"", "\\\"");
            latest_token.Replace ("\n", "\\n");
            latest_token.Replace ("\r", "\\r");
            latest_token.Replace ("\t", "\\t");
            latest_token.Replace ("\f", "\\f");
            latest_token.Replace ("\b", "\\b");

            latest_token = "\"" + latest_token + "\"";

            // --- In your C++ injection code, when handling a new token ---
            if (!inject_thinking)
                {
                    this->update_web_mutex.lock ();
                    JS_buffer
                        += "if (window.latestContentFragment && "
                           "window.latestContentFragment.lastElementChild) {"
                           "  "
                           "window.latestContentFragment.lastElementChild."
                           "textContent += "
                           + latest_token
                           + ";"
                             "  window.lastParagraph = "
                             "window.latestContentFragment.lastElementChild;"
                             "}";
                    this->update_web_mutex.unlock ();
                }
            else
                {
                    this->update_web_mutex.lock ();
                    JS_buffer
                        += "if (window.latestThinkFragment && "
                           "window.latestThinkFragment.lastElementChild) {"
                           "  "
                           "window.latestThinkFragment.lastElementChild."
                           "textContent += "
                           + latest_token
                           + ";"
                             "  window.lastParagraph = "
                             "window.latestThinkFragment.lastElementChild;"
                             "}";
                    this->update_web_mutex.unlock ();
                }

            if (done)
                {
                    this->update_web_mutex.lock ();
                    JS_buffer.RemoveLast ();
                    JS_buffer += " cleanMathBlocksAfterDone(content); }";
                    this->update_web_mutex.unlock ();
                    wxGetApp ().CallAfter ([this] () {
                        std::unique_lock<std::mutex> lock (update_web_mutex);
                        this->web->RunScriptAsync (JS_buffer, NULL);
                        JS_buffer.clear ();
                    });
                }

            if (!this->alive)
                break;

            this->new_data = false;
        }
}

private_llm_window::private_llm_window (wxWindow *parent)
    : wxWindow (parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
{
    wxSizer *sizer_v1 = new wxBoxSizer (wxVERTICAL);
    wxSizer *sizer_v2 = new wxBoxSizer (wxVERTICAL);

    wxPanel *panel = new wxPanel (this);
    web = wxWebView::New (panel, wxID_ANY, wxWebViewDefaultURLStr,
                          wxDefaultPosition, wxSize (2000, 1000));

    web->AddScriptMessageHandler ("wx_msg");
    web->SetPage (HTML_complete, "text/html;charset=UTF-8");
    web->Bind (wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED,
               &private_llm_window::on_click_send_prompt_button, this);

    sizer_v2->Add (web, 1, wxEXPAND | wxALL, 5);

    panel->SetSizer (sizer_v2);
    sizer_v1->Add (panel, 1, wxEXPAND | wxALL, 0);

    this->SetSizer (sizer_v1);
    this->Layout ();
}

private_llm_notebook::private_llm_notebook (wxWindow *parent, long style)
    : wxAuiNotebook (parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style)
{
    this->AddPage (new private_llm_window (this), "DeepSeek R1 1.5B Param",
                   true);
}

private_llm_frame::private_llm_frame (wxWindow *parent)
    : wxFrame (parent, wxID_ANY, "Demo", wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_FRAME_STYLE)
{
    private_llm_notebook *book
        = new private_llm_notebook (this, wxAUI_NB_DEFAULT_STYLE);
}
