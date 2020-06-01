#pragma once

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

class YAPLJIT {
public:
    using ObjectLayerT = llvm::orc::LegacyRTDyldObjectLinkingLayer;
    using CompileLayerT = llvm::orc::LegacyIRCompileLayer<ObjectLayerT, llvm::orc::SimpleCompiler>;

private:
    llvm::orc::ExecutionSession m_ExecutionSession;
    std::shared_ptr<llvm::orc::SymbolResolver> m_Resolver;
    std::unique_ptr<llvm::TargetMachine> m_TargetMachine;
    const llvm::DataLayout m_DataLayout;
    ObjectLayerT m_ObjectLayer;
    CompileLayerT m_CompileLayer;
    std::vector<llvm::orc::VModuleKey> m_ModuleKeys;

public:
    YAPLJIT()
        : m_Resolver(llvm::orc::createLegacyLookupResolver(
                        m_ExecutionSession,
                        [this](llvm::StringRef name) {
                            return findMangledSymbol(name);
                        },
                        [](llvm::Error err) {
                            llvm::cantFail(std::move(err), "lookupFlags failed.");
                        }
                    )
                ),
        m_TargetMachine(llvm::EngineBuilder().selectTarget()),
        m_DataLayout(m_TargetMachine->createDataLayout()),
        m_ObjectLayer(
                llvm::AcknowledgeORCv1Deprecation,
                m_ExecutionSession,
                [this](llvm::orc::VModuleKey) {
                return ObjectLayerT::Resources {
                        std::make_shared<llvm::SectionMemoryManager>(),
                        m_Resolver
                    };
                }
            ),
        m_CompileLayer(
                    llvm::AcknowledgeORCv1Deprecation,
                    m_ObjectLayer,
                    llvm::orc::SimpleCompiler(*m_TargetMachine)
                )
        {
            llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
        }

    llvm::TargetMachine &getTargetMachine() { return *m_TargetMachine; }

    llvm::orc::VModuleKey addModule(std::unique_ptr<llvm::Module> module) {
        auto key = m_ExecutionSession.allocateVModule();
        llvm::cantFail(m_CompileLayer.addModule(key, std::move(module)));
        m_ModuleKeys.push_back(key);
        return key;
    }

    void removeModule(llvm::orc::VModuleKey key) {
        m_ModuleKeys.erase(llvm::find(m_ModuleKeys, key));
        llvm::cantFail(m_CompileLayer.removeModule(key));
    }

    llvm::JITSymbol findSymbol(const std::string &name) {
        return findMangledSymbol(mangle(name));
    }

private:
    std::string mangle(const std::string &name) {
        std::string mangledName;

        {
            llvm::raw_string_ostream mangledNameStream(mangledName);
            llvm::Mangler::getNameWithPrefix(mangledNameStream, name, m_DataLayout);
        }

        return mangledName;
    }
    
    llvm::JITSymbol findMangledSymbol(const std::string &name) {
#ifdef _WIN32
        const bool exportedSymbolOnly = false;
#else
        const bool exportedSymbolOnly = true;
#endif

        for (auto key : llvm::make_range(m_ModuleKeys.begin(), m_ModuleKeys.end())) {
            if (auto symbol = m_CompileLayer.findSymbolIn(key, name, exportedSymbolOnly)) {
                return symbol;
            }
        }

        if (auto symbolAddress = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name)) {
            return llvm::JITSymbol(symbolAddress, llvm::JITSymbolFlags::Exported);
        }

#ifdef _WIN32
        if (name.length() > 2 && name[0] == '_') {
            if (auto symbolAddress = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name.substr(1))) {
                return llvm::JITSymbol(symbolAddress, llvm::JITSymbolFlags::Exported);
            }
        }
#endif

        return nullptr;
    }
};
