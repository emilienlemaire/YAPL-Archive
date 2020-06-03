//
// Created by Emilien Lemaire on 20/04/2020.
//

#pragma once

#include <string>

#include "utils/token.h"
#include "utils/type.h"

static type strToType(const std::string &str) {
    if (str == "int")
        return type_int;
    if (str == "float")
        return type_float;
    if (str == "char")
        return type_char;

    return type_void;
}

static std::string typeToString(type t_Type) {
    switch (t_Type) {
        case type_int:
            return "int";
        case type_float:
            return "float";
        case type_char:
            return "char";
        case type_void:
            return "void";
    }
}

static int getTokenPrecedence(int tok) {
    if (!isascii(tok)) {
        return -1;
    }

    switch (tok) {
        case '<':
            return 10;
        case '+':
            return 20;
        case '-':
            return 20;
        case '*':
            return 40;
        case '/':
            return 40;
        default:
            return -1;
    }
}

static std::string tokToString(int tok) {
    switch (tok) {
        case tok_eof:
            return "eof";
        case tok_type:
            return "type";
        case tok_include:
            return "include";
        case tok_return:
            return "return";
        case tok_identifier:
            return "identifier";
        case tok_val_int:
            return "val_int";
        case tok_val_float:
            return "val_float";
        case tok_val_char:
            return "val_char";
        case tok_sc:
            return "sc";
        case tok_eq:
            return "eq";
        default:
            return std::string(1, (char)tok);
        case tok_popen:
            return "popen";
        case tok_pclose:
            return "pclose";
        case tok_bopen:
            return "bopen";
        case tok_bclose:
            return "bclose";
        case tok_comma:
            return "comma";
        case tok_eol:
            return "eol";
        case INT_MIN:
            return "IO Waiting";
    }
}
