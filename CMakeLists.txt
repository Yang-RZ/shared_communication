cmake_minimum_required(VERSION 3.0.2)

set(supported_languages CXX)

project(shared_comm LANGUAGES ${supported_languages})

if (NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif (NOT DEFINED CMAKE_BUILD_TYPE)

if (WIN32)
  set(Boost_INCLUDE_DIR "C:/Apps/boost_1_84_0")
elseif(UNIX)
  set(CMAKE_EXE_LINKER_FLAGS "-Wl,--no-as-needed -ldl")
endif()

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS system thread filesystem log timer)
find_package(Threads)

file(GLOB_RECURSE HEADER_DIR_LIST "include/*.h")

add_library(${PROJECT_NAME} SHARED
  ${HEADER_DIR_LIST}
  src/shared_comm_base.cpp
  src/shared_socket.cpp
  src/sync_helper.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/api>
  $<INSTALL_INTERFACE:include/api>
)
target_compile_options(${PROJECT_NAME} PRIVATE -std=c++14)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(repair_communication tools/repair_communication.cpp)
target_link_libraries(repair_communication ${PROJECT_NAME})

# examples
option(SHARED_COMM_BUILD_EXAMPLES "Build examples" ON)

if(SHARED_COMM_BUILD_EXAMPLES)
  add_executable(test_client1 test_cases/test_client.cpp)
  target_link_libraries(test_client1 ${PROJECT_NAME})

  add_executable(test_client2 test_cases/test_client.cpp)
  target_link_libraries(test_client2 ${PROJECT_NAME})

  add_executable(test_client3 test_cases/test_client.cpp)
  target_link_libraries(test_client3 ${PROJECT_NAME})

  add_executable(test_client4 test_cases/test_client.cpp)
  target_link_libraries(test_client4 ${PROJECT_NAME})

  add_executable(test_server test_cases/test_server.cpp)
  target_include_directories(test_server PRIVATE ${Boost_INCLUDE_DIRS}
  )
  target_link_libraries(test_server ${PROJECT_NAME} ${Boost_LIBRARIES})
endif(SHARED_COMM_BUILD_EXAMPLES)

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}-targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)
install(DIRECTORY include/${PROJECT_NAME}
  DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)

# install CMake package configuration
install(EXPORT ${PROJECT_NAME}-targets DESTINATION lib/${PROJECT_NAME})
install(FILES ${PROJECT_NAME}-config.cmake DESTINATION lib/${PROJECT_NAME})
