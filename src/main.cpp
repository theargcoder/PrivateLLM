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
                        if (window->markdown.find ('\n') != std::string::npos)
                            {
                                window->new_data = true;
                                window->conditon.notify_one ();
                            }
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
private_llm_window::call_ollama (const std::string &model_name,
                                 const std::string &prompt)
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
            std::string jsonData = R"({"model": ")" + model_name
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

            std::string model_name = models[1][0];

            call_ollama (model_name, prompt);
        }
}

void
private_llm_window::on_click_send_prompt_button (wxWebViewEvent &event)
{
    std::cout << "user just sent prompt: " << event.GetString () << '\n';
    send_prompt_thread = std::thread (&private_llm_window::send_prompt, this,
                                      std::string (event.GetString ()));
    writer_thread = std::thread (&private_llm_window::write_response, this);
    writer_thread.detach ();
    send_prompt_thread.detach ();
}

void
private_llm_window::write_response ()
{
    std::string html_cpy;
    bool inject_thinking = false;

    std::string think_id = "think" + std::to_string (number_of_divs);
    std::string content_id = "content" + std::to_string (number_of_divs);

    number_of_divs++;

    wxString crafted_injection = pre_think + think_id + post_think_pre_content
                                 + content_id + post_content;

    crafted_injection.Replace ("\\", "\\\\");
    crafted_injection.Replace ("\"", "\\\"");
    crafted_injection.Replace ("\n", "\\n");
    crafted_injection.Replace ("\r", "\\r");
    crafted_injection.Replace ("\t", "\\t");
    crafted_injection.Replace ("\f", "\\f");
    crafted_injection.Replace ("\b", "\\b");

    crafted_injection = "\"" + crafted_injection + "\"";

    wxString javasript = "let client = "
                         "document.getElementById('client');"
                         "if (client) { "
                         "  let newContent = "
                         + crafted_injection
                         + ";"
                           "  client.innerHTML += newContent; "
                           "}";

    wxGetApp ().CallAfter (
        [this, javasript] () { web->RunScriptAsync (javasript, NULL); });

    while (this->alive)
        {
            char *html;
            {
                std::unique_lock<std::mutex> lock (this->buffer_mutex);
                this->conditon.wait (lock, [this] { return new_data; });
                if (!this->alive)
                    break;
                if (markdown.find ("<think>") != std::string::npos)
                    {
                        inject_thinking = true;
                    }
                else if (markdown.find ("</think>") != std::string::npos)
                    {
                        inject_thinking = false;
                    }

                html = cmark_markdown_to_html (
                    markdown.c_str (), markdown.size (),
                    CMARK_OPT_VALIDATE_UTF8 | CMARK_OPT_UNSAFE);
                markdown.clear ();
            }
            html_cpy = std::string (html);
            wxString wx_page = wxString (html_cpy);

            wx_page.Replace ("\\", "\\\\");
            wx_page.Replace ("\"", "\\\"");
            wx_page.Replace ("\n", "\\n");
            wx_page.Replace ("\r", "\\r");
            wx_page.Replace ("\t", "\\t");
            wx_page.Replace ("\f", "\\f");
            wx_page.Replace ("\b", "\\b");

            wx_page = "\"" + wx_page + "\"";
            if (!this->alive)
                break;
            wxString js;
            if (!inject_thinking)
                {
                    js = "let content = "
                         "document.getElementById('"
                         + content_id
                         + "');"
                           "if (content) { "
                           "  let newContent = "
                         + wx_page
                         + "; "
                           "  content.innerHTML += newContent; "
                         + " cleanMathBlocks(content);"
                         + "MathJax.typesetPromise([content]);" + "}";
                }
            else
                {
                    wx_page.Replace ("<p>", R"(<p class="ba94db8a">)", true);
                    js = "let think= "
                         "document.getElementById('"
                         + think_id
                         + "');"
                           "if (think) { "
                           "  let newContent = "
                         + wx_page
                         + "; "
                           "  think.innerHTML += newContent; "
                           "}";
                }

            if (done)
                {
                    std::string id
                        = "content" + std::to_string (number_of_divs);
                    js = "let content = "
                         "document.getElementById('"
                         + content_id
                         + "');"
                           "if (content) { "
                           "  let newContent = "
                         + wx_page
                         + "; "
                           "  content.innerHTML += newContent; "
                         + " cleanMathBlocksAfterDone(content);"
                         + "MathJax.typesetPromise([content]);" + "}";
                }

            std::cout << wx_page << '\n';

            if (!this->alive)
                break;
            wxGetApp ().CallAfter (
                [this, js] () { web->RunScriptAsync (js, NULL); });

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

    /*
    button_prompt->Bind (wxEVT_BUTTON, [this] (wxCommandEvent &evt) {
        std::cout << "EVT BUTTON WHUATTT ???? " << '\n';
        writer_thread
            = std::thread (&private_llm_window::write_response, this);
        send_prompt_thread
            = std::thread (&private_llm_window::send_prompt, "hello", this);
        writer_thread.detach ();
        send_prompt_thread.detach ();
    });
    */

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
