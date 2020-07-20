//
// Created by Emilien Lemaire on 19/04/2020.
//

#include <cstdio>
#include <cstring>
#include <iostream>

#include "Lexer/Lexer.hpp"
#include "utils/token.hpp"

Lexer::Lexer(const char *path)
    :m_Logger(CppLogger::Level::Trace, "Lexer", true)
{
    m_HasFile = strlen(path) != 0;

    m_File = m_HasFile ? std::fopen(path, "r") : nullptr;

    CppLogger::Format lexerFormat({
            CppLogger::FormatAttribute::Name,
            CppLogger::FormatAttribute::Message
            });

    m_Logger.setFormat(lexerFormat);

    if (!m_File && m_HasFile) {
        m_Logger.printFatalError("Couldn't open file: {}", path);
    } else if (m_HasFile) {
        m_Logger.printInfo("File opened: {}", path);
    }
}

Lexer::~Lexer() {
    if (m_HasFile) {
        std::fclose(m_File);
    }
}

int Lexer::getChar() {
    m_CurrentChar = !m_HasFile ?
        std::fgetc(stdin) :
        std::fgetc(m_File);
    m_CharCount++;
    if (m_CurrentChar == '\n'){
        m_LineCount++;
    }

    return m_CurrentChar;
}

Token Lexer::getToken() {

    while (isspace(m_CurrentChar)) {
        getChar();
    }

    std::string test;

    switch (m_CurrentChar) {
        case ';':
            getChar();
            return Token{ token::tok_sc };
        case '=':
            getChar();
            return Token{ token::tok_eq };
        case '(':
            getChar();
            return Token{ token::tok_popen };
        case ')':
            getChar();
            return Token{ token::tok_pclose };
        case '{':
            getChar();
            return Token{ token::tok_bopen };
        case '}':
            getChar();
            return Token{ token::tok_bclose };
        case ',':
            getChar();
            return Token{ token::tok_comma };
        default:
            break;
    }



    if (isalpha(m_CurrentChar)) {
        m_Identifier = m_CurrentChar;

        while (isalnum(getChar())) {
            m_Identifier += m_CurrentChar;
        }

        if (m_Identifier == "include") {
            return Token{ token::tok_include };
        }

        if (m_Identifier == "return") {
            return Token{ token::tok_return };
        }

        if (m_Identifier == "float" || m_Identifier == "double") {
            return Token{ token::tok_type, m_Identifier };
        }

        if (m_Identifier == "int") {
            return Token{ token::tok_type, m_Identifier };
        }

        if (m_Identifier == "if") {
            return Token{ token::tok_if };
        }

        if (m_Identifier == "else") {
            return Token{ token::tok_else };
        }
        
        if (m_Identifier == "for") {
            return Token{ token::tok_for };
        }
        
        if (m_Identifier == "exit") {
            return Token{ token::tok_eof };
        }
        return Token{ token::tok_identifier, m_Identifier };
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
            return Token{ token::tok_val_float, "", m_ValueStr };
        }

        return Token{ token::tok_val_int, "", m_ValueStr };
    }

    int tok = m_CurrentChar;
    getChar();
    return Token{ tok };
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

const bool Lexer::hasFile() const {
    return m_HasFile;
}

