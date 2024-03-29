//
// Created by Emilien Lemaire on 21/04/2020.
//

#pragma once

#include <memory>
#include <string>

#include "ExprAST.hpp"

class DeclarationAST: public ExprAST {
protected:
    std::string m_Name;
public:
    DeclarationAST(const std::string &mType, const std::string &mName)
        : ExprAST(mType), m_Name(mName)
    {}

    DeclarationAST()
        : ExprAST("int"), m_Name("")
    {}

    const std::string &getName() const {
        return m_Name;
    }
};

class PrototypeAST : public DeclarationAST {
protected:
    std::vector<std::shared_ptr<DeclarationAST>> m_Params;
public:
    PrototypeAST(std::shared_ptr<DeclarationAST> declaration,
                 std::vector<std::shared_ptr<DeclarationAST>> mParams)
            : DeclarationAST(*declaration.get()),
              m_Params(std::move(mParams))
    {}

    const std::vector<std::shared_ptr<DeclarationAST>> &getParams() const {
        return m_Params;
    }
};

class FunctionDefinitionAST: public DeclarationAST {
private:
    std::vector<std::shared_ptr<ExprAST>> m_Blocks;
    std::shared_ptr<ExprAST> m_ReturnBlock;
    std::shared_ptr<PrototypeAST> m_Prototype;
public:
    FunctionDefinitionAST(std::shared_ptr<PrototypeAST> proto,
                          std::vector<std::shared_ptr<ExprAST>> blocks,
                          std::shared_ptr<ExprAST> returnBlock = nullptr)
        :
            DeclarationAST(proto->getType(), proto->getName()), m_Blocks(std::move(blocks)),
            m_ReturnBlock(std::move(returnBlock)), m_Prototype(std::move(proto))
    {}

    const std::shared_ptr<PrototypeAST> &getPrototype() const { return m_Prototype; }
    const std::shared_ptr<ExprAST> &getReturnExpr() const { return m_ReturnBlock; }
};

class VariableDefinitionAST : public DeclarationAST {
private:
    std::shared_ptr<NumberExprAST> m_Value;
public:
    VariableDefinitionAST(const std::string &mType, const std::string &mName,
                          std::shared_ptr<NumberExprAST> mValue)
    : DeclarationAST(mType, mName), m_Value(std::move(mValue))
    {}
};

class AnonExprAst: public ExprAST {
private:
    std::shared_ptr<ExprAST> m_Expr;
    std::shared_ptr<PrototypeAST> m_Proto;
public:
    AnonExprAst(std::shared_ptr<ExprAST> expr, std::shared_ptr<PrototypeAST> proto)
        :ExprAST(proto->getType()), m_Expr(expr), m_Proto(proto)
    {}

    std::shared_ptr<ExprAST> getExpr() { return m_Expr; }
    std::shared_ptr<PrototypeAST> getProto() { return m_Proto; }
};
