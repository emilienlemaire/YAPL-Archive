#pragma once

#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/Support/Error.h"
#include <llvm/ADT/StringRef.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/IRTransformLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

#include <memory>
#include <type_traits>

class YAPLJIT {
private:
    llvm::orc::ExecutionSession m_ExecutionSession;
    llvm::orc::RTDyldObjectLinkingLayer m_ObjectLayer;
    llvm::orc::IRCompileLayer m_CompileLayer;
    llvm::orc::IRTransformLayer m_OptimizeLayer;

    llvm::DataLayout m_DataLayout;
    llvm::orc::MangleAndInterner m_Mangle;
    llvm::orc::ThreadSafeContext m_TSContext;

    llvm::orc::JITDylib &m_MainJITDylib;

public:
    YAPLJIT(llvm::orc::JITTargetMachineBuilder targetMachineBuilder, llvm::DataLayout dataLayout)
        : m_ObjectLayer(
                m_ExecutionSession,
                []() {
                    return std::make_unique<llvm::SectionMemoryManager>();
                }
            ),
        m_CompileLayer(
                m_ExecutionSession,
                m_ObjectLayer,
                std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(targetMachineBuilder))
            ),
        m_OptimizeLayer(
                m_ExecutionSession,
                m_CompileLayer,
                optimizeModule
            ),
        m_DataLayout(std::move(dataLayout)),
        m_Mangle(m_ExecutionSession, this->m_DataLayout),
        m_TSContext(std::make_unique<llvm::LLVMContext>()),
        m_MainJITDylib(m_ExecutionSession.createBareJITDylib("<main>"))
        {
            m_MainJITDylib.addGenerator(
                    llvm::cantFail(
                        llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
                                m_DataLayout.getGlobalPrefix()
                            )
                        )
                    );
        }

    static llvm::Expected<std::unique_ptr<YAPLJIT>> Create() {
        auto JITTargetMachine = llvm::orc::JITTargetMachineBuilder::detectHost();

        if (!JITTargetMachine) {
            return JITTargetMachine.takeError();
        }

        auto dataLayout = JITTargetMachine->getDefaultDataLayoutForTarget();

        if (!dataLayout) {
            return dataLayout.takeError();
        }

        return std::make_unique<YAPLJIT>(std::move(*JITTargetMachine), std::move(*dataLayout));
    }

    const llvm::DataLayout &getDataLayout() const { return m_DataLayout; }

    llvm::LLVMContext &getContext() { return *m_TSContext.getContext(); }

    llvm::Error addModule(std::unique_ptr<llvm::Module> module) {
        return m_OptimizeLayer.add( m_MainJITDylib, llvm::orc::ThreadSafeModule(
                    std::move(module),
                    m_TSContext
                    )
                );
    }

    llvm::Expected<llvm::JITEvaluatedSymbol> lookup(const std::string &name) {
        return m_ExecutionSession.lookup({&m_MainJITDylib}, m_Mangle(name));
    }
private:
    static llvm::Expected<llvm::orc::ThreadSafeModule> optimizeModule(
            llvm::orc::ThreadSafeModule tsModule,
            const llvm::orc::MaterializationResponsibility &responsibility
            ) {
        tsModule.withModuleDo(
                [](llvm::Module &module) {
                    auto functionPassManager = std::make_unique<llvm::legacy::FunctionPassManager>(&module);
                    functionPassManager->add(llvm::createInstructionCombiningPass());
                    functionPassManager->add(llvm::createReassociatePass());
                    functionPassManager->add(llvm::createGVNPass());
                    functionPassManager->add(llvm::createCFGSimplificationPass());
                    functionPassManager->doInitialization();

                    for (auto &function: module) {
                        functionPassManager->run(function);
                    }
                }
            );

        return std::move(tsModule);
    }
};
