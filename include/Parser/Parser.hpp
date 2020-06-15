//
// Created by Emilien Lemaire on 21/04/2020.
//

#pragma once
#include <condition_variable>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <deque>
#include <thread>
#include "AST/ExprAST.hpp"
#include "Lexer/Lexer.hpp"
#include "AST/AST.hpp"
#include "utils/token.hpp"

class Parser {
private:
    std::shared_ptr<Lexer> m_Lexer;
    Token m_CurrentToken;

    //std::thread m_IO;
    //std::promise<void> m_StopIOThread;
    //std::condition_variable m_ConditionnalVariable;
    //std::mutex m_Mutex;
    //std::deque<Token> m_Tokens;
    //std::deque<Token> m_ToProcess;

    int m_AnonFuncNum = 0;

    std::map<std::string, std::string> m_NameType;

    CppLogger::CppLogger m_Logger;

public:
    Parser(std::shared_ptr<Lexer> lexer);

    ~Parser();

    Token getNextToken();
    Token waitForToken();

    const int getAnonFuncNum() const { return m_AnonFuncNum; }
    void incrementAnonFuncNum() { m_AnonFuncNum++; }

    void parse();
    std::shared_ptr<ExprAST> parseNext();
    std::shared_ptr<ExprAST> parsePrimaryExpr(const std::string &scope = "");
    std::shared_ptr<ExprAST> parseTopLevelExpr();
    std::shared_ptr<DeclarationAST> parseDeclaration(const std::string &scope = "");
    void parseInclude();
    std::shared_ptr<PrototypeAST> parsePrototype(std::shared_ptr<DeclarationAST> declarationAST);
    std::shared_ptr<VariableDefinitionAST> parseVariableDefinition(std::shared_ptr<DeclarationAST> declarationAST);
    std::shared_ptr<FunctionDefinitionAST> parseDefinition(std::shared_ptr<PrototypeAST> proto);
    std::shared_ptr<ExprAST> parseIdentifier(const std::string &scope = "");
    std::shared_ptr<IntExprAST> parseIntExpr();
    std::shared_ptr<FloatExprAST> parseFloatExpr();
    std::shared_ptr<ExprAST> parseParensExpr(const std::string &scope = "");
    std::shared_ptr<IfExprAST> parseIfExpr(const std::string &scope = "");

    std::shared_ptr<ExprAST> parseExpression(const std::string &scope = "");

    std::shared_ptr<ExprAST> parseBinaryExpr(int exprPrec, std::shared_ptr<ExprAST> LHS, const std::string &scope = "");

    //void stopIOThread() { m_StopIOThread.set_value(); }
};



