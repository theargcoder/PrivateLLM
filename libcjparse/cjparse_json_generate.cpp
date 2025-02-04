#include "cjparse_json_generate.h"

cjparse_json_generator::cjparse_json_generator (
    cjparse::cjparse_json_value &JSON, bool generate_pretty_json)
{
    pretty = generate_pretty_json;
    print_json_from_memory (JSON);
}

cjparse_json_generator::cjparse_json_generator (cjparse::json_value &JSON,
                                                bool generate_pretty_json)
{
    pretty = generate_pretty_json;
    cjparse::cjparse_json_value new_value;
    if (std::holds_alternative<cjparse::json_object> (JSON))
        {
            new_value = cjparse::cjparse_json_value (
                std::get<cjparse::json_object> (JSON));
        }
    else if (std::holds_alternative<cjparse::json_array> (JSON))
        {
            new_value = cjparse::cjparse_json_value (
                std::get<cjparse::json_array> (JSON));
        }
    else if (std::holds_alternative<cjparse::json_string> (JSON))
        {
            new_value = cjparse::cjparse_json_value (
                std::get<cjparse::json_string> (JSON));
        }
    else if (std::holds_alternative<cjparse::json_number> (JSON))
        {
            new_value = cjparse::cjparse_json_value (
                std::get<cjparse::json_number> (JSON));
        }
    else if (std::holds_alternative<bool> (JSON))
        {
            new_value = cjparse::cjparse_json_value (std::get<bool> (JSON));
        }
    else if (std::holds_alternative<cjparse::json_null> (JSON))
        {
            new_value = cjparse::cjparse_json_value (
                std::get<cjparse::json_null> (JSON));
        }

    print_json_from_memory (new_value);
}

void
cjparse_json_generator::print_json_from_memory (
    cjparse::cjparse_json_value &JSON)
{
    std::visit (
        [this] (auto &json_val) {
            using T = std::decay_t<decltype (json_val)>;
            if constexpr (std::is_same_v<T, cjparse::json_object>)
                {
                    std::string n_of_tabs;
                    if (pretty)
                        {
                            n_of_iteration++;
                            for (int i = 0; i < n_of_iteration; i++)
                                n_of_tabs = n_of_tabs + std::string ("    ");
                        }
                    JSON_string.push_back ('{');
                    if (pretty)
                        JSON_string.push_back ('\n');

                    std::size_t size = json_val.size ();
                    int counter = 0;
                    for (auto &[name, value] : json_val)
                        {
                            if (pretty)
                                JSON_string = JSON_string + n_of_tabs;
                            JSON_string = JSON_string + '\"' + name + "\":";
                            if (pretty)
                                JSON_string.push_back (' ');
                            print_json_from_memory (value);
                            if (counter != size - 1)
                                JSON_string.push_back (',');
                            if (pretty)
                                JSON_string.push_back ('\n');
                            counter++;
                        }
                    if (pretty)
                        {
                            n_of_iteration--;
                            n_of_tabs = "";
                            for (int i = 0; i < n_of_iteration; i++)
                                n_of_tabs = n_of_tabs + std::string ("    ");
                            JSON_string = JSON_string + n_of_tabs;
                        }
                    JSON_string.push_back ('}');
                }
            else if constexpr (std::is_same_v<T, cjparse::json_array>)
                {
                    std::string n_of_tabs;
                    if (pretty)
                        {
                            n_of_iteration++;
                            for (int i = 0; i < n_of_iteration; i++)
                                n_of_tabs = n_of_tabs + std::string ("    ");
                        }
                    JSON_string.push_back ('[');
                    if (pretty)
                        JSON_string.push_back ('\n');

                    std::size_t size = json_val.size ();
                    int counter = 0;

                    for (auto &value : json_val)
                        {
                            if (pretty)
                                JSON_string = JSON_string + n_of_tabs;
                            print_json_from_memory (value);
                            if (counter != size - 1)
                                JSON_string.push_back (',');
                            if (pretty)
                                JSON_string.push_back ('\n');
                            counter++;
                        }
                    if (pretty)
                        {
                            n_of_iteration--;
                            n_of_tabs = "";
                            for (int i = 0; i < n_of_iteration; i++)
                                n_of_tabs = n_of_tabs + std::string ("    ");
                            JSON_string = JSON_string + n_of_tabs;
                        }
                    JSON_string.push_back (']');
                }
            else if constexpr (std::is_same_v<T, cjparse::json_null>)
                {
                    JSON_string = JSON_string + "null";
                }
            else if constexpr (std::is_same_v<T, cjparse::json_string>)
                {
                    JSON_string = JSON_string + "\"" + json_val + "\"";
                }
            else if constexpr (std::is_same_v<T, cjparse::json_number>)
                {
                    print_json_number (json_val);
                }
            else if constexpr (std::is_same_v<T, bool>)
                {
                    JSON_string = JSON_string + (json_val ? "true" : "false");
                }
        },
        JSON.value);
}

void
cjparse_json_generator::print_json_number (cjparse::json_number &num)
{
    std::visit (
        [this] (auto &val) {
            JSON_string = JSON_string + std::to_string (val);
        },
        num);
}
