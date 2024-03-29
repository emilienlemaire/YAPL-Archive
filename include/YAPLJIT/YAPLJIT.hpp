#pragma once

#include <llvm/ADT/StringRef.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LLVMContext.h>

#include <memory>

class YAPLJIT {
private:
    llvm::orc::ExecutionSession m_ExecutionSession;
    llvm::orc::RTDyldObjectLinkingLayer m_ObjectLayer;
    llvm::orc::IRCompileLayer m_CompileLayer;

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
        return m_CompileLayer.add(m_MainJITDylib,
                llvm::orc::ThreadSafeModule(std::move(module), m_TSContext));
    }

    llvm::Expected<llvm::JITEvaluatedSymbol> lookup(const std::string &name) {
        return m_ExecutionSession.lookup({&m_MainJITDylib}, m_Mangle(name));
    }

};
