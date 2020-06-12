#include <iostream>
#include <memory>

#include "AST/ExprAST.hpp"
#include "IRGenerator/IRGenerator.hpp"
#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"

int main(int argc, char* argv[]) {

    std::cerr << "YAPL v0.0.3" << std::endl;

    if (argc < 2) {
        IRGenerator generator("");
        generator.generate();
    } else {
        IRGenerator generator(argv[1]);
        generator.generate();
    }

    return 0;
}
