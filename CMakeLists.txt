cmake_minimum_required(VERSION 3.16)
project(PocketLedger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ledger src/ledger.cpp)
target_include_directories(ledger PUBLIC include)
if(MSVC)
  target_compile_options(ledger PRIVATE /W4 /WX)
else()
  target_compile_options(ledger PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(pocket-ledger src/main.cpp)
target_link_libraries(pocket-ledger PRIVATE ledger)

include(CTest)
if(BUILD_TESTING)
  add_executable(ledger-tests tests/test_ledger.cpp)
  target_link_libraries(ledger-tests PRIVATE ledger)
  add_test(NAME ledger-tests COMMAND ledger-tests)
endif()
