#include <iostream>
#include <memory>

#include "IRGenerator/IRGenerator.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {

        IRGenerator generator("");
        generator.generate();

    } else {
    
        IRGenerator generator(argv[1]);
        generator.generate();
    }

    return 0;
}
