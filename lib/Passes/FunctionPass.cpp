
#include "Passes/FunctionPass.hpp"
#include "llvm/Support/raw_ostream.h"

char FunctionPass::ID = 0;

bool FunctionPass::runOnFunction(llvm::Function &function) {
    llvm::errs() << "Running function pass on: ";
    llvm::errs().write_escaped(function.getName()) << "\n";
    return false;
}

static llvm::RegisterPass<FunctionPass> X("function", "My Custom Function Pass", false, false);
