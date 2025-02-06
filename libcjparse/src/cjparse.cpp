#include "../include/cjparse.h"
#include "../include/cjparse_json_parser.h"

cjparse::cjparse (std::string &str) { cjparse_json_parser (str, JSON); }

cjparse::cjparse (std::string &str,
                  std::string json_string_pattern_to_keep_raw)
{
    cjparse_json_parser (str, JSON, json_string_pattern_to_keep_raw);
}

cjparse::cjparse (std::string &str,
                  std::vector<std::string> json_string_patterns_to_keep_raw)
{
    cjparse_json_parser (str, JSON, json_string_patterns_to_keep_raw);
}
cjparse::cjparse (std::stringstream &fake_str)
{
    // to be determied
}

cjparse::json_value
cjparse::return_full_json ()
{
    json_value value_to_return;

    std::visit ([&value_to_return] (auto value) { value_to_return = value; },
                JSON.value);
    return std::move (value_to_return);
}

bool
cjparse::is_container_an_object ()
{
    json_value possible_object = return_full_json ();
    return std::holds_alternative<json_object> (possible_object);
}

bool
cjparse::is_container_an_array ()
{
    json_value possible_object = return_full_json ();
    return std::holds_alternative<json_array> (possible_object);
}

bool
cjparse::is_container_neither_object_or_array ()
{
    bool bool_to_return;
    json_value possible_object = return_full_json ();
    bool is_array = std::holds_alternative<json_array> (possible_object);
    bool is_object = std::holds_alternative<json_object> (possible_object);

    bool_to_return = !is_array && !is_object;
    return bool_to_return;
}

template <typename json_type>
json_type
cjparse::return_json_container ()
{
    json_type type_to_return;
    type_to_return = std::get<json_type> (return_full_json ());
    return std::move (type_to_return);
}

cjparse::json_value
cjparse::return_the_value (std::string object_name)
{
    bool return_empty
        = false; // just exists to reuse internal function no other purpose
    json_value full_json, value_to_return;
    std::visit ([&full_json] (auto value) { full_json = value; }, JSON.value);

    return_the_value_internal (object_name, full_json, value_to_return,
                               return_empty);
    return std::move (value_to_return);
}

cjparse::json_value
cjparse::return_the_value (std::vector<std::string> object_name_vector_in)
{
    bool return_empty = false;
    json_value full_json, value_to_return;
    std::visit ([&full_json] (auto value) { full_json = value; }, JSON.value);

    auto it = object_name_vector_in.begin ();

    while (it != object_name_vector_in.end ())
        {
            return_the_value_internal (*it, full_json, full_json,
                                       return_empty);
            it++;
        }
    if (!return_empty)
        value_to_return = std::move (full_json);

    return std::move (value_to_return);
}

void
cjparse::return_the_value_internal (std::string object_name,
                                    json_value &first_value,
                                    json_value &value_to_return,
                                    bool &return_empty)
{
    json_value temp_value = first_value;

    if (std::holds_alternative<json_object> (temp_value))
        {
            auto value_object = std::get<json_object> (temp_value);
            auto iter = value_object.find (object_name);
            if (iter != value_object.end ())
                {
                    value_to_return = std::move (iter->second.value);
                    return;
                }
            else
                {
                    return_empty = true;
                    return;
                }
        }
    if (std::holds_alternative<json_array> (temp_value))
        {
            auto value_array = std::get<json_array> (temp_value);
            auto iter = value_array.begin ();
            while (iter != value_array.end ())
                {
                    return_the_value_internal (object_name, iter->value,
                                               value_to_return, return_empty);
                    iter++;
                }
        }

    value_to_return = std::move (temp_value);
}

template <class T>
bool
cjparse::check_if_type (std::string object_name)
{
    bool bool_to_return = false; // here so we can reuse internal method
    json_value full_json;
    std::visit ([&full_json] (auto value) { full_json = value; }, JSON.value);

    check_if_type_internal<T> (object_name, full_json, full_json,
                               bool_to_return);

    return bool_to_return;
}

template <class T>
bool
cjparse::check_if_type (std::vector<std::string> object_name_vector_in)
{
    bool bool_to_return = false;
    json_value full_json, value_to_return;
    std::visit ([&full_json] (auto value) { full_json = value; }, JSON.value);

    auto it = object_name_vector_in.begin ();

    while (it != object_name_vector_in.end ())
        {
            check_if_type_internal<T> (*it, full_json, full_json,
                                       bool_to_return);
            it++;
        }
    return bool_to_return;
}

template <class T>
void
cjparse::check_if_type_internal (std::string name_to_check_if_type,
                                 json_value &first_value,
                                 json_value &value_to_alter,
                                 bool &bool_to_return)
{

    json_value temp_value = first_value;

    if (std::holds_alternative<json_object> (temp_value))
        {

            auto value_object = std::get<json_object> (temp_value);
            auto iter = value_object.find (name_to_check_if_type);
            if (iter != value_object.end ())
                {
                    value_to_alter = std::move (iter->second.value);
                    if (std::holds_alternative<T> (value_to_alter))
                        bool_to_return = true;
                    else
                        bool_to_return = false;

                    return;
                }
            else
                {
                    bool_to_return = false;
                    return;
                }
        }
    if (std::holds_alternative<json_array> (temp_value))
        {
            auto value_array = std::get<json_array> (temp_value);
            auto iter = value_array.begin ();
            while (iter != value_array.end ())
                {
                    check_if_type_internal<T> (name_to_check_if_type,
                                               iter->value, value_to_alter,
                                               bool_to_return);
                    iter++;
                }
        }
}
