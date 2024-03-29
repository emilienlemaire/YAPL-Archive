//
// Created by Emilien Lemaire on 21/04/2020.
//

#pragma once

#include <memory>
#include <string>
#include <vector>

class ExprAST {
private:
    std::string m_Type;
public:
    ExprAST(std::string type)
        : m_Type(type)
    {}

    virtual ~ExprAST() = default;

    const std::string getType() const { return m_Type; }
};

class EOFExprAST : public ExprAST {
public:
    EOFExprAST()
        : ExprAST("void")
    {}
};

class VariableExprAST : public ExprAST {
private:
    std::string m_Identifier;
public:
    VariableExprAST(std::string type, const std::string &mIdentifier)
        : ExprAST(type), m_Identifier(mIdentifier)
    {}

    const std::string &getIdentifier() const { return m_Identifier; }
};

class NumberExprAST : public ExprAST {
protected:
    union num{
        int ival;
        float fval;
    };
public:
    NumberExprAST(std::string type)
        : ExprAST(type)
    {}
    virtual const num &getValue() const = 0;
};

class IntExprAST: public NumberExprAST {
private:
    num m_Value;
public:
    IntExprAST(int value)
        : NumberExprAST("int") 
    {
        m_Value.ival = value;
    }

    virtual const num & getValue() const override {
        return m_Value;
    }
};

class FloatExprAST: public NumberExprAST {
private:
    num m_Value;
public:
    FloatExprAST(double value)
        : NumberExprAST("float")
    {
        m_Value.fval = value;
    }

    virtual const num &getValue() const override {
        return m_Value;
    }
};

class BinaryOpExprAST: public ExprAST {
private:
    char m_Op;
    std::shared_ptr<ExprAST> m_LHS;
    std::shared_ptr<ExprAST> m_RHS;
public:
    BinaryOpExprAST(char op,
                    std::shared_ptr<ExprAST> LHS,
                    std::shared_ptr<ExprAST> RHS)
        :ExprAST(LHS->getType()), m_Op(op), m_LHS(std::move(LHS)), m_RHS(std::move(RHS))
    {}

    const char &getOp() const { return m_Op; }
    const std::shared_ptr<ExprAST> &getLHS() const { return m_LHS; }
    const std::shared_ptr<ExprAST> &getRHS() const { return m_RHS; }
};

class CallFunctionExprAST: public ExprAST {
private:
    std::string m_Callee;
    std::vector<std::shared_ptr<ExprAST>> m_Args;
public:
    CallFunctionExprAST(const std::string &type, const std::string &mCallee,
                        std::vector<std::shared_ptr<ExprAST>> mArgs)
        :ExprAST(type) ,m_Callee(mCallee),  m_Args(std::move(mArgs))
    {}

    const std::string &getCallee() const { return m_Callee; }
    const std::vector<std::shared_ptr<ExprAST>> &getArgs() const { return m_Args; }
};

