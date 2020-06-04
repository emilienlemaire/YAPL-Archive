#include <iostream>
#include <memory>

#include "AST/ExprAST.hpp"
#include "IRGenerator/IRGenerator.hpp"
#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        Lexer lexer("");
        Parser parser(std::make_shared<Lexer>(lexer));

        parser.parse();
        //IRGenerator generator("");
        //generator.generate();
    } else {
        Lexer lexer(argv[1]);
        Parser parser(std::make_shared<Lexer>(lexer));

        auto expr = std::make_shared<ExprAST>();

        while (std::dynamic_pointer_cast<EOFExprAST>(expr)) {
            expr = parser.parseNext();
        }
        //IRGenerator generator(argv[1]);
        //generator.generate();
    }

    return 0;
}
