#include "cjparse.h"

#pragma once
#ifndef CJPARSE_JSON_GENERATOR
#define CJPARSE_JSON_GENERATOR

class cjparse_json_generator
{
  public:
    cjparse_json_generator (cjparse::cjparse_json_value &JSON,
                            bool generate_pretty_json);
    cjparse_json_generator (cjparse::json_value &JSON,
                            bool generate_pretty_json);

  public:
    void print_json_from_memory (cjparse::cjparse_json_value &JSON);
    void print_json_number (cjparse::json_number &num);

  public: // for user to use the string at will.
    std::string JSON_string;

  private:
    bool pretty;
    int n_of_iteration = 0;
};

#endif // CJPARSE_JSON_GENERATOR
