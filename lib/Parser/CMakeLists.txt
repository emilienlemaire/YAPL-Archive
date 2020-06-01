add_library(parser STATIC
        Parser.cpp)

target_link_libraries(parser PRIVATE lexer)