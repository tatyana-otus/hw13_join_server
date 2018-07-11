cmake_minimum_required(VERSION 3.2)

project(join_server VERSION 1.0.$ENV{TRAVIS_BUILD_NUMBER})

add_executable(join_server main.cpp)

find_package(Threads)
find_package(Boost COMPONENTS system  REQUIRED)
#find_package(Boost COMPONENTS system program_options REQUIRED)


target_include_directories(join_server PRIVATE ./include)

set_target_properties(join_server PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
 # COMPILE_DEFINITIONS BOOST_ASIO_ENABLE_HANDLER_TRACKING
  COMPILE_OPTIONS -Wall -Wextra -Wpedantic 
)

target_link_libraries(join_server
        ${Boost_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        )

#set (CMAKE_CXX_FLAGS "-fsanitize=thread")

install(TARGETS join_server RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT lisstic.tt@gmail.com)

include (CPack)

add_subdirectory(tests)
enable_testing()
