//
// Created by Emilien Lemaire on 19/04/2020.
//

#include "Lexer/Lexer.hpp"
#include "utils/token.h"
#include <cstdio>
#include <cstring>
#include <iostream>

Lexer::Lexer(const char *path) {
    m_IsCLI = strlen(path) == 0;

    std::cout << strlen(path) << std::endl;

    std::cout << (m_IsCLI ? "A JIT Compiler" : "Not a JIT Compiler") << std::endl;

    m_File = !m_IsCLI ? std::fopen(path, "r") : nullptr;

    if (!m_File && !m_IsCLI) {
        std::string str("Failed to open file: ");
        str += path;
        std::perror(str.c_str());
    } else if (!m_IsCLI) {
        std::cout << "File opened successfully" << std::endl;
    } else {
        std::cout << "YAPL >>>";
    }
}

Lexer::~Lexer() {
    if (m_IsCLI) {
        std::fclose(m_File); 
    }
}

int Lexer::getChar() {
    m_CurrentChar = m_IsCLI ?
        getchar() :
        std::fgetc(m_File);
    m_CharCount++;
    if (m_CurrentChar == '\n')
        m_LineCount++;
    return m_CurrentChar;
}

int Lexer::getToken() {
    while (isspace(m_CurrentChar)) {
        getChar();
    }

    switch (m_CurrentChar) {
        case ';':
            getChar();
            return tok_sc;
        case '=':
            getChar();
            return tok_eq;
        case '(':
            getChar();
            return tok_popen;
        case ')':
            getChar();
            return tok_pclose;
        case '{':
            getChar();
            return tok_bopen;
        case '}':
            getChar();
            return tok_bclose;
        case ',':
            getChar();
            return tok_comma;
        default:
            break;
    }



    if (isalpha(m_CurrentChar)) {
        m_Identifier = m_CurrentChar;

        while (isalnum(getChar())) {
            m_Identifier += m_CurrentChar;
        }

        if (m_Identifier == "include") {
            return tok_include;
        }

        if (m_Identifier == "return") {
            return tok_return;
        }

        if (m_Identifier == "float") {
            return tok_type;
        }

        if (m_Identifier == "int") {
            return tok_type;
        }

        return tok_identifier;
    }

    if(isdigit(m_CurrentChar) || m_CurrentChar == '.') {
        m_ValueStr = "";
        bool isFloat = false;

        do {
            if (m_CurrentChar == '.') {
                if (isFloat) {
                    break;
                } else {
                    isFloat = true;
                }
            }
            m_ValueStr += m_CurrentChar;
            getChar();
        } while (isdigit(m_CurrentChar) || m_CurrentChar == '.');

        if (isFloat) {
            return tok_val_float;
        }

        return tok_val_int;
    }

    int tok = m_CurrentChar;
    getChar();
    return tok;
}

const std::string &Lexer::getIdentifier() const {
    return m_Identifier;
}

const std::string &Lexer::getValueStr() const {
    return m_ValueStr;
}

int Lexer::getCurrentChar() const {
    return m_CurrentChar;
}

const int &Lexer::getCharCount() const {
    return m_CharCount;
}

const int &Lexer::getLineCount() const {
    return m_LineCount;
}
