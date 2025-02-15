#include "../include/cjparse_json_parser.h"

cjparse_json_parser::cjparse_json_parser (
    std::string &str, cjparse::cjparse_json_value &JSON_container)
{
    cjparse_parse_value (str, JSON_container);
}

cjparse_json_parser::cjparse_json_parser (
    std::string &str, cjparse::cjparse_json_value &JSON_container,
    std::string json_string_pattern_to_keep_raw)
{
    inside_str_ignore.push_back (json_string_pattern_to_keep_raw);
    cjparse_parse_value (str, JSON_container);
}

cjparse_json_parser::cjparse_json_parser (
    std::string &str, cjparse::cjparse_json_value &JSON_container,
    std::vector<std::string> json_string_pattern_to_keep_raw)
{
    inside_str_ignore = json_string_pattern_to_keep_raw;

    cjparse_parse_value (str, JSON_container);
}

int
cjparse_json_parser::check_what_is_the_value (std::string &str)
{
    if (str.length () == 0)
        {
            // ERROR 0;
        }
    if (str[0] == '{')
        return 1; // OBJECT
    else if (str[0] == '[')
        return 2; // ARRAY
    else if (str[0] == '\"')
        return 3; // STRING
    else if (std::isdigit (str[0]) || str[0] == '-')
        return 4; // NUMBER
    else if (str == "true" || str == "false")
        return 5; // BOOL
    else if (str == "null")
        return 6; // NULL
    else
        return 0; // bad JSON
};

std::string
cjparse_json_parser::decode_unicode (const std::string &raw_str)
{
    std::string result;
    size_t i = 0;

    while (i < raw_str.size ())
        {
            // Check if the current substring matches any raw pattern
            bool is_raw = false;
            if (!inside_str_ignore
                     .empty ()) // Only check if there are patterns
                {
                    for (const auto &pattern : inside_str_ignore)
                        {
                            if (raw_str.compare (i, pattern.size (), pattern)
                                == 0)
                                {
                                    result
                                        += pattern; // Append raw pattern as-is
                                    i += pattern.size ();
                                    is_raw = true;
                                    break; // Stop checking once a match is
                                           // found
                                }
                        }
                }
            if (is_raw)
                continue; // Skip further processing for this substring

            if (raw_str[i] == '\\' && i + 1 < raw_str.size ())
                {
                    std::string escape_seq = raw_str.substr (
                        i, 2); // Get two-character escape sequence

                    // If the escape sequence is in inside_str_ignore, append
                    // as-is
                    if (std::find (inside_str_ignore.begin (),
                                   inside_str_ignore.end (), escape_seq)
                        != inside_str_ignore.end ())
                        {
                            result += escape_seq;
                            i += 2;
                            continue;
                        }

                    // Handle common escape sequences
                    switch (raw_str[i + 1])
                        {
                        case 'n':
                            result += '\n';
                            i += 2;
                            break;
                        case 't':
                            result += '\t';
                            i += 2;
                            break;
                        case '"':
                            result += '"';
                            i += 2;
                            break;
                        case '\\':
                            result += '\\';
                            i += 2;
                            break;

                        case 'u': // Unicode escape sequence
                            if (i + 5 < raw_str.size ())
                                {
                                    std::stringstream ss;
                                    ss << std::hex
                                       << raw_str.substr (i + 2, 4);
                                    int codePoint;
                                    ss >> codePoint;

                                    if (codePoint >= 0xD800
                                        && codePoint <= 0xDBFF
                                        && i + 9 < raw_str.size ()
                                        && raw_str[i + 6] == '\\'
                                        && raw_str[i + 7] == 'u')
                                        {
                                            std::stringstream lowSurrogate;
                                            lowSurrogate
                                                << std::hex
                                                << raw_str.substr (i + 8, 4);
                                            int lowCodePoint;
                                            lowSurrogate >> lowCodePoint;

                                            codePoint
                                                = ((codePoint - 0xD800) << 10)
                                                  + (lowCodePoint - 0xDC00)
                                                  + 0x10000;
                                            i += 6;
                                        }

                                    if (codePoint <= 0x7F)
                                        {
                                            result += static_cast<char> (
                                                codePoint);
                                        }
                                    else if (codePoint <= 0x7FF)
                                        {
                                            result += static_cast<char> (
                                                (codePoint >> 6) | 0xC0);
                                            result += static_cast<char> (
                                                (codePoint & 0x3F) | 0x80);
                                        }
                                    else if (codePoint <= 0xFFFF)
                                        {
                                            result += static_cast<char> (
                                                (codePoint >> 12) | 0xE0);
                                            result += static_cast<char> (
                                                ((codePoint >> 6) & 0x3F)
                                                | 0x80);
                                            result += static_cast<char> (
                                                (codePoint & 0x3F) | 0x80);
                                        }
                                    else
                                        {
                                            result += static_cast<char> (
                                                (codePoint >> 18) | 0xF0);
                                            result += static_cast<char> (
                                                ((codePoint >> 12) & 0x3F)
                                                | 0x80);
                                            result += static_cast<char> (
                                                ((codePoint >> 6) & 0x3F)
                                                | 0x80);
                                            result += static_cast<char> (
                                                (codePoint & 0x3F) | 0x80);
                                        }

                                    i += 6;
                                }
                            break;

                        default:
                            // If it's an unknown escape sequence, just add the
                            // backslash as-is
                            result += raw_str[i];
                            i++;
                        }
                }
            else
                {
                    result += raw_str[i];
                    i++;
                }
        }

    return result;
}

