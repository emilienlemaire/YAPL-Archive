//
// Created by Emilien Lemaire on 01/05/2020.
//

#pragma once
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <memory>

#include "AST/DeclarationAST.hpp"
#include "Parser/Parser.hpp"
#include "YAPLJIT/YAPLJIT.hpp"

class IRGenerator {
private:
    llvm::LLVMContext m_Context;
    std::unique_ptr<llvm::IRBuilder<>> m_Builder;
    std::unique_ptr<llvm::Module> m_Module;

    std::unique_ptr<YAPLJIT> m_YAPLJIT;
    std::shared_ptr<Lexer> m_Lexer;
    Parser m_Parser;

    std::map<std::string, llvm::Value *> m_NamedValues;
    std::map<std::string, std::shared_ptr<PrototypeAST>> m_FunctionDefs;

public:
    IRGenerator(const char *argv);

    ~IRGenerator() = default;

    void generate();

    llvm::Value *generateTopLevel(std::shared_ptr<ExprAST> parsedExpression);
    llvm::Value *generateBinary(std::shared_ptr<BinaryOpExprAST> parsedBinaryOpExpr);
    llvm::Value *generateFunctionCall(std::shared_ptr<CallFunctionExprAST> parsedFunctionCall);

    llvm::Function *generateDeclaration(std::shared_ptr<DeclarationAST> parsedDeclaration);
    llvm::Function *generatePrototype(std::shared_ptr<PrototypeAST> parsedPrototype);
    llvm::Function *generateFunctionDefinition(std::shared_ptr<FunctionDefinitionAST> parsedFunctionDefinition);
    llvm::Function *getFunction(const std::string &name);

    std::unique_ptr<llvm::Module> getModule() { return std::move(m_Module); }
    std::unique_ptr<llvm::Module> generateAndTakeOwnership(FunctionDefinitionAST funcAST, const std::string &suffix);

    void reloadModuleAndPassManger();
};



