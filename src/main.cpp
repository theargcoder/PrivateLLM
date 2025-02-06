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
            wxString full_page = HTML_heading +
                                 R"(<body>
                  <title>Bro your ollama has no models</title>
                  <p>open your terminal and run "substitute this for the model name</p>
                  <p>
                      you may also go to the official ollama webpage
                      <a href="https://ollama.com"> ollama.com </a>
                  to seek a better explanation of how to install models via the terminal
                  </p>
                  </body>)" + HTML_ending;
            web->SetPage (full_page, "");
        }
    else
        {
            for (auto pp : models)
                std::cout << pp[0] << '\n';

            std::string model_name = models[0][0];

            wxSizer *full_sizer = new wxBoxSizer (wxVERTICAL);

            std::thread ollama_thread ([this, model_name] () {
                callOllama (
                    markdown, model_name,
                    "so i need you to integrate e^(ax^(2)) where a is a "
                    "constant "
                    "who's sign does not matter, also i need you to use "
                    "proper math notation",
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
                                                = R"(<div class="think">)";
                                        if (str_response.find ("</think>")
                                            != std::string::npos)
                                            str_response = R"(</div>)";
                                        HTML_data += str_response;
                                    }
                            }
                        char *html = cmark_markdown_to_html (
                            HTML_data.c_str (), HTML_data.size (),
                            CMARK_OPT_VALIDATE_UTF8 | CMARK_OPT_UNSAFE);

                        wxString page
                            = HTML_heading + wxString (html) + HTML_ending;
                        html_cpy = std::string (page);
                        wxGetApp ().CallAfter ([web, page] () {
                            web->SetPage (page, "text/html;charset=UTF-8");
                        });
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

            wxString full_page = HTML_heading
                                 + "<title> PROCESSING \n 1 second bro</title>"
                                 + HTML_ending;
            web->SetPage (full_page, "");
        }
}