cjparse::json_string
cjparse_json_parser::cjparse_parse_value_string (std::string &str)
{
    std::size_t st_of_string = str.find ('\"', 0);
    std::size_t en_of_string
        = return_the_matching_pattern (str, 0, '\"', '\"');

    std::string str_to_ret = decode_unicode (
        str.substr (st_of_string + 1, en_of_string - st_of_string - 1));

    return str_to_ret;
}

cjparse::json_number
cjparse_json_parser::cjparse_parse_value_number (std::string &str)
{
    std::string str_number_only;
    for (char ch : str)
        {
            if (std::isdigit (ch))
                str_number_only.push_back (ch);
            if (ch == 'e' || ch == 'E')
                str_number_only.push_back (ch);
            if (ch == '.')
                str_number_only.push_back (ch);
            if (ch == '-' || ch == '+')
                str_number_only.push_back (ch);
        }
    // If nothing valid was found, throw an error.
    if (str_number_only.empty ())
        {
            throw std::invalid_argument ("No valid numeric characters found.");
        }
    try
        {
            // If we detect a decimal point or exponent, treat it as a floating
            // point number.
            if (str_number_only.find ('.') != std::string::npos
                || str_number_only.find ('e') != std::string::npos
                || str_number_only.find ('E') != std::string::npos)
                {
                    // Convert using std::stod (or std::stold if you need more
                    // precision)
                    double d = std::stod (str_number_only);
                    return d; // json_number will hold a double
                }
            else
                {
                    // For integer numbers, use stoll first to capture large
                    // values.
                    long long int ll = std::stoll (str_number_only);

                    // Choose the smallest type that can fit the value.
                    if (ll >= std::numeric_limits<int>::min ()
                        && ll <= std::numeric_limits<int>::max ())
                        {
                            return static_cast<int> (ll);
                        }
                    else if (ll >= std::numeric_limits<long int>::min ()
                             && ll <= std::numeric_limits<long int>::max ())
                        {
                            return static_cast<long int> (ll);
                        }
                    else
                        {
                            return ll; // fits in long long int
                        }
                }
        }
    catch (const std::out_of_range &)
        {
            // If the number is too large for integer types, return max
            // possible
            if (str_number_only.find ('.') != std::string::npos
                || str_number_only.find ('e') != std::string::npos
                || str_number_only.find ('E') != std::string::npos)
                {
                    return std::numeric_limits<
                        double>::max (); // Max representable double
                }
            else
                {
                    return std::numeric_limits<
                        long long int>::max (); // Max integer value
                }
        }
    catch (const std::invalid_argument &)
        {
            throw std::invalid_argument (
                "Conversion error: invalid argument in number conversion.");
        }
}

bool
cjparse_json_parser::cjparse_parse_value_bool (std::string &str)
{
    if (str.substr (0, 4) == "true")
        return true;
    else
        return false;
}

