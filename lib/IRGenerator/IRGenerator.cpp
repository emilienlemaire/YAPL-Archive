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


#include "IRGenerator/IRGenerator.hpp"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>

#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

IRGenerator::IRGenerator(const char * argv)
    :m_Lexer(std::make_shared<Lexer>(argv)), m_Parser(m_Lexer)
{

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    m_Module = std::make_unique<llvm::Module>("test", m_Context);
    m_Builder = std::make_unique<llvm::IRBuilder<>>(m_Context);
    m_YAPLJIT = std::move(YAPLJIT::Create().get());

    m_Module->setDataLayout(m_YAPLJIT->getDataLayout());

    m_PassManager = std::make_unique<PassManager>(m_Module.get());
}

void IRGenerator::generate() {

    if (!m_Lexer->hasFile()) {
        std::cerr << "(YAPL)>>>";
    }
    std::shared_ptr<ExprAST> expr = m_Parser.parseNext();

    while (!(std::dynamic_pointer_cast<EOFExprAST>(expr))) {

        if (auto parsedExpr = std::dynamic_pointer_cast<DeclarationAST>(expr)) {
            fprintf(stderr, "Read declaration:\n");
            if (auto *declaration = generateDeclaration(std::move(parsedExpr))) {
                declaration->print(llvm::errs());
                m_YAPLJIT->addModule(std::move(m_Module));
                reloadModuleAndPassManger();
            }
        } else if (expr) {
            std::cerr << "Read top level:\n";
            auto *topLevel = generateTopLevel(std::move(expr));
            topLevel->print(llvm::errs());
            auto type = topLevel->getType()->getPointerElementType();

            bool isFloat = false;
            if (auto fType = static_cast<llvm::FunctionType*>(type)) {
                isFloat = fType->getReturnType()->isDoubleTy();
            }

            fprintf(stderr, "\n");
            auto H = m_YAPLJIT->addModule(std::move(m_Module));
            reloadModuleAndPassManger();

            auto exprSymbol = m_YAPLJIT->lookup(std::to_string(m_Parser.getAnonFuncNum())).get();

            assert(exprSymbol && "Function not found");
            if (isFloat) {
                double (*FP)() = (double(*)())(intptr_t)exprSymbol.getAddress();

                fprintf(stderr, "Evaluated to %f\n", FP());
            } else {
                int (*FP)() = (int(*)())(intptr_t)exprSymbol.getAddress();
                fprintf(stderr, "Evaluated to %d\n", FP());
            }

            m_Parser.incrementAnonFuncNum();
        }

        if (!m_Lexer->hasFile()) {
            std::cerr << "(YAPL)>>>";
        }
        expr = m_Parser.parseNext();
    }
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
        std::cerr << "Number expression not recognized" << std::endl;
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
            std::cerr << "Unknown variable" << std::endl;
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

    if (L->getType() != R->getType()) {
        R->mutateType(L->getType());
    }


    if (parsedBinaryOpExpr->getType() == "float") {
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

llvm::Value *IRGenerator::generateFunctionCall(std::shared_ptr<CallFunctionExprAST> parsedFunctionCall) {
    llvm::Function *calleeFunction = getFunction(parsedFunctionCall->getCallee());
    if (!calleeFunction) {
        std::cerr << "Unknown function called" << std::endl;
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
    return m_Builder->CreateCall(calleeFunction, callArgs, "calltmp");
}

/******************** DeclarationAST ********************************************/


llvm::Function *IRGenerator::generateDeclaration(std::shared_ptr<DeclarationAST> parsedDeclaration) {

    if (auto parsedDefinition = std::dynamic_pointer_cast<FunctionDefinitionAST>(parsedDeclaration)) {
        return generateFunctionDefinition(std::move(parsedDefinition));
    }

    if (auto parsedProto = std::dynamic_pointer_cast<PrototypeAST>(parsedDeclaration)) {
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
        } else if (param->getType() == "float") {
            paramTypes.push_back(llvm::Type::getDoubleTy(m_Context));
        } else {
            std::cerr << "Unknown param type: " << param->getType() << " param ignored!" << std::endl;
        }
    }

    llvm::FunctionType * functionType;

    if (parsedPrototype->getType() == "int") {
        functionType = llvm::FunctionType::get(llvm::Type::getInt32Ty(m_Context), paramTypes, false);
    } else if (parsedPrototype->getType() == "float") {
        functionType = llvm::FunctionType::get(llvm::Type::getDoubleTy(m_Context), paramTypes, false);
    } else {
        std::cerr << "Unknown function type: " << parsedPrototype->getType() << " function ignored!" << std::endl;
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
    auto &proto = *parsedFunctionDefinition->getPrototype().get();
    auto p = parsedFunctionDefinition->getPrototype();
    m_FunctionDefs[p->getName()] = std::move(p);

    llvm::Function *function = getFunction(proto.getName());

    if (!function) {
        return nullptr;
    }

    if (!function->empty()) {
        std::cerr << "Function cannot be redefined!" << std::endl;
    }

    llvm::BasicBlock *basicBlock = llvm::BasicBlock::Create(m_Context, "entry", function);
    m_Builder->SetInsertPoint(basicBlock);

    m_NamedValues.clear();

    for (auto &arg : function->args()) {
        m_NamedValues[arg.getName().str()] = &arg;
    }
    auto returnExpr = parsedFunctionDefinition->getReturnExpr();
    if (llvm::Value *retValue = generateTopLevel(returnExpr)) {
        if (retValue->getType() != function->getReturnType())
            retValue->mutateType(function->getReturnType());

        m_Builder->CreateRet(retValue);

        llvm::verifyFunction(*function);

        m_PassManager->run(*function);

        return function;
    }

    function->eraseFromParent();

    return function;
}

void IRGenerator::reloadModuleAndPassManger() {
    m_Module = std::make_unique<llvm::Module>("JIT", m_Context);
    m_Module->setDataLayout(m_YAPLJIT->getDataLayout());
    m_PassManager = std::make_unique<PassManager>(m_Module.get());
}

llvm::Function *IRGenerator::getFunction(const std::string &name) {
    if (auto *func = m_Module->getFunction(name)) {
        return func;
    }

    auto funcDef = m_FunctionDefs.find(name);

    if (funcDef != m_FunctionDefs.end()) {
        return generatePrototype(funcDef->second);
    }

    return nullptr;
}
