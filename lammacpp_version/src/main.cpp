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

void
private_llm_window::send_prompt (std::string prompt)
{
    const std::string model_path
        = "/Users/luccalabattaglia/Library/Caches/llama.cpp/"
          "unsloth_DeepSeek-R1-Distill-Qwen-7B-GGUF_DeepSeek-R1-Distill-Qwen-"
          "7B-Q4_K_M.gguf";

    // load dynamic backends
    ggml_backend_load_all ();

    llama_model_params model_params = llama_model_default_params ();

    llama_model *model
        = llama_model_load_from_file (model_path.c_str (), model_params);

    if (!model)
        {
            fprintf (stderr, "%s: error: unable to load model\n", __func__);
            return;
        }

    const llama_vocab *vocab = llama_model_get_vocab (model);

    llama_context_params ctx_params = llama_context_default_params ();

    llama_context *ctx = llama_init_from_model (model, ctx_params);

    if (!ctx)
        {
            fprintf (stderr, "%s: error: failed to create the llama_context\n",
                     __func__);
            return;
        }

    llama_sampler *smpl
        = llama_sampler_chain_init (llama_sampler_chain_default_params ());
    llama_sampler_chain_add (smpl, llama_sampler_init_min_p (0.05f, 1));
    llama_sampler_chain_add (smpl, llama_sampler_init_temp (0.8f));
    llama_sampler_chain_add (smpl,
                             llama_sampler_init_dist (LLAMA_DEFAULT_SEED));

    std::vector<llama_chat_message> messages;
    std::vector<char> formatted;

    const char *tmpl = llama_model_chat_template (model, nullptr);

    messages.push_back ({ "user", strdup (prompt.c_str ()) });

    int new_len = llama_chat_apply_template (
        tmpl, messages.data (), messages.size (), true, formatted.data (),
        formatted.size ());

    if (new_len > (int)formatted.size ())
        {
            formatted.resize (new_len);
            new_len = llama_chat_apply_template (
                tmpl, messages.data (), messages.size (), true,
                formatted.data (), formatted.size ());
        }

    if (new_len < 0)
        {
            fprintf (stderr, "failed to apply the chat template\n");
            return;
        }

    llama_sampler_free (smpl);
    llama_free (ctx);
    llama_model_free (model);
}

void
private_llm_window::on_click_send_prompt_button (wxWebViewEvent &event)
{
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

    send_prompt_thread = std::thread (&private_llm_window::send_prompt, this,
                                      std::string (event.GetString ()));
    writer_thread = std::thread (&private_llm_window::write_response, this);
    writer_thread.detach ();
    send_prompt_thread.detach ();
}

void
private_llm_window::write_response ()
{
    bool inject_thinking = false;

    std::string think_id = "think" + std::to_string (number_of_divs);
    std::string content_id = "content" + std::to_string (number_of_divs);

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
            wxString wx_page = wxString (html);

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
                    wx_page.Replace ("<p>", R"(<p class=\"ba94db8a\">)", true);
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
            wxGetApp ().CallAfter ([this, js] () {
                web->RunScriptAsync (js, NULL);

                if (this->done)
                    {
                        {
                            std::lock_guard<std::mutex> lock (
                                this->buffer_mutex);
                            this->alive = false;
                            this->new_data = true;
                        }
                        this->conditon
                            .notify_one (); // Notify outside the lock

                        if (this->writer_thread.joinable ())
                            {
                                this->writer_thread.join ();
                            }
                        if (this->send_prompt_thread.joinable ())
                            {
                                this->send_prompt_thread.join ();
                            }
                    }
            });

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
            = std::thread (&private_llm_window::send_prompt, "hello",
    this); writer_thread.detach (); send_prompt_thread.detach ();
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
