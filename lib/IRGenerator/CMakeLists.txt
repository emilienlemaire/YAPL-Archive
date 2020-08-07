add_library(irgenerator SHARED
        IRGenerator.cpp)

if(APPLE)
    target_link_directories(yapl PRIVATE "${CMAKE_SOURCE_DIR}/llvm-libs")
endif(APPLE)

target_link_libraries(irgenerator PRIVATE
        parser functionpass)

target_link_libraries(irgenerator PUBLIC
        ${llvm_libs})
