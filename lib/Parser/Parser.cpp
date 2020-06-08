//
// Created by Emilien Lemaire on 21/04/2020.
//

#include <chrono>
#include <climits>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <thread>

#include "AST/DeclarationAST.hpp"
#include "AST/ExprAST.hpp"
#include "Parser/Parser.hpp"
#include "helper/helper.hpp"
#include "utils/token.hpp"
#include "llvm/ADT/Twine.h"

Parser::Parser(std::shared_ptr<Lexer> lexer)
        : m_Lexer(std::move(lexer))
{

    std::future<void> stopIOFuture = m_StopIOThread.get_future();

    m_IO = std::thread([&](std::future<void> t_StopFuture){
        Token tmp;
        while(t_StopFuture.wait_for(std::chrono::seconds(0)) == std::future_status::timeout) {
            tmp = m_Lexer->getToken();
            std::lock_guard lock{m_Mutex};
            m_Tokens.push_back(tmp);
            m_ConditionnalVariable.notify_one();
        }

    }, std::move(stopIOFuture));

    m_CurrentToken = getNextToken();
}

Token Parser::getNextToken(){
    {
        std::unique_lock lock{m_Mutex};

        if (m_ConditionnalVariable.wait_for(lock, std::chrono::seconds(0),
                    [&]{ return !m_Tokens.empty(); })) {
            for (auto tok : m_Tokens) {
                m_ToProcess.push_back(tok);
            }
            m_Tokens.clear();
        }
    }

    if (!m_ToProcess.empty()) {
        Token nextTok = m_ToProcess[0];
        m_ToProcess.pop_front();
        return nextTok;
    }

    return Token{ INT_MIN };
}

