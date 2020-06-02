//
// Created by Emilien Lemaire on 21/04/2020.
//

#include <iostream>
#include <memory>
#include <helper/helper.hpp>
#include <utils/token.h>

#include "Parser/Parser.hpp"

Parser::Parser(std::shared_ptr<Lexer> lexer)
        : m_Lexer(std::move(lexer))
{
    m_CurrentToken = m_Lexer->getToken();
}

void Parser::parse() {

    while (m_CurrentToken != tok_eof) {
        switch (m_CurrentToken) {
            case tok_type:
                parseDeclaration();
                std::cout << "yapl >>>";
                break;
            case tok_include:
                parseInclude();
                std::cout << "yapl >>>";
                break;
            case tok_sc:
                m_CurrentToken = m_Lexer->getToken();
                std::cout << "yapl >>>";
                break;
            default:
                parseTopLevelExpr();
                std::cout << "tok_sc" << std::endl;
                break;
        }
    }
}

std::shared_ptr<ExprAST> Parser::parseNext() {
    switch (m_CurrentToken) {
        case tok_type:
            return parseDeclaration();
        case tok_include:
            return nullptr;
        case tok_eof:
            return std::make_unique<EOFExprAST>();
        default:
            return parseTopLevelExpr();
    }
}

std::shared_ptr<DeclarationAST> Parser::parseDeclaration() {
    std::string dType = m_Lexer->getIdentifier();
    std::string dName;

    m_CurrentToken = m_Lexer->getToken();

    if (m_CurrentToken != tok_identifier) {
        std::cerr << "The type must be followed by an identifier!" << std::endl;
        return nullptr;
    }

    dName = m_Lexer->getIdentifier();

#ifdef VERBOSE_PARSING
    std::cout << "Parsing declaration with type: " << dType <<
        " and name: " << dName << std::endl;
#endif

    m_CurrentToken = m_Lexer->getToken();

    if (m_CurrentToken == tok_popen) {

#ifdef VERBOSE_PARSING
    std::cout << "Parsing prototype." << std::endl;
#endif

        auto declaration = std::make_shared<DeclarationAST>(dType, dName);
        auto proto = parsePrototype(std::move(declaration));

#ifdef VERBOSE_PARSING
        auto args = proto->getParams();
        std::cout << "Parsed prototype with arguments:";

        for (const auto arg : args) {
            std::cout << " " << arg->getType() << " " << arg->getName() << ";";
        }

        std::cout << std::endl;

        std::cout << "Current token: " << tokToString(m_CurrentToken) << std::endl;
#endif

        if (m_CurrentToken == tok_sc) {
            return proto;
        }

        if (m_CurrentToken == tok_bopen) {
            return parseDefinition(proto);
        }

        if (m_CurrentToken != tok_sc) {
            std::cerr << "Expected function body or ';' after prototype" << std::endl;
            m_CurrentToken = m_Lexer->getToken();
            return nullptr;
        }

        return proto;
    }

    if (m_CurrentToken == tok_eq) {
        auto declaration = std::make_shared<DeclarationAST>(dType, dName);
        return parseVariableDefinition(declaration);
    }

    if (m_CurrentToken != tok_sc) {
        std::cerr << "Expected ';' after variable declaration!" << std::endl;
        return nullptr;
    }

    m_CurrentToken = m_Lexer->getToken();
    std::cout << "Found a variable declaration: " << dType << " " << dName << std::endl;

    return std::make_shared<DeclarationAST>(dType, dName);
}

void Parser::parseInclude() {
    m_CurrentToken = m_Lexer->getToken();
}

