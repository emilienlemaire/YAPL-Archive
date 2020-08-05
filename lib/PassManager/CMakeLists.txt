add_library(passmanager STATIC PassManager.cpp)

target_link_libraries(passmanager PUBLIC ${llvm_libs})
