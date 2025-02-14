#include "../include/main.h"

std::mutex buffer_mutex; // for thread safety

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

wxDECLARE_APP (myApp);
wxIMPLEMENT_APP (myApp);

size_t
WriteCallback (void *contents, size_t size, size_t nmemb, std::string *output)
{
    size_t totalSize = size * nmemb;
    buffer_mutex.lock ();
    output->append ((char *)contents, totalSize);
    buffer_mutex.unlock ();
    return totalSize;
}

bool
isOllamaRunning ()
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
startOllama ()
{
    std::cout << "Starting Ollama server..." << std::endl;
    std::system ("ollama serve &"); // Run in the background
    std::this_thread::sleep_for (std::chrono::seconds (2)); // Wait for startup
}

// Function to call Ollama API
void
callOllama (std::string &buffer, const std::string &model_name,
            const std::string &prompt, std::atomic<bool> &coordination)
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
            curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt (curl, CURLOPT_WRITEDATA, &buffer);

            res = curl_easy_perform (curl);
            if (res != CURLE_OK)
                {
                    std::cerr << "cURL error: " << curl_easy_strerror (res)
                              << std::endl;
                }
            coordination = false;

            curl_slist_free_all (headers);
            curl_easy_cleanup (curl);
        }

    curl_global_cleanup ();
}

std::string
execCommand (const char *cmd)
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
                callOllama (
                    markdown, model_name,
                    "i need you to prove to me step by step using proper math "
                    "notation the method of variation of parameters",
                    cool_bruh);
            });

            std::thread writer ([this, web] () {
                std::string html_cpy;
                while (cool_bruh)
                    {
                        HTML_data = "";
                        buffer_mutex.lock ();
                        std::stringstream html_stream (markdown);
                        buffer_mutex.unlock ();
                        std::string line;
                        bool manually_close_div = false;

                        while (std::getline (html_stream, line))
                            {
                                std::vector<std::string> ignore_pattern{
                                    R"(\\()", R"(\\))", R"(\\[)",
                                    R"(\\])", R"(\()",  R"(\))",
                                    R"(\[)",  R"(\])",  R"(\\)"
                                };
                                cjparse json (line, ignore_pattern);
                                if (json.is_container_neither_object_or_array ())
                                    {
                                        break; // ollama responded with non
                                        // json we are beyond
                                        // fucked;
                                    }
                                if (json.is_container_an_object ())
                                    {
                                        cjparse::json_value response
                                            = json.return_the_value (
                                                "response");
                                        std::string str_response
                                            = std::get<std::string> (response);
                                        if (str_response.find ("<think>")
                                            != std::string::npos)
                                            str_response
                                                = R"(<div class="think" id = "thinking">)";
                                        if (str_response.find ("</think>")
                                            != std::string::npos)
                                            {
                                                str_response = R"(</div>)";
                                            }
                                        else
                                            {
                                                manually_close_div = true;
                                            }
                                        HTML_data += str_response;
                                    }
                            }
                        if (manually_close_div)
                            HTML_data += R"(</div>)";
                        char *html = cmark_markdown_to_html (
                            HTML_data.c_str (), HTML_data.size (),
                            CMARK_OPT_VALIDATE_UTF8 | CMARK_OPT_UNSAFE);

                        html_cpy = std::string (html);
                        wxString wx_page = wxString (html);

                        // Escape JavaScript special characters in the HTML
                        // content
                        wxString escaped_page = wx_page;
                        escaped_page.Replace ("'", "\\'");
                        escaped_page.Replace ("\"", "\\\"");
                        escaped_page.Replace ("\n", "\\n");
                        escaped_page.Replace ("\r", "");

                        wxString js = R"delim(
                            let content = document.getElementById('content');
                            if (content) {
                                let think = document.getElementById('thinking');
                                if (think) {
                                    let scrollPos = think.scrollTop;  // Save the scroll position of 'thinking'
                                    let newContent = ')delim"
                                      + escaped_page + R"delim("';
                                    content.innerHTML = newContent;    // Update the innerHTML of 'content'
                                    setTimeout(() => { think.scrollTop = scrollPos; }, 50);
                                }
                            }
                        )delim";

                        // Execute the JavaScript inside the WebView
                        wxGetApp ().CallAfter (
                            [web, js] () { web->RunScriptAsync (js, NULL); });
                        free (html);

                        std::this_thread::sleep_for (
                            std::chrono::milliseconds{ 200 });
                    }
                std::cout << html_cpy << '\n';
                std::cout << "\n\n\n\n\n\n\n";
                std::cout << markdown << '\n';
            });
            ollama_thread.detach ();
            writer.detach ();

            web->SetPage (HTML_complete, "text/html;charset=UTF-8");
        }
}
