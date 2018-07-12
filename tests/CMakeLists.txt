project(test)

find_package(Boost COMPONENTS system unit_test_framework REQUIRED)
find_package(Threads)

add_executable(test_server_start_cli   test_server_start.cpp )
add_executable(test_server_cli         test_server.cpp )


target_include_directories(test_server_start_cli  PRIVATE ../include)
target_include_directories(test_server_start_cli  PRIVATE ${Boost_INCLUDE_DIR})

target_include_directories(test_server_cli PRIVATE ../include)
target_include_directories(test_server_cli PRIVATE ${Boost_INCLUDE_DIR})

set_target_properties(test_server_start_cli test_server_cli PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
  COMPILE_OPTIONS -Wpedantic -Wall -Wextra
  COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK
)
#set (CMAKE_CXX_FLAGS "-fsanitize=thread")

target_link_libraries(test_server_start_cli
  ${CMAKE_THREAD_LIBS_INIT} 
  ${Boost_LIBRARIES}
)

  target_link_libraries(test_server_cli
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
  )


set(CTEST_OUTPUT_ON_FAILURE "TRUE")

enable_testing()

add_test(test_start test_server_start_cli  -t test_suite_main_1)

add_test(send_cmd            test_server_cli -t test_suite_main_1)
add_test(send_invalid_cmd    test_server_cli -t test_suite_main_2)
add_test(send_multy_cmd      test_server_cli -t test_suite_main_3)
add_test(send_cmd_get_tables test_server_cli -t test_suite_main_4)