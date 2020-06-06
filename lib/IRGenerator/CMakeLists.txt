set(${llvm_libs}
    LLVMAggressiveInstCombine
    LLVMAnalysis
    LLVMAsmPrinter
    LLVMAsmParser
    LLVMBinaryFormat
    LLVMBitReader
    LLVMBitWriter
    LLVMBitstreamReader
    LLVMCodeGen
    LLVMCore
    LLVMCoroutines
    LLVMDebugInfoCodeView
    LLVMDebugInfoDWARF
    LLVMDebugInfoMSF
    LLVMDemangle
    LLVMExecutionEngine
    LLVMGlobalISel
    LLVMIRReader
    LLVMInstCombine
    LLVMInstrumentation
    LLVMLinker
    LLVMMC
    LLVMMCDisassembler
    LLVMMCJIT
    LLVMMCParser
    LLVMObject
    LLVMPasses
    LLVMProfileData
    LLVMRemarks
    LLVMRuntimeDyld
    LLVMScalarOpts
    LLVMSelectionDAG
    LLVMSupport
    LLVMTarget
    LLVMTextAPI
    LLVMTransformUtils
    LLVMVectorize
    LLVMX86AsmParser
    LLVMX86CodeGen
    LLVMX86Desc
    LLVMX86Disassembler
    LLVMX86Info
    LLVMFrontendOpenMP
    LLVMMLPolicies
    LLVMOrcJIT
    LLVMOrcError
    LLVMJITLink
    )

add_library(irgenerator STATIC
        IRGenerator.cpp)

target_link_directories(yapl PRIVATE "${CMAKE_SOURCE_DIR}/llvm-libs")
target_link_libraries(irgenerator PRIVATE
        parser passmanager)

target_link_libraries(irgenerator PUBLIC
        ${llvm_libs})