std::shared_ptr<PrototypeAST> Parser::parsePrototype(std::shared_ptr<DeclarationAST> declarationAST) {
    std::vector<std::shared_ptr<DeclarationAST>> args;

#ifdef VERBOSE_PARSING
    std::cout << "Starting prototype parsing" << std::endl;
    std::cout << "Current token: " << tokToString(m_CurrentToken) << std::endl;
#endif

    m_CurrentToken = m_Lexer->getToken();

#ifdef VERBOSE_PARSING
    std::cout << "Current token: " << tokToString(m_CurrentToken) << std::endl;
#endif

    if (m_CurrentToken != tok_type) {
        if (m_CurrentToken == tok_pclose) {

#ifdef VERBOSE_PARSING
            std::cout << "Found proto without args" << std::endl;
#endif

            std::cout << "Found prototype: " << declarationAST->getType() << " " << declarationAST->getName() << std::endl;
            return std::make_shared<PrototypeAST>(std::move(declarationAST), std::move(args));
        } else {
            std::cerr << "Parameters must be typed!" << std::endl;
            return nullptr;
        }
    }

    std::string argType = m_Lexer->getIdentifier();
    std::string argName;
    m_CurrentToken = m_Lexer->getToken();

    if (m_CurrentToken != tok_identifier) {
        std::cerr << "Parameters must be named!" << std::endl;
        return nullptr;
    }

    argName = m_Lexer->getIdentifier();
    auto fstArg = std::make_shared<DeclarationAST>(argType, argName);
    args.push_back(std::move(fstArg));

    m_CurrentToken = m_Lexer->getToken();

    while (m_CurrentToken == tok_comma) {
        m_CurrentToken = m_Lexer->getToken();

        if (m_CurrentToken != tok_type) {
            std::cerr << "Parameters must be typed!" << std::endl;
            return nullptr;
        }

        argType = m_Lexer->getIdentifier();
        m_CurrentToken = m_Lexer->getToken();

        if (m_CurrentToken != tok_identifier) {
            std::cerr << "Parameters must be named!" << std::endl;
            return nullptr;
        }

        argName = m_Lexer->getIdentifier();

        for (const auto &arg : args) {
            if (arg->getName() == argName)
                return nullptr;
        }

        auto loopArg = std::make_shared<DeclarationAST>(argType, argName);

        args.push_back(std::move(loopArg));

        m_CurrentToken = m_Lexer->getToken();
    }

    if (m_CurrentToken != tok_pclose) {
        std::cerr << "A ')' is expected at the end of a prototype" << std::endl;
        return nullptr;
    }

    m_CurrentToken = m_Lexer->getToken();

    auto proto = std::make_shared<PrototypeAST>(declarationAST, args);

#ifdef VERBOSE_PARSING
    std::cout << "Reached end of prototype parsing" << std::endl;
#endif


    return std::move(proto);
}

std::shared_ptr<FunctionDefinitionAST> Parser::parseDefinition(std::shared_ptr<PrototypeAST> proto) {
    m_CurrentToken = m_Lexer->getToken();
    std::vector<std::shared_ptr<ExprAST>> blocks;

    while (m_CurrentToken != tok_return && m_CurrentToken != tok_bclose) {
        if (m_CurrentToken == tok_type)
            blocks.push_back(std::move(parseDeclaration()));
        else
            blocks.push_back(std::move(parseExpression()));
    }


    if (m_CurrentToken == tok_return) {
        m_CurrentToken = m_Lexer->getToken();
        auto expr = parseExpression();

        if (!expr) {
            std::cout << "Expecting expression after 'return'!" << std::endl;
            return nullptr;
        }

        if (m_CurrentToken != tok_sc) {
            std::cerr << "Expected ';' at the end of the expression line: " << m_Lexer->getLineCount() << std::endl;
            return nullptr;
        }

        m_CurrentToken = m_Lexer->getToken();

        if (m_CurrentToken != tok_bclose) {
            std::cerr << "Expected '}' at the end of the definition" << std::endl;
            return nullptr;
        }

        auto decla = std::make_shared<DeclarationAST>(proto->getType(), proto->getName());

        m_CurrentToken = m_Lexer->getToken();

        return std::make_shared<FunctionDefinitionAST>(proto, std::move(blocks), std::move(expr));
    }

    if (m_CurrentToken != tok_bclose) {
        std::cerr << "Expected '}' at the end of the definition" << std::endl;

        return nullptr;
    }

    m_CurrentToken = m_Lexer->getToken();
    return std::make_shared<FunctionDefinitionAST>(proto, std::move(blocks));
}

std::shared_ptr<ExprAST> Parser::parseTopLevelExpr() {
    if (auto expr = parseExpression()) {

        if (auto callExpr = std::dynamic_pointer_cast<CallFunctionExprAST>(expr))
            return callExpr;

        auto declaration = std::make_shared<DeclarationAST>("float", "__anon_expr");
        auto proto = std::make_shared<PrototypeAST>(std::move(declaration),
                                                    std::vector<std::shared_ptr<DeclarationAST>>());

        auto vec = std::vector<std::shared_ptr<ExprAST>>();

        return std::make_shared<FunctionDefinitionAST>(proto, std::move(vec), std::move(expr));
    }

    return nullptr;
}

