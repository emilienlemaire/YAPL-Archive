

#include "PassManager/PassManager.hpp"

PassManager::PassManager(std::shared_ptr<llvm::Module> module)
    : m_Module(module)
{
    m_FunctionPassManager = std::make_unique<llvm::legacy::FunctionPassManager>(m_Module.get());

    m_FunctionPassManager->add(llvm::createInstructionCombiningPass());
    m_FunctionPassManager->add(llvm::createReassociatePass());
    m_FunctionPassManager->add(llvm::createGVNPass());
    m_FunctionPassManager->add(llvm::createCFGSimplificationPass());

    m_FunctionPassManager->doInitialization();
}

void PassManager::run(llvm::Function &function){
    m_FunctionPassManager->run(function);
}
