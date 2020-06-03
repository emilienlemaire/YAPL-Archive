//
// Created by Emilien Lemaire on 21/04/2020.
//

#pragma once
#include <condition_variable>
#include <memory>
#include <mutex>
#include <deque>
#include <thread>
#include "Lexer/Lexer.hpp"
#include "AST/AST.hpp"
#include "utils/token.h"

class Parser {
private:
    std::shared_ptr<Lexer> m_Lexer;
    int m_CurrentToken;

    std::thread m_IO;
    std::condition_variable m_ConditionnalVariable;
    std::mutex m_Mutex;
    std::deque<int> m_Tokens;
    std::deque<int> m_ToProcess;

public:
    Parser(std::shared_ptr<Lexer> lexer);

    ~Parser() = default;

    int getNextToken();

    void parse();
    std::shared_ptr<ExprAST> parseNext();
    std::shared_ptr<ExprAST> parsePrimaryExpr();
    std::shared_ptr<ExprAST> parseTopLevelExpr();
    std::shared_ptr<DeclarationAST> parseDeclaration();
    void parseInclude();
    std::shared_ptr<PrototypeAST> parsePrototype(std::shared_ptr<DeclarationAST> declarationAST);
    std::shared_ptr<VariableDefinitionAST> parseVariableDefinition(std::shared_ptr<DeclarationAST> declarationAST);
    std::shared_ptr<FunctionDefinitionAST> parseDefinition(std::shared_ptr<PrototypeAST> proto);
    std::shared_ptr<ExprAST> parseIdentifier();
    std::shared_ptr<IntExprAST> parseIntExpr();
    std::shared_ptr<FloatExprAST> parseFloatExpr();
    std::shared_ptr<ExprAST> parseParensExpr();

    std::shared_ptr<ExprAST> parseExpression();

    std::shared_ptr<ExprAST> parseBinaryExpr(int exprPrec, std::shared_ptr<ExprAST> LHS);
};



