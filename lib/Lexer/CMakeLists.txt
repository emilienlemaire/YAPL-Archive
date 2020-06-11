add_library(lexer SHARED
        Lexer.cpp)

target_link_libraries(lexer PUBLIC cpplogger)
