add_library(passmanager STATIC passmanager.cpp)

target_link_libraries(passmanager PUBLIC ${llvm_libs})
