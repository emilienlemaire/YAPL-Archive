set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.15.sdk)

add_library(irgenerator STATIC
        IRGenerator.cpp)

target_link_libraries(irgenerator PRIVATE
        parser)

target_link_libraries(irgenerator PUBLIC
        ${llvm_libs})
