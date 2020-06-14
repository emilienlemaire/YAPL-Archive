//
// Created by Emilien Lemaire on 19/04/2020.
//

#pragma once


#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "utils/token.hpp"
#include "CppLogger2/CppLogger2.h"


class Lexer {
private:
    FILE* m_File;
    bool m_HasFile;
    std::string m_Identifier;
    std::string m_ValueStr;
    int m_CurrentChar = ' ';
    int m_CharCount = 0;
    int m_LineCount = 1;
    CppLogger::CppLogger m_Logger;

public:
    Lexer(const char* path);
    ~Lexer();

    int getChar();
    Token getToken();

    const std::string &getIdentifier() const;

    const std::string &getValueStr() const;

    int getCurrentChar() const;
    const int &getCharCount() const;
    const int &getLineCount() const;

    const bool hasFile() const;
};



