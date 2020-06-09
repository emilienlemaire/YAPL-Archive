add_library(functionpass SHARED
    FunctionPass.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

target_link_libraries(functionpass
  "$<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>")