cjparse::json_null
cjparse_json_parser::cjparse_parse_value_null (std::string &str)
{
    return std::any ();
}

void
cjparse_json_parser::cjparse_parse_array (std::string &str,
                                          cjparse::cjparse_json_value &value)
{
    // TO BE FIXXED SOON
    cjparse::json_array array;
    cjparse::cjparse_json_value temp_value;

    std::size_t outter_initial_delimeter = str.find_first_of ('[', 0);
    std::size_t outter_final_delimiter = return_the_matching_pattern (
        str, outter_initial_delimeter, '[', ']');

    std::string obj_name, str_temp;

    bool nested = false;
    int state = 0;

    bool keep_looping = true;

    std::size_t curr_outer_initial_delimeter,
        curr_outer_final_delimeter = outter_initial_delimeter,
        not_white_after_curr_outer_initial, not_white_after_curr_outer_final;
    while (keep_looping)
        {
            if (state == 0)
                {
                    curr_outer_initial_delimeter = str.find_first_not_of (
                        { str[curr_outer_final_delimeter], 0x20, 0x0a, 0x0d,
                          0x09 },
                        curr_outer_final_delimeter);
                    if (str[curr_outer_initial_delimeter] == ',')
                        curr_outer_initial_delimeter = str.find_first_not_of (
                            { str[curr_outer_initial_delimeter], 0x20, 0x0a,
                              0x0d, 0x09 },
                            curr_outer_initial_delimeter);
                    not_white_after_curr_outer_initial = str.find_first_of (
                        { ',', '}', ']' }, curr_outer_initial_delimeter + 1);
                    str_temp = std::move (
                        str.substr (curr_outer_initial_delimeter,
                                    not_white_after_curr_outer_initial
                                        - curr_outer_initial_delimeter));
                    int temp_state = check_what_is_the_value (str_temp);
                    if (temp_state == 1 || temp_state == 2 || temp_state == 3)
                        state = 1;
                    if (temp_state == 4 || temp_state == 5 || temp_state == 6)
                        state = 2;
                }
            if (state == 1)
                {
                    find_delimeters_check_if_comma_alter_state (
                        str, curr_outer_initial_delimeter,
                        curr_outer_final_delimeter,
                        not_white_after_curr_outer_final,
                        str[curr_outer_initial_delimeter]);
                    state = 5;
                }
            if (state == 2)
                {
                    curr_outer_final_delimeter
                        = not_white_after_curr_outer_initial;
                    not_white_after_curr_outer_final
                        = str.find_first_not_of ({ 0x20, 0x0a, 0x0d, 0x09 },
                                                 curr_outer_final_delimeter);
                    state = 3;
                }
            if (state == 3)
                {
                    state = 7;
                    if (not_white_after_curr_outer_final != std::string::npos
                        && not_white_after_curr_outer_final
                               <= outter_final_delimiter)
                        state = 4;
                    else
                        nested = false;
                }
            if (state == 4)
                {
                    nested = str[not_white_after_curr_outer_final] == ',';
                    state = 7;
                }
            if (state == 5)
                {
                    state = 8;
                    if (not_white_after_curr_outer_final != std::string::npos
                        && not_white_after_curr_outer_final
                               <= outter_final_delimiter)
                        state = 6;
                    else
                        nested = false;
                }
            if (state == 6)
                {
                    nested = str[not_white_after_curr_outer_final] == ',';
                    state = 8;
                }
            if (state == 7)
                {
                    std::string str_value = std::move (
                        str.substr (curr_outer_initial_delimeter,
                                    curr_outer_final_delimeter
                                        - curr_outer_initial_delimeter));
                    cjparse_json_parser::cjparse_parse_value (str_value,
                                                              temp_value);
                    array.push_back (std::move (temp_value));
                    if (nested)
                        state = 0;
                    else
                        keep_looping = false;
                }
            if (state == 8)
                {
                    std::string str_value = std::move (
                        str.substr (curr_outer_initial_delimeter,
                                    curr_outer_final_delimeter
                                        - curr_outer_initial_delimeter + 1));
                    cjparse_json_parser::cjparse_parse_value (str_value,
                                                              temp_value);
                    array.push_back (std::move (temp_value));
                    if (nested)
                        state = 0;
                    else
                        keep_looping = false;
                }
        }

    value = cjparse::cjparse_json_value (array);
}

