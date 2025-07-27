include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(utils_test 
    imsi_test.cpp
)

target_link_libraries(utils_test
    pgw_utils
    GTest::gtest_main  
    GTest::gmock_main)