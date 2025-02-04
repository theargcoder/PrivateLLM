#include "../include/main.h"

size_t
WriteCallback (void *contents, size_t size, size_t nmemb, std::string *output)
{
    size_t totalSize = size * nmemb;
    output->append ((char *)contents, totalSize);
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
std::string
callOllama (const std::string &prompt)
{
    if (!isOllamaRunning ())
        {
            startOllama (); // Start Ollama if it's not running
        }

    CURL *curl;
    CURLcode res;
    std::string response;

    curl_global_init (CURL_GLOBAL_ALL);
    curl = curl_easy_init ();

    if (curl)
        {
            std::string url = "http://localhost:11434/api/generate";
            std::string jsonData
                = R"({"model": "deepseek-r1:14b", "prompt": ")" + prompt
                  + R"("})";

            struct curl_slist *headers = nullptr;
            headers = curl_slist_append (headers,
                                         "Content-Type: application/json");

            curl_easy_setopt (curl, CURLOPT_URL, url.c_str ());
            curl_easy_setopt (curl, CURLOPT_POST, 1L);
            curl_easy_setopt (curl, CURLOPT_POSTFIELDS, jsonData.c_str ());
            curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt (curl, CURLOPT_WRITEDATA, &response);

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

    return response;
}

private_llm_frame::private_llm_frame (wxWindow *parent, int id, wxString title,
                                      wxPoint pos, wxSize size, int style)
    : wxFrame (parent, id, title, pos, size, style)
{
    wxSizer *full_sizer = new wxBoxSizer (wxVERTICAL);

    std::string markdown = callOllama ("hi r1 how are you doing");
    char *html = cmark_markdown_to_html (markdown.c_str (), markdown.size (),
                                         CMARK_OPT_DEFAULT);

    wxWebView *web = wxWebView::New (this, wxID_ANY, wxWebViewDefaultURLStr,
                                     wxDefaultPosition, wxDefaultSize);

    wxString page = wxString (html);
    web->SetPage (page, "");

    free (html);
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
