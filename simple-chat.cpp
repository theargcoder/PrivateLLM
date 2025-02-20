#include <iostream>
#include <llama.h>
#include <stdexcept>
#include <string>
#include <vector>

int
main ()
{
    // Hardcoded parameters
    const std::string model_path
        = "/Users/luccalabattaglia/Library/Caches/llama.cpp/"
          "unsloth_DeepSeek-R1-Distill-Qwen-7B-GGUF_DeepSeek-R1-Distill-Qwen-"
          "7B-Q4_K_M.gguf";
    const int n_ctx = 4096;
    const int ngl = 200;

    // Set up error logging (only errors will be printed)
    llama_log_set (
        [] (enum ggml_log_level level, const char *text, void *) {
            if (level >= GGML_LOG_LEVEL_ERROR)
                {
                    std::cerr << text;
                }
        },
        nullptr);

    // Load dynamic backends
    ggml_backend_load_all ();

    // Load the model
    llama_model_params model_params = llama_model_default_params ();
    model_params.n_gpu_layers = ngl;
    llama_model *model
        = llama_model_load_from_file (model_path.c_str (), model_params);
    if (!model)
        {
            std::cerr << "Error: unable to load model\n";
            return 1;
        }
    const llama_vocab *vocab = llama_model_get_vocab (model);

    // Initialize the context
    llama_context_params ctx_params = llama_context_default_params ();
    ctx_params.n_ctx = n_ctx;
    ctx_params.n_batch = n_ctx;
    llama_context *ctx = llama_init_from_model (model, ctx_params);
    if (!ctx)
        {
            std::cerr << "Error: failed to create the llama_context\n";
            llama_model_free (model);
            return 1;
        }

    // Initialize the sampler chain
    llama_sampler *smpl
        = llama_sampler_chain_init (llama_sampler_chain_default_params ());
    llama_sampler_chain_add (smpl, llama_sampler_init_min_p (0.05f, 1));
    llama_sampler_chain_add (smpl, llama_sampler_init_temp (0.8f));
    llama_sampler_chain_add (smpl,
                             llama_sampler_init_dist (LLAMA_DEFAULT_SEED));

    // Lambda function to generate a response from a prompt
    auto generate = [&] (const std::string &prompt) -> std::string {
        std::string response;
        const bool is_first = (llama_get_kv_cache_used_cells (ctx) == 0);

        // Determine the number of tokens for the prompt
        int n_prompt_tokens
            = -llama_tokenize (vocab, prompt.c_str (), prompt.size (), nullptr,
                               0, is_first, true);
        if (n_prompt_tokens <= 0)
            {
                throw std::runtime_error ("Failed to tokenize prompt");
            }
        std::vector<llama_token> prompt_tokens (n_prompt_tokens);
        if (llama_tokenize (vocab, prompt.c_str (), prompt.size (),
                            prompt_tokens.data (), prompt_tokens.size (),
                            is_first, true)
            < 0)
            {
                throw std::runtime_error ("Failed to tokenize the prompt");
            }

        // Prepare a batch for the prompt
        llama_batch batch = llama_batch_get_one (prompt_tokens.data (),
                                                 prompt_tokens.size ());
        llama_token new_token_id;
        while (true)
            {
                // Ensure there's enough context space for the batch
                int current_ctx = llama_n_ctx (ctx);
                int used_ctx = llama_get_kv_cache_used_cells (ctx);
                if (used_ctx + batch.n_tokens > current_ctx)
                    {
                        std::cerr << "Context size exceeded\n";
                        break;
                    }
                if (llama_decode (ctx, batch))
                    {
                        throw std::runtime_error ("Failed to decode");
                    }
                new_token_id = llama_sampler_sample (smpl, ctx, -1);
                // End-of-generation check
                if (llama_vocab_is_eog (vocab, new_token_id))
                    {
                        break;
                    }
                char buf[256];
                int n = llama_token_to_piece (vocab, new_token_id, buf,
                                              sizeof (buf), 0, true);
                if (n < 0)
                    {
                        throw std::runtime_error (
                            "Failed to convert token to piece");
                    }
                std::string piece (buf, n);
                std::cout << piece; // Print as generated
                std::cout.flush ();
                response += piece;
                // Prepare the next batch with the new token
                batch = llama_batch_get_one (&new_token_id, 1);
            }
        return response;
    };

    // Fixed prompt and generation call
    const std::string prompt
        = "prove to me the method of variation of parameters for ODE's";
    std::cout << "Prompt: " << prompt << "\n\nResponse:\n";
    try
        {
            std::string response = generate (prompt);
            std::cout << "\n\nFinal Response:\n" << response << "\n";
        }
    catch (const std::exception &e)
        {
            std::cerr << "Error during generation: " << e.what () << "\n";
            llama_sampler_free (smpl);
            llama_free (ctx);
            llama_model_free (model);
            return 1;
        }

    // Clean up resources
    llama_sampler_free (smpl);
    llama_free (ctx);
    llama_model_free (model);
    return 0;
}
