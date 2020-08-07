
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
struct FunctionPass : public llvm::FunctionPass {
    static char ID;
    
    FunctionPass()
        : llvm::FunctionPass(ID) 
    {}

    virtual bool runOnFunction(llvm::Function &function) override;
};

