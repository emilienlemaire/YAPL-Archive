#pragma once

#include <memory>

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

class PassManager {
private:
    std::shared_ptr<llvm::Module> m_Module;
    std::unique_ptr<llvm::legacy::FunctionPassManager> m_FunctionPassManager;
public:
    PassManager(std::shared_ptr<llvm::Module> module);
    void run(llvm::Function &function);
};
