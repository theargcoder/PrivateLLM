#include "../include/main.h"

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

    ~myApp ()
    {
        // to do
        //  implemet a system to destroy all threads and proprly close the app
    }
};

wxDECLARE_APP (myApp);
wxIMPLEMENT_APP (myApp);

size_t
private_llm_frame::WriteCallback (void *contents, size_t size, size_t nmemb,
                                  void *user_data)
{
    size_t totalSize = size * nmemb;

    private_llm_frame *frame = static_cast<private_llm_frame *> (user_data);
    {
        std::lock_guard<std::mutex> lock (frame->buffer_mutex);
        std::string str_temp;
        str_temp.assign ((char *)contents, totalSize);
        cjparse json (str_temp, frame->ignore_pattern);
        if (json.is_container_neither_object_or_array ())
            {
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
                        frame->markdown += std::get<std::string> (response);
                        if (frame->markdown.find ('\n') != std::string::npos)
                            {
                                frame->new_data = true;
                                frame->conditon.notify_one ();
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
                                frame->done = true;
                                frame->new_data = true;
                                frame->conditon.notify_one ();
                            }
                    }
            }
        else if (json.is_container_an_array ())
            {
                // process JSON array formatted response (shouldn't happen)
            }
    }

    return totalSize;
}

bool
private_llm_frame::isOllamaRunning ()
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
private_llm_frame::startOllama ()
{
    std::cout << "Starting Ollama server..." << std::endl;
    std::system ("ollama serve &"); // Run in the background
    std::this_thread::sleep_for (std::chrono::seconds (2)); // Wait for startup
}

// Function to call Ollama API
void
private_llm_frame::callOllama (const std::string &model_name,
                               const std::string &prompt)
{
    if (!isOllamaRunning ())
        {
            startOllama (); // Start Ollama if it's not running
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
                              private_llm_frame::WriteCallback);
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
private_llm_frame::execCommand (const char *cmd)
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

private_llm_frame::private_llm_frame (wxWindow *parent, int id, wxString title,
                                      wxPoint pos, wxSize size, int style)
    : wxFrame (parent, id, title, pos, size, style)
{
    wxWebView *web = wxWebView::New (this, wxID_ANY, wxWebViewDefaultURLStr,
                                     wxDefaultPosition, wxDefaultSize);

    std::string output = execCommand ("ollama list");
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

            wxSizer *full_sizer = new wxBoxSizer (wxVERTICAL);

            std::thread ollama_thread ([this, model_name] () {
                callOllama (model_name,
                            "i need the anti-derivative of e^(-x^2) "
                            "with proper math notation");
            });

            std::thread writer ([this, web] () {
                std::string html_cpy;
                bool inject_thinking = false;
                while (true)
                    {
                        HTML_data = "";
                        char *html;
                        {
                            std::unique_lock<std::mutex> lock (
                                private_llm_frame::buffer_mutex);
                            if (markdown.find ("<think>") != std::string::npos)
                                {
                                    inject_thinking = true;
                                    markdown.clear ();
                                    private_llm_frame::conditon.wait (
                                        lock, [this] { return new_data; });
                                    continue;
                                }
                            else if (markdown.find ("</think>")
                                     != std::string::npos)
                                {
                                    inject_thinking = false;
                                    markdown.clear ();
                                    private_llm_frame::conditon.wait (
                                        lock, [this] { return new_data; });
                                    continue;
                                }
                            else
                                {
                                    html = cmark_markdown_to_html (
                                        markdown.c_str (), markdown.size (),
                                        CMARK_OPT_VALIDATE_UTF8
                                            | CMARK_OPT_UNSAFE);
                                    markdown.clear ();
                                    private_llm_frame::conditon.wait (
                                        lock, [this] { return new_data; });
                                }
                        }

                        html_cpy = std::string (html);
                        HTML_data += html_cpy;
                        wxString wx_page = wxString (html_cpy);

                        wx_page.Replace ("\\", "\\\\");
                        wx_page.Replace ("\"", "\\\"");
                        wx_page.Replace ("\n", "\\n");
                        wx_page.Replace ("\r", "\\r");
                        wx_page.Replace ("\t", "\\t");
                        wx_page.Replace ("\f", "\\f");
                        wx_page.Replace ("\b", "\\b");

                        wx_page = "\"" + wx_page + "\"";
                        wxString js;
                        if (!inject_thinking)
                            {
                                js = "let content = "
                                     "document.getElementById('content');"
                                     "if (content) { "
                                     "  let newContent = "
                                     + wx_page
                                     + "; "
                                       "  content.innerHTML += newContent; "
                                     + " cleanMathBlocks(content);"
                                     + "MathJax.typesetPromise([content]);"
                                     + "}";
                            }
                        else
                            {
                                js = "let think= "
                                     "document.getElementById('thinking');"
                                     "if (think) { "
                                     "  let newContent = "
                                     + wx_page
                                     + "; "
                                       "  think.innerHTML += newContent; "
                                       "}";
                            }

                        if (done)
                            {
                                js = "let content = "
                                     "document.getElementById('content');"
                                     "if (content) { "
                                     "  let newContent = "
                                     + wx_page
                                     + "; "
                                       "  content.innerHTML += newContent; "
                                     + " cleanMathBlocksAfterDone(content);"
                                     + "MathJax.typesetPromise([content]);"
                                     + "}";
                            }

                        std::cout << html << '\n';

                        wxGetApp ().CallAfter (
                            [web, js] () { web->RunScriptAsync (js, NULL); });

                        private_llm_frame::new_data = false;
                    }
            });
            web->SetPage (HTML_complete, "text/html;charset=UTF-8");
            ollama_thread.detach ();
            writer.detach ();
        }
}
