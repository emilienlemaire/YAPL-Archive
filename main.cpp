#include <iostream>
#include <memory>

#include "IRGenerator/IRGenerator.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "File to compile missing" << std::endl;
        exit(1);
    }

    IRGenerator generator(argv[1]);

    generator.generate();

    return 0;
}