void
cjparse_json_parser::cjparse_parse_object (std::string &str,
                                           cjparse::cjparse_json_value &value)
{
    cjparse::json_object object;
    cjparse::cjparse_json_value temp_value;

    std::size_t outter_initial_delimeter = str.find_first_of ('{', 0);
    std::size_t outter_final_delimiter = return_the_matching_pattern (
        str, outter_initial_delimeter, '{', '}');

    std::size_t st_of_name, en_of_name, double_point_after_name,
        not_white_after_double_point;
    std::string obj_name, str_temp;

    bool nested = false;
    int state = 0;

    bool keep_looping = true;

    std::size_t curr_outer_initial_delimeter,
        curr_outer_final_delimeter = outter_initial_delimeter,
        not_white_after_curr_outer_initial, not_white_after_curr_outer_final;

    while (keep_looping)
        {
            if (state == 0)
                {
                    st_of_name = str.find_first_of (
                        '\"', curr_outer_final_delimeter + 1);
                    en_of_name = return_the_matching_pattern (str, st_of_name,
                                                              '\"', '\"');
                    obj_name = std::move (str.substr (
                        st_of_name + 1, en_of_name - st_of_name - 1));
                    double_point_after_name
                        = str.find_first_of (':', en_of_name + 1);
                    if (str[double_point_after_name - 1] == '\\')
                        {
                        } // HORRIBLE JSON

                    curr_outer_initial_delimeter
                        = str.find_first_not_of ({ 0x20, 0x0a, 0x0d, 0x09 },
                                                 double_point_after_name + 1);
                    not_white_after_curr_outer_initial = str.find_first_of (
                        { ',', '}', ']' }, curr_outer_initial_delimeter);
                    str_temp = std::move (
                        str.substr (curr_outer_initial_delimeter,
                                    not_white_after_curr_outer_initial
                                        - curr_outer_initial_delimeter));
                    int temp_state = check_what_is_the_value (str_temp);
                    if (temp_state == 1 || temp_state == 2 || temp_state == 3)
                        state = 1;
                    if (temp_state == 4 || temp_state == 5 || temp_state == 6)
                        state = 2;
                }
            if (state == 1)
                {
                    find_delimeters_check_if_comma_alter_state (
                        str, curr_outer_initial_delimeter,
                        curr_outer_final_delimeter,
                        not_white_after_curr_outer_final,
                        str[curr_outer_initial_delimeter]);
                    state = 5;
                }
            if (state == 2)
                {
                    curr_outer_final_delimeter
                        = not_white_after_curr_outer_initial;
                    not_white_after_curr_outer_final
                        = str.find_first_not_of ({ 0x20, 0x0a, 0x0d, 0x09 },
                                                 curr_outer_final_delimeter);
                    state = 3;
                }
            if (state == 3)
                {
                    state = 7;
                    if (not_white_after_curr_outer_final != std::string::npos
                        && not_white_after_curr_outer_final
                               <= outter_final_delimiter)
                        state = 4;
                    else
                        nested = false;
                }
            if (state == 4)
                {
                    nested = str[not_white_after_curr_outer_final] == ',';
                    state = 7;
                }
            if (state == 5)
                {
                    state = 8;
                    if (not_white_after_curr_outer_final != std::string::npos
                        && not_white_after_curr_outer_final
                               <= outter_final_delimiter)
                        state = 6;
                    else
                        nested = false;
                }
            if (state == 6)
                {
                    nested = str[not_white_after_curr_outer_final] == ',';
                    state = 8;
                }
            if (state == 7)
                {
                    std::string str_value = std::move (
                        str.substr (curr_outer_initial_delimeter,
                                    curr_outer_final_delimeter
                                        - curr_outer_initial_delimeter));
                    cjparse_json_parser::cjparse_parse_value (str_value,
                                                              temp_value);
                    auto repeted = object.find (obj_name);
                    bool name_already_exists = repeted != object.end ();
                    if (name_already_exists)
                        obj_name = obj_name + std::to_string (rand () % 1000);

                    object[obj_name] = temp_value;
                    if (nested)
                        state = 0;
                    else
                        keep_looping = false;
                }
            if (state == 8)
                {
                    std::string str_value = std::move (
                        str.substr (curr_outer_initial_delimeter,
                                    curr_outer_final_delimeter
                                        - curr_outer_initial_delimeter + 1));
                    cjparse_json_parser::cjparse_parse_value (str_value,
                                                              temp_value);
                    auto repeted = object.find (obj_name);
                    bool name_already_exists = repeted != object.end ();
                    if (name_already_exists)
                        obj_name = obj_name
                                   + std::to_string (
                                       rand ()
                                       % 1000); // will have to change this to
                                                // make it predictable.....

                    object[obj_name] = std::move (temp_value);
                    if (nested)
                        state = 0;
                    else
                        keep_looping = false;
                }
        }

    value = cjparse::cjparse_json_value (object);
}

void
cjparse_json_parser::cjparse_parse_value (std::string &str,
                                          cjparse::cjparse_json_value &value)
{
    int what_is_the_value = check_what_is_the_value (str);
    if (what_is_the_value == 1)
        {
            cjparse_json_parser::cjparse_parse_object (str, value);
        }
    if (what_is_the_value == 2)
        {
            cjparse_json_parser::cjparse_parse_array (str, value);
        }
    if (what_is_the_value == 3)
        {
            value = cjparse_json_parser::cjparse_parse_value_string (str);
        }
    if (what_is_the_value == 4)
        {
            value = cjparse_json_parser::cjparse_parse_value_number (str);
        }
    if (what_is_the_value == 5)
        {
            value = cjparse_json_parser::cjparse_parse_value_bool (str);
        }
    if (what_is_the_value == 6)
        {
            cjparse_json_parser::cjparse_parse_value_null (str);
        }
}

std::size_t
cjparse_json_parser::return_the_matching_pattern (
    std::string &str, std::size_t pos_of_bracket_to_match, char pattern,
    char pattern_to_match)
{
    std::size_t size = str.length ();

    int numb_of_pattern_between = 0;
    int numb_of_pattern_match_between = 0;
    std::size_t pos_of_nearest_pattern_match = pos_of_bracket_to_match;

    bool keep_looping = true;

    unsigned char state = 0;
    while (keep_looping)
        {
            if (state == 0)
                {
                    numb_of_pattern_between = 0;
                    numb_of_pattern_match_between = 0;
                    pos_of_nearest_pattern_match = str.find_first_of (
                        pattern_to_match, pos_of_nearest_pattern_match + 1);
                    if (pos_of_nearest_pattern_match == std::string::npos)
                        return std::string::npos;

                    for (std::size_t i = pos_of_bracket_to_match + 1;
                         i < pos_of_nearest_pattern_match; i++)
                        {
                            if (str[i] == pattern && str[i - 1] != '\\')
                                numb_of_pattern_between++;

                            if (str[i] == pattern_to_match
                                && str[i - 1] != '\\')
                                numb_of_pattern_match_between++;
                        }
                    state = 1;
                }
            if (state == 1)
                {
                    if (numb_of_pattern_between
                        == numb_of_pattern_match_between)
                        return pos_of_nearest_pattern_match;

                    if (numb_of_pattern_between
                        != numb_of_pattern_match_between)
                        state = 0;

                    if (numb_of_pattern_between == 0
                        && numb_of_pattern_match_between != 0)
                        {
                            keep_looping = false;
                            return std::string::npos;
                        } // BIG FAT ERROR , HORRIBLE JSON
                }
        }

    return std::string::npos;
}

void
cjparse_json_parser::find_delimeters_check_if_comma_alter_state (
    std::string &str, std::size_t &initial_delimeter,
    std::size_t &final_delimeter,
    std::size_t &not_white_position_after_final_delimiter, char pattern)
{
    char pattern_to_match;
    if (pattern == '{')
        pattern_to_match = '}';
    else if (pattern == '[')
        pattern_to_match = ']';
    else if (pattern == '\"')
        pattern_to_match = pattern;
    else
        return;

    final_delimeter = return_the_matching_pattern (str, initial_delimeter,
                                                   pattern, pattern_to_match);
    not_white_position_after_final_delimiter = str.find_first_not_of (
        { pattern_to_match, 0x20, 0x0a, 0x0d, 0x09 }, final_delimeter);
}
