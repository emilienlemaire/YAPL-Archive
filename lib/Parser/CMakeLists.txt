add_library(parser SHARED
        Parser.cpp)

target_link_libraries(parser PRIVATE lexer ${llvm_libs})