Token Parser::waitForToken() {

    Token tok = getNextToken();
    while (tok.token == INT_MIN) {
        tok = getNextToken();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return tok;
}

void Parser::parse() {

    std::shared_ptr<ExprAST> parsedExpr = nullptr;

    while (m_CurrentToken.token != tok_eof ||
            (!m_Lexer->hasFile() && true)) {

        std::cerr << "(yapl)>>>";

        m_CurrentToken = waitForToken();

        switch (m_CurrentToken.token) {
            case tok_type:
                parseDeclaration();
                break;
            case tok_include:
                parseInclude();
                break;
            default:
                parseTopLevelExpr();
                break;
        }
    }
}

std::shared_ptr<ExprAST> Parser::parseNext() {

    m_CurrentToken = waitForToken();

    switch ( m_CurrentToken.token ) {
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

std::shared_ptr<DeclarationAST> Parser::parseDeclaration(const std::string &scope) {
    std::string dType = m_CurrentToken.identifier;
    std::string dName;

    m_CurrentToken = waitForToken();

    if (m_CurrentToken.token != tok_identifier) {
        std::cerr << "The type must be followed by an identifier!" << std::endl;
        return nullptr;
    }

    dName = m_CurrentToken.identifier;

    m_CurrentToken = waitForToken();

    if (m_CurrentToken.token == tok_popen) {

        auto declaration = std::make_shared<DeclarationAST>(dType, dName);
        auto proto = parsePrototype(std::move(declaration));

        if (m_CurrentToken.token == tok_sc) {
            return proto;
        }

        m_CurrentToken = waitForToken();

        if (m_CurrentToken.token == tok_bopen) {
            return parseDefinition(proto);
        }

        if (m_CurrentToken.token != tok_sc) {
            std::cerr << "Expected function body or ';' after prototype" << std::endl;
            return nullptr;
        }

        return proto;
    }

    if (m_CurrentToken.token == tok_eq) {
        auto declaration = std::make_shared<DeclarationAST>(dType, dName);
        std::string variableName = (scope.length() > 0) ?
            scope + "::" + declaration->getName() :
            declaration->getName();

        m_NameType[variableName] = declaration->getType();

        return parseVariableDefinition(declaration);
    }

    if (m_CurrentToken.token != tok_sc) {
        std::cerr << "Expected ';' after variable declaration!" << std::endl;
        return nullptr;
    }

    std::string variableName = (scope.length() > 0) ?
        scope + "::" + dName :
        dName;

    m_NameType[variableName] = dType;

    return std::make_shared<DeclarationAST>(dType, dName);
}

void Parser::parseInclude() {
    m_CurrentToken = waitForToken();
}

std::shared_ptr<PrototypeAST> Parser::parsePrototype(std::shared_ptr<DeclarationAST> declarationAST) {
    std::vector<std::shared_ptr<DeclarationAST>> args;

    m_CurrentToken = waitForToken();

    if (m_CurrentToken.token != tok_type) {
        if (m_CurrentToken.token == tok_pclose) {
            return std::make_shared<PrototypeAST>(std::move(declarationAST), std::move(args));
        } else {
            std::cerr << "Parameters must be typed!" << std::endl;
            return nullptr;
        }
    }

    std::string argType = m_CurrentToken.identifier;
    std::string argName;
    m_CurrentToken = waitForToken();

    if (m_CurrentToken.token != tok_identifier) {
        std::cerr << "Parameters must be named!" << std::endl;
        return nullptr;
    }

    argName = m_CurrentToken.identifier;
    auto fstArg = std::make_shared<DeclarationAST>(argType, argName);
    args.push_back(std::move(fstArg));

    auto loopArg = std::make_shared<DeclarationAST>(argType, argName);

    std::string argScopedName = declarationAST->getName() + "::" + argName;

    m_NameType[argScopedName] = argType;
    m_CurrentToken = waitForToken();

    while (m_CurrentToken.token == tok_comma) {
        m_CurrentToken = waitForToken();

        if (m_CurrentToken.token != tok_type) {
            std::cerr << "Parameters must be typed!" << std::endl;
            return nullptr;
        }

        argType = m_CurrentToken.identifier;
        m_CurrentToken = waitForToken();

        if (m_CurrentToken.token != tok_identifier) {
            std::cerr << "Parameters must be named!" << std::endl;
            return nullptr;
        }

        argName = m_CurrentToken.identifier;

        for (const auto &arg : args) {
            if (arg->getName() == argName)
                return nullptr;
        }

        auto loopArg = std::make_shared<DeclarationAST>(argType, argName);
        std::string argScopedName = declarationAST->getName() + "::" + argName;

        m_NameType[argScopedName] = argType;
        args.push_back(std::move(loopArg));

        m_CurrentToken = waitForToken();
    }

    if (m_CurrentToken.token != tok_pclose) {
        std::cerr << "A ')' is expected at the end of a prototype" << std::endl;
        return nullptr;
    }


    auto proto = std::make_shared<PrototypeAST>(declarationAST, args);

    m_NameType[proto->getName()] = proto->getType();
    return std::move(proto);
}

std::shared_ptr<FunctionDefinitionAST> Parser::parseDefinition(std::shared_ptr<PrototypeAST> proto) {
    m_CurrentToken = waitForToken();

    std::vector<std::shared_ptr<ExprAST>> blocks;

    while (m_CurrentToken.token != tok_return && m_CurrentToken.token != tok_bclose) {

        if (m_CurrentToken.token == tok_return || m_CurrentToken.token == tok_bclose)
            break;

        if (m_CurrentToken.token == tok_type)
            blocks.push_back(std::move(parseDeclaration(proto->getName())));
        else
            blocks.push_back(std::move(parseExpression(proto->getName())));
    }


    if (m_CurrentToken.token == tok_return) {
        m_CurrentToken = waitForToken();
        auto expr = parseExpression(proto->getName());

        if (!expr) {
            std::cerr << "Expecting expression after 'return'!" << std::endl;
            return nullptr;
        }

        if (m_CurrentToken.token != tok_sc) {
            std::cerr << "Expected ';' at the end of the expression line: "
                << m_Lexer->getLineCount() << std::endl;
            return nullptr;
        }

        m_CurrentToken = waitForToken();

        if (m_CurrentToken.token != tok_bclose) {
            std::cerr << "Expected '}' at the end of the definition got: "
                << tokToString(m_CurrentToken.token) << std::endl;
            return nullptr;
        }

        auto decla = std::make_shared<DeclarationAST>(proto->getType(), proto->getName());

        return std::make_shared<FunctionDefinitionAST>(proto, std::move(blocks), std::move(expr));
    }

    if (m_CurrentToken.token != tok_bclose) {

        std::cerr << "Expected '}' at the end of the definition" << std::endl;

        return nullptr;
    }

    return std::make_shared<FunctionDefinitionAST>(proto, std::move(blocks));
}

std::shared_ptr<ExprAST> Parser::parseTopLevelExpr() {
    if (auto expr = parseExpression()) {

        auto declaration = std::make_shared<DeclarationAST>(expr->getType(), ("__anon_expr" + llvm::Twine(m_AnonFuncNum)).str());
        auto proto = std::make_shared<PrototypeAST>(std::move(declaration),
                                                    std::vector<std::shared_ptr<DeclarationAST>>());

        return std::make_shared<AnonExprAst>(std::move(expr), std::move(proto));
    }

    return nullptr;
}

std::shared_ptr<ExprAST> Parser::parseIdentifier(const std::string &scope) {
    std::string identifier = m_CurrentToken.identifier;

    m_CurrentToken = waitForToken();

    std::string scopedId = (scope.length() > 0) ?
        scope + "::" + identifier :
        identifier;

    auto it = m_NameType.find(scopedId);

    if (it == m_NameType.end()) {
        std::cerr << "Variable or function called but not declared: " << scopedId << std::endl;
        return nullptr;
    }

    std::string type = it->second;

    if (m_CurrentToken.token != tok_popen){

        return std::make_shared<VariableExprAST>(type, identifier);
    }

    m_CurrentToken = waitForToken();

    std::vector<std::shared_ptr<ExprAST>> args;

    if (m_CurrentToken.token != tok_pclose) {
        while (1) {
            if (auto arg = parseExpression(scope)) {
                args.push_back(std::move(arg));
            } else {
                return nullptr;
            }

            if (m_CurrentToken.token == tok_pclose) {
                m_CurrentToken = waitForToken();
                break;
            }

            if (m_CurrentToken.token != tok_comma) {
                m_CurrentToken = waitForToken();
                std::cerr << "Expected ')' or ',' in argument list" << std::endl;
                return nullptr;
            }

            m_CurrentToken = waitForToken();
        }

    }

    return std::make_shared<CallFunctionExprAST>(type, identifier, std::move(args));
}

std::shared_ptr<IntExprAST> Parser::parseIntExpr() {
    int val = std::stoi(m_CurrentToken.valueStr);
    m_CurrentToken = waitForToken();
    return std::make_shared<IntExprAST>(val);
}

std::shared_ptr<FloatExprAST> Parser::parseFloatExpr() {
    double val = std::stod(m_CurrentToken.valueStr);
    m_CurrentToken = waitForToken();
    return std::make_shared<FloatExprAST>(val);
}

std::shared_ptr<ExprAST> Parser::parseParensExpr(const std::string &scope) {
    m_CurrentToken = waitForToken();
    auto expr = parseExpression(scope);
    if (!expr)
        return nullptr;

    if (m_CurrentToken.token != tok_pclose) {
        std::cerr << "Expected ')'" << std::endl;
    }

    m_CurrentToken = waitForToken();

    return expr;
}

std::shared_ptr<ExprAST> Parser::parseExpression(const std::string &scope) {
    auto LHS = parsePrimaryExpr(scope);

    if (!LHS)
        return nullptr;

    return parseBinaryExpr(0, std::move(LHS), scope);
}

std::shared_ptr<ExprAST> Parser::parsePrimaryExpr(const std::string &scope) {
    while (m_CurrentToken.token == INT_MIN) {
        m_CurrentToken = waitForToken();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    switch (m_CurrentToken.token) {
        case tok_identifier:
            return parseIdentifier(scope);
        case tok_val_float:
            return parseFloatExpr();
        case tok_val_int:
            return parseIntExpr();
        case tok_popen:
            return parseParensExpr(scope);
        default:
            std::cerr << "Unexpected token instead of expression : " << tokToString(m_CurrentToken.token) << std::endl;
            m_CurrentToken = waitForToken();
            return nullptr;
    }
}

std::shared_ptr<ExprAST> Parser::parseBinaryExpr(int exprPrec, std::shared_ptr<ExprAST> LHS, const std::string &scope) {
    while (true) {
        int tokPrec = getTokenPrecedence(m_CurrentToken.token);
        if (tokPrec < exprPrec) {
            return LHS;
        }

        int binOp = m_CurrentToken.token;

        m_CurrentToken = waitForToken();

        auto RHS = parsePrimaryExpr(scope);

        if (!RHS)
            return nullptr;

        int nextPrec = getTokenPrecedence(m_CurrentToken.token);

        if (tokPrec < nextPrec) {
            RHS = parseBinaryExpr(tokPrec + 1, std::move(RHS), scope);

            if (!RHS) {
                return nullptr;
            }
        }

        LHS = std::make_shared<BinaryOpExprAST>(binOp, std::move(LHS), std::move(RHS));
    }
}

std::shared_ptr<VariableDefinitionAST>
Parser::parseVariableDefinition(std::shared_ptr<DeclarationAST> declarationAST) {
    m_CurrentToken = waitForToken();
    std::shared_ptr<NumberExprAST> value;

    if (m_CurrentToken.token == tok_val_int) {
        if (declarationAST->getType() == "float" || declarationAST->getType() == "double") {
            std::cerr << "Expected a float got an int, cast not implemented yet!!" << std::endl;
            return nullptr;
        }
        value = parseIntExpr();
        return std::make_shared<VariableDefinitionAST>(declarationAST->getType(), declarationAST->getName(), value);
    }

    if (m_CurrentToken.token == tok_val_float) {
        if (declarationAST->getType() == "int") {
            std::cerr << "Expected an int got a float, cast not implemented yet!!" << std::endl;
            return nullptr;
        }
        value = parseFloatExpr();
        return std::make_shared<VariableDefinitionAST>(declarationAST->getType(), declarationAST->getName(), value);
    }

    std::cerr << "Unexpected token: " << tokToString(m_CurrentToken.token) <<
              " when expected " << declarationAST->getType() << std::endl;

    return nullptr;

}

Parser::~Parser() {

    m_StopIOThread.set_value();

    if (m_IO.joinable()) {
        m_IO.join();
    }
}
