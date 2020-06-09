add_library(irgenerator STATIC
        IRGenerator.cpp)

target_link_directories(yapl PRIVATE "${CMAKE_SOURCE_DIR}/llvm-libs")
target_link_libraries(irgenerator PRIVATE
        parser functionpass)

target_link_libraries(irgenerator PUBLIC
        ${llvm_libs})
