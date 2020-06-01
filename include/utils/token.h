//
// Created by Emilien Lemaire on 21/04/2020.
//

#pragma once


enum token {
    tok_eof = -1,

    //types
    tok_type = -2,

    //commands
    tok_include = -5,
    tok_return = -6,

    //primary
    tok_identifier = -7,
    tok_val_int = -8,
    tok_val_float = -9,
    tok_val_char = -10,

    //syntax
    tok_sc = -11,
    tok_eq = -12,
    tok_popen = -13,
    tok_pclose = -14,
    tok_bopen = -15,
    tok_bclose = -16,
    tok_comma = -17
};