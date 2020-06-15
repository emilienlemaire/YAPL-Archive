//
// Created by Emilien Lemaire on 01/05/2020.
//
/* TODO:
 *  - Type checking
 *  - Generate multi-line function
 *  - Handle include
 *  - Control blocks
 *  - Integrate CppLogger
 *  - Function overload
 *
 * */


#include <cstddef>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>

#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>

#include "AST/ExprAST.hpp"
#include "CppLogger2/include/CppLogger.h"
#include "CppLogger2/include/Format.h"
#include "YAPLJIT/YAPLJIT.hpp"
#include "IRGenerator/IRGenerator.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

IRGenerator::IRGenerator(const char * argv)
    :m_Lexer(std::make_shared<Lexer>(argv)), m_Parser(m_Lexer),
    m_Logger(CppLogger::Level::Trace, "Generator", true)
{

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    m_Module = std::make_unique<llvm::Module>("test", m_Context);
    m_Builder = std::make_unique<llvm::IRBuilder<>>(m_Context);
    m_YAPLJIT = std::move(YAPLJIT::Create().get());

    m_Module->setDataLayout(m_YAPLJIT->getDataLayout());

    m_Logger.setFormat({
            CppLogger::FormatAttribute::Name,
            CppLogger::FormatAttribute::Message
            });

    m_Logger.setOStream(std::cerr);
}

void IRGenerator::generate() {

    if (!m_Lexer->hasFile()) {
        std::cerr << "(YAPL)>>>";
    }
    std::shared_ptr<ExprAST> expr = m_Parser.parseNext();

    while (!(std::dynamic_pointer_cast<EOFExprAST>(expr))) {

        if (auto parsedExpr = std::dynamic_pointer_cast<DeclarationAST>(expr)) {
            m_Logger.printInfo("Read declaration:");
            if (auto *declaration = generateDeclaration(parsedExpr)) {
                declaration->print(llvm::errs(), nullptr, false, true);
                auto H = m_YAPLJIT->addModule(std::move(m_Module));
                initializeModule();
            }
        } else if (expr) {
            m_Logger.printInfo("Read top level:");
            auto *topLevel = generateTopLevel(expr);
            topLevel->print(llvm::errs(), true);
            auto type = topLevel->getType()->getPointerElementType();

            bool isFloat = false;
            if (auto fType = static_cast<llvm::FunctionType*>(type)) {
                isFloat = fType->getReturnType()->isDoubleTy();
            }

            m_Logger.printTrace("");
            auto H = m_YAPLJIT->addModule(std::move(m_Module));
            initializeModule();

            auto exprSymbol = m_YAPLJIT->lookup(
                    ("__anon_expr" + llvm::Twine(m_Parser.getAnonFuncNum())).str())
                .get();

            m_Parser.incrementAnonFuncNum();

            assert(exprSymbol && "Function not found");
            if (isFloat) {
                double (*FP)() = (double(*)())(intptr_t)exprSymbol.getAddress();

                m_Logger.printInfo("Evaluated to {}", FP());
            } else {
                int (*FP)() = (int(*)())(intptr_t)exprSymbol.getAddress();
                m_Logger.printInfo("Evaluated to {}", FP());
            }
        }

        if (!m_Lexer->hasFile()) {
            std::cerr << "(YAPL)>>>";
        }
        expr = m_Parser.parseNext();
    }

    std::cout << "Exiting the loop" << std::endl;
}

/******************** ExprAST ********************************************/

llvm::Value *IRGenerator::generateTopLevel(std::shared_ptr<ExprAST> parsedExpression) {
    if (auto parsedNumber = std::dynamic_pointer_cast<NumberExprAST>(parsedExpression)) {
        if (std::dynamic_pointer_cast<IntExprAST>(parsedNumber)) {
            int intVal = parsedNumber->getValue().ival;
            return llvm::ConstantInt::get(m_Context, llvm::APInt(32, intVal));
        }
        if (std::dynamic_pointer_cast<FloatExprAST>(parsedNumber)) {
            double floatVal = parsedNumber->getValue().fval;
            return llvm::ConstantFP::get(m_Context, llvm::APFloat(floatVal));
        }
        m_Logger.printError("Number expression not recognized");
        return nullptr;
    }

    if (auto anonExpr = std::dynamic_pointer_cast<AnonExprAst>(parsedExpression)) {
        auto functionBlocks = std::vector<std::shared_ptr<ExprAST>>();
        auto anonFuncExpr = std::make_shared<FunctionDefinitionAST>(anonExpr->getProto(),
                std::move(functionBlocks), anonExpr->getExpr());

        auto function = generateFunctionDefinition(std::move(anonFuncExpr));

        return function;
    }

    if (auto parsedVariable = std::dynamic_pointer_cast<VariableExprAST>(parsedExpression)) {
        llvm::Value* val = m_NamedValues[parsedVariable->getIdentifier()];
        if(!val) {
            m_Logger.printError("Unknown variable: {}", parsedVariable->getIdentifier());
            return nullptr;
        }

        return val;
    }

    if (auto parsedBin = std::dynamic_pointer_cast<BinaryOpExprAST>(parsedExpression)) {
        return generateBinary(std::move(parsedBin));
    }

    if (auto parsedCall = std::dynamic_pointer_cast<CallFunctionExprAST>(parsedExpression)) {
        auto call = generateFunctionCall(std::move(parsedCall));
        return call;
    }

    if (auto parsedIf = std::dynamic_pointer_cast<IfExprAST>(parsedExpression)) {
        auto ifValue = generateIfStatement(std::move(parsedIf));
        return ifValue;
    }

    return nullptr;
}

llvm::Value *IRGenerator::generateBinary(std::shared_ptr<BinaryOpExprAST> parsedBinaryOpExpr) {
    char op = parsedBinaryOpExpr->getOp();

    auto LHS = parsedBinaryOpExpr->getLHS();
    auto RHS = parsedBinaryOpExpr->getRHS();

    llvm::Value *L = generateTopLevel(std::move(LHS));
    llvm::Value *R = generateTopLevel(std::move(RHS));

    if(!L || !R)
        return nullptr;

    if (L->getType()->getTypeID() != R->getType()->getTypeID()) {
        if (R->getType()->getTypeID() == llvm::Type::getInt32Ty(m_Context)->getTypeID()) {
            R = m_Builder->CreateSIToFP(R, L->getType(), "casttmp");
        } else {
            R = m_Builder->CreateFPToSI(R, L->getType(), "casttmp");
        }
    }


    if (parsedBinaryOpExpr->getType() == "float" || parsedBinaryOpExpr->getType() == "double") {
        switch (op) {
            case '+':
                return m_Builder->CreateFAdd(L, R, "addtmp");
            case '-':
                return m_Builder->CreateFSub(L, R, "subtmp");
            case '*':
                return m_Builder->CreateFMul(L, R, "multmp");
            case '<':
                L = m_Builder->CreateFCmpULT(L, R, "cmptmp");
                return m_Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(m_Context), "booltmp");
            default:
                std::cerr << "Invalid binary operator" << std::endl;
                return nullptr;
        }
    } else {
        switch (op) {
            case '+':
                return m_Builder->CreateAdd(L, R, "addtmp");
            case '-':
                return m_Builder->CreateSub(L, R, "subtmp");
            case '*':
                return m_Builder->CreateMul(L, R, "multmp");
            case '<':
                return m_Builder->CreateICmpULT(L, R, "cmptmp");
            default:
                std::cerr << "Invalid binary operator" << std::endl;
                return nullptr;
        }
    }

}

llvm::Value *IRGenerator::generateFunctionCall(
        std::shared_ptr<CallFunctionExprAST> parsedFunctionCall) {
    llvm::Function *calleeFunction = getFunction(parsedFunctionCall->getCallee());
    if (!calleeFunction) {
        m_Logger.printError("Unknown function: {}", parsedFunctionCall->getCallee());
        return nullptr;
    }

    auto args = parsedFunctionCall->getArgs();
    std::vector<llvm::Value *> callArgs;
    if (calleeFunction->arg_size() != args.size()) {
        return nullptr;
    }

    for (const auto &arg : args) {
        llvm::Value *argVal = generateTopLevel(std::move(arg));
        callArgs.push_back(argVal);

        if(!callArgs.back()) {
            return nullptr;
        }
    }

    for (unsigned i = 0; i < callArgs.size(); i++) {
        if (callArgs[i]->getType()->getTypeID() !=
                calleeFunction->getArg(i)->getType()->getTypeID()) {
            callArgs[i] =
                (callArgs[i]->getType()->getTypeID() ==
                 llvm::Type::getInt32Ty(m_Context)->getTypeID()) ?
                callArgs[i] =
                    m_Builder->CreateSIToFP(callArgs[i], calleeFunction->getArg(i)->getType(), "casttmp") :
                callArgs[i] =
                    m_Builder->CreateFPToSI(callArgs[i], calleeFunction->getArg(i)->getType(), "casttmp");
        }
    }

    return m_Builder->CreateCall(calleeFunction, callArgs, "calltmp");
}

llvm::Value *IRGenerator::generateIfStatement(std::shared_ptr<IfExprAST> parsedIfExpr) {
    llvm::Value *conditionValue = generateTopLevel(parsedIfExpr->getCondition());

    if (!conditionValue) {
        return nullptr;
    }

    if (conditionValue->getType()->getTypeID() == llvm::Type::getInt32Ty(m_Context)->getTypeID()) {
        conditionValue = m_Builder->CreateSIToFP(conditionValue, llvm::Type::getDoubleTy(m_Context));
    }
    conditionValue = m_Builder->CreateFCmpONE(
            conditionValue,
            llvm::ConstantFP::get(m_Context, llvm::APFloat(0.0)),
            "ifcond");

    llvm::Function *function = m_Builder->GetInsertBlock()->getParent();

    llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(m_Context, "then", function);
    llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(m_Context, "else");
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(m_Context, "ifcont");

    m_Builder->CreateCondBr(conditionValue, thenBB, elseBB);

    m_Builder->SetInsertPoint(thenBB);

    llvm::Value *thenValue = generateTopLevel(parsedIfExpr->getThen());

    if (!thenValue) {
        return nullptr;
    }

    m_Builder->CreateBr(mergeBB);

    thenBB = m_Builder->GetInsertBlock();

    function->getBasicBlockList().push_back(elseBB);
    m_Builder->SetInsertPoint(elseBB);

    llvm::Value *elseValue = generateTopLevel(parsedIfExpr->getElse());

    if (!elseValue) {
        return nullptr;
    }


    if (elseValue->getType()->getTypeID() != thenValue->getType()->getTypeID()) {
        if (thenValue->getType()->getTypeID() == llvm::Type::getInt32Ty(m_Context)->getTypeID()) {
            elseValue = m_Builder->CreateFPToSI(elseValue, thenValue->getType(), "casttmp");
        } else {
            elseValue = m_Builder->CreateSIToFP(elseValue, thenValue->getType(), "casttmp");
        }
    }

    m_Builder->CreateBr(mergeBB);

    elseBB = m_Builder->GetInsertBlock();

    function->getBasicBlockList().push_back(mergeBB);
    m_Builder->SetInsertPoint(mergeBB);

    llvm::PHINode *phiNode = m_Builder->CreatePHI(
            thenValue->getType(),
            2,
            "iftmp");

    phiNode->addIncoming(thenValue, thenBB);
    phiNode->addIncoming(elseValue, elseBB);

    return phiNode;
}

/******************** DeclarationAST ********************************************/


llvm::Function *IRGenerator::generateDeclaration(std::shared_ptr<DeclarationAST> parsedDeclaration) {

    if (auto parsedDefinition =
            std::dynamic_pointer_cast<FunctionDefinitionAST>(parsedDeclaration)) {
        return generateFunctionDefinition(std::move(parsedDefinition));
    }

    if (auto parsedProto =
            std::dynamic_pointer_cast<PrototypeAST>(parsedDeclaration)) {
        return generatePrototype(std::move(parsedProto));
    }

    return nullptr;
}

