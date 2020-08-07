add_library(functionpass SHARED
    FunctionPass.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

if(APPLE)
    target_link_libraries(functionpass
        "$<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>")
endif(APPLE)