std::shared_ptr<ExprAST> Parser::parseIdentifier() {
    std::string identifier = m_Lexer->getIdentifier();

    m_CurrentToken = m_Lexer->getToken();

    if (m_CurrentToken != tok_popen)
        return std::make_shared<VariableExprAST>(identifier);

    m_CurrentToken = m_Lexer->getToken();

    std::vector<std::shared_ptr<ExprAST>> args;

    if (m_CurrentToken != tok_pclose) {
        while (1) {
            if (auto arg = parseExpression()) {
                args.push_back(std::move(arg));
            } else {
                return nullptr;
            }

            if (m_CurrentToken == tok_pclose) {
                m_CurrentToken = m_Lexer->getToken();
                break;
            }

            if (m_CurrentToken != tok_comma) {
                m_CurrentToken = m_Lexer->getToken();
                std::cerr << "Expected ')' or ',' in argument list" << std::endl;
                return nullptr;
            }

            m_CurrentToken = m_Lexer->getToken();
        }

    }

    m_CurrentToken = m_Lexer->getToken();

    return std::make_shared<CallFunctionExprAST>(identifier, std::move(args));
}

std::shared_ptr<IntExprAST> Parser::parseIntExpr() {
    int val = std::stoi(m_Lexer->getValueStr());
    m_CurrentToken = m_Lexer->getToken();
    return std::make_shared<IntExprAST>(val);
}

std::shared_ptr<FloatExprAST> Parser::parseFloatExpr() {
    double val = std::stod(m_Lexer->getValueStr());
    m_CurrentToken = m_Lexer->getToken();
    return std::make_shared<FloatExprAST>(val);
}

std::shared_ptr<ExprAST> Parser::parseParensExpr() {
    m_CurrentToken = m_Lexer->getToken();
    auto expr = parseExpression();
    if (!expr)
        return nullptr;

    if (m_CurrentToken != tok_pclose) {
        std::cerr << "Expected ')'" << std::endl;
    }

    m_CurrentToken = m_Lexer->getToken();

    return expr;
}

std::shared_ptr<ExprAST> Parser::parseExpression() {
    auto LHS = parsePrimaryExpr();

    if (!LHS)
        return nullptr;

    return parseBinaryExpr(0, std::move(LHS));
}

std::shared_ptr<ExprAST> Parser::parsePrimaryExpr() {
    switch (m_CurrentToken) {
        case tok_identifier:
            return parseIdentifier();
        case tok_val_float:
            return parseFloatExpr();
        case tok_val_int:
            return parseIntExpr();
        case tok_popen:
            return parseParensExpr();
        default:
            std::cerr << "Unexpected token instead of expression : " << tokToString(m_CurrentToken) << std::endl;
            m_CurrentToken = m_Lexer->getToken();
            return nullptr;
        
    }
}

std::shared_ptr<ExprAST> Parser::parseBinaryExpr(int exprPrec, std::shared_ptr<ExprAST> LHS) {
    while (true) {
        int tokPrec = getTokenPrecedence(m_CurrentToken);
        if (tokPrec < exprPrec) {
            //m_CurrentToken = m_Lexer->getToken();
            return LHS;
        }

        int binOp = m_CurrentToken;

        m_CurrentToken = m_Lexer->getToken();

        auto RHS = parsePrimaryExpr();

        if (!RHS)
            return nullptr;

        int nextPrec = getTokenPrecedence(m_CurrentToken);

        if (tokPrec < nextPrec) {
            RHS = parseBinaryExpr(tokPrec + 1, std::move(RHS));

            if (!RHS) {
                return nullptr;
            }
        }

        LHS = std::make_shared<BinaryOpExprAST>(binOp, std::move(LHS), std::move(RHS));
    }
}

std::shared_ptr<VariableDefinitionAST>
Parser::parseVariableDefinition(std::shared_ptr<DeclarationAST> declarationAST) {
    m_CurrentToken = m_Lexer->getToken();
    std::shared_ptr<NumberExprAST> value;

    if (m_CurrentToken == tok_val_int) {
        if (declarationAST->getType() == "float") {
            std::cerr << "Expected a float got an int, cast not implemented yet!!" << std::endl;
            return nullptr;
        }
        value = parseIntExpr();
        m_CurrentToken = m_Lexer->getToken();
        return std::make_shared<VariableDefinitionAST>(declarationAST->getType(), declarationAST->getName(), value);
    }

    if (m_CurrentToken == tok_val_float) {
        if (declarationAST->getType() == "int") {
            std::cerr << "Expected an int got a float, cast not implemented yet!!" << std::endl;
            return nullptr;
        }
        value = parseFloatExpr();
        m_CurrentToken = m_Lexer->getToken();
        return std::make_shared<VariableDefinitionAST>(declarationAST->getType(), declarationAST->getName(), value);
    }

    std::cerr << "Unexpected token: " << tokToString(m_CurrentToken) <<
              " when expected " << declarationAST->getType() << std::endl;

    return nullptr;

}

