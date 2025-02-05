#include "../include/main.h"

std::mutex buffer_mutex;
std::string markdown;
std::atomic<bool> cool_bruh = true;

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
    std::vector<std::array<std::string, 4> > models;
    // Regex pattern to match NAME, ID, SIZE (with GB), and MODIFIED fields
    std::regex pattern (R"((\S+)\s+(\S+)\s+(\d+\.\d+ GB)\s+(\d+ \w+ ago))");
    std::smatch match;

    std::string::const_iterator search_start (output.cbegin ());

    while (std::regex_search (search_start, output.cend (), match, pattern))
        {
            models.push_back ({ match[1], match[2], match[3], match[4] });
            search_start = match.suffix ().first; // Move to next line
        }

    std::string model_name = models[0][0];

    wxSizer *full_sizer = new wxBoxSizer (wxVERTICAL);

    std::thread ollama_thread ([this, model_name] () {
        callOllama (markdown, model_name, "i want to move to china",
                    cool_bruh);
    });

    std::thread writer ([this, web] () {
        std::cout << "technically we are updating the ui lol" << '\n';
        while (cool_bruh)
            {
                buffer_mutex.lock ();
                char *html = cmark_markdown_to_html (
                    markdown.c_str (), markdown.size (),
                    CMARK_OPT_NORMALIZE | CMARK_OPT_UNSAFE);
                buffer_mutex.unlock ();
                std::string html_str (html);
                std::cout << "technically we are updating the ui lol"
                          << markdown << '\n';
                cjparse json (markdown);

                if (json.is_container_neither_object_or_array ())
                    {
                        return; // ollama responded with non
                        // json we are beyond
                        // fucked;
                    }
                if (json.is_container_an_object ())
                    {
                        cjparse::json_value response
                            = json.return_the_value ("response");
                        cjparse_json_generator generate
                            = cjparse_json_generator (response, true);
                        std::cout << generate.JSON_string << '\n';
                        if (std::holds_alternative<std::string> (response))
                            {
                                std::string str
                                    = std::get<std::string> (response);
                                wxString page = wxString (str) + '\n';
                                wxGetApp ().CallAfter ([web, page] () {
                                    web->SetPage (page, "");
                                });
                                free (html);
                            }
                        else
                            {
                                std::cout << "shit is not a string bro"
                                          << '\n';
                            }
                        std::this_thread::sleep_for (
                            std::chrono::milliseconds{ 200 });
                    }
            }
    });
    ollama_thread.detach ();
    writer.detach ();

    web->SetPage (wxString ("<title> HI </title>"), "");
}
