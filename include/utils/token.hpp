//
// Created by Emilien Lemaire on 21/04/2020.
//

#pragma once


#include <string>
enum token {
    tok_eof = -1,
    tok_eol = -2,

    //types
    tok_type = -3,

    //commands
    tok_include = -6,
    tok_return = -7,

    //primary
    tok_identifier = -8,
    tok_val_int = -9,
    tok_val_float = -10,
    tok_val_char = -11,

    //syntax
    tok_sc = -12,
    tok_eq = -13,
    tok_popen = -14,
    tok_pclose = -15,
    tok_bopen = -16,
    tok_bclose = -17,
    tok_comma = -18
};

struct Token {
    int token;
    std::string identifier = "";
    std::string valueStr = "";
};