llvm::Function *IRGenerator::generatePrototype(std::shared_ptr<PrototypeAST> parsedPrototype) {
    auto params = parsedPrototype->getParams();
    std::vector<llvm::Type *> paramTypes;

    for (const auto &param : params) {
        if (param->getType() == "int") {
            paramTypes.push_back(llvm::Type::getInt32Ty(m_Context));
        } else if (param->getType() == "float" || param->getType() == "double") {
            paramTypes.push_back(llvm::Type::getDoubleTy(m_Context));
        } else {
            m_Logger.printError("Unknown type: {}, ignoring parameter: ", param->getType());
        }
    }

    llvm::FunctionType * functionType;

    if (parsedPrototype->getType() == "int") {
        functionType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(m_Context),
                paramTypes,
                false);

    } else if (parsedPrototype->getType() == "float" || parsedPrototype->getType() == "double") {
        functionType = llvm::FunctionType::get(
                llvm::Type::getDoubleTy(m_Context),
                paramTypes,
                false);

    } else {
        m_Logger.printError("Unknown type: {}, ignoring function \"{}\" ",
                parsedPrototype->getType(),
                parsedPrototype->getName());
        return nullptr;
    }

    llvm::Function *function = llvm::Function::Create(
            functionType,
            llvm::Function::ExternalLinkage,
            parsedPrototype->getName(),
            m_Module.get());

    unsigned idx = 0;
    for (auto &arg : function->args()) {
        arg.setName(params[idx++]->getName());
    }

    return function;
}

llvm::Function *IRGenerator::generateFunctionDefinition(std::shared_ptr<FunctionDefinitionAST> parsedFunctionDefinition) {
    std::string name = parsedFunctionDefinition->getName();
    auto p = parsedFunctionDefinition->getPrototype();
    m_FunctionDefs[p->getName()] = std::move(p);

    llvm::Function *function = getFunction(name);

    if (!function) {
        return nullptr;
    }

    if (!function->empty()) {
        m_Logger.printError("Function cannot be redefined: {}", name);
    }

    llvm::BasicBlock *basicBlock = llvm::BasicBlock::Create(m_Context, "entry", function);
    m_Builder->SetInsertPoint(basicBlock);

    m_NamedValues.clear();

    for (auto &arg : function->args()) {
        m_NamedValues[arg.getName().str()] = &arg;
    }
    auto returnExpr = parsedFunctionDefinition->getReturnExpr();
    if (llvm::Value *retValue = generateTopLevel(returnExpr)) {
        if (retValue->getType() != function->getReturnType()) {
            if (retValue->getValueID() == llvm::Type::getInt32Ty(m_Context)->getTypeID()) {
                retValue = m_Builder
                    ->CreateSIToFP(retValue, llvm::Type::getDoubleTy(m_Context), "casttmp");
            } else {
                retValue = m_Builder
                    ->CreateFPToSI(retValue, llvm::Type::getInt32Ty(m_Context), "casttmp");
            }
        }

        m_Builder->CreateRet(retValue);

        llvm::verifyFunction(*function);

        return function;
    }

    function->eraseFromParent();

    return function;
}

void IRGenerator::initializeModule() {
    m_Module = std::make_unique<llvm::Module>("JIT", m_Context);
    m_Module->setDataLayout(m_YAPLJIT->getDataLayout());
}

llvm::Function *IRGenerator::getFunction(const std::string &name) {
    if (auto *func = m_Module->getFunction(name)) {
        return func;
    }

    auto funcDef = m_FunctionDefs.find(name);

    if (funcDef != m_FunctionDefs.end()) {
        return generatePrototype(funcDef->second);
    }

    m_Logger.printError("Function not found: {}", name);

    return nullptr;
}

std::unique_ptr<llvm::Module> IRGenerator::generateAndTakeOwnership(FunctionDefinitionAST funcAST,
        const std::string &suffix) {
    if (auto func = generateFunctionDefinition(std::make_shared<FunctionDefinitionAST>(funcAST))) {
        func->setName(func->getName() + suffix);

        auto module = std::move(m_Module);

        initializeModule();

        return module;
    } else {
        m_Logger.printFatalError("Couln't compile function: '{}'. Aborting.", func->getName().str());
        return nullptr;
    }
}

IRGenerator::~IRGenerator() {
    //m_Parser.stopIOThread();
}
