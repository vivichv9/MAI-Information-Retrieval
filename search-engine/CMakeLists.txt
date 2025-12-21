cmake_minimum_required(VERSION 3.20)
project(search_engine_cpp LANGUAGES CXX)

option(ENABLE_MONGODB "Enable MongoDB (mongocxx) loader" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

add_executable(search_engine
  src/main.cpp
  src/tokenizer/html_strip.cpp
  src/tokenizer/tokenizer.cpp
  src/stemmer/stemmer.cpp
  src/search/boolean_query_parser.cpp
  src/search/search_engine.cpp
  src/index/index_builder.cpp
  src/web/web_server.cpp
  src/cli/cli.cpp
)

target_include_directories(search_engine PRIVATE src)
target_link_libraries(search_engine PRIVATE httplib::httplib)

if (ENABLE_MONGODB)
  target_compile_definitions(search_engine PRIVATE ENABLE_MONGODB=1)

  find_package(mongocxx QUIET)
  find_package(bsoncxx QUIET)

  if (mongocxx_FOUND AND bsoncxx_FOUND)
    target_link_libraries(search_engine PRIVATE mongo::mongocxx_shared mongo::bsoncxx_shared)
  else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MONGOCXX REQUIRED libmongocxx)
    pkg_check_modules(BSONCXX REQUIRED libbsoncxx)

    target_include_directories(search_engine PRIVATE ${MONGOCXX_INCLUDE_DIRS} ${BSONCXX_INCLUDE_DIRS})
    target_link_directories(search_engine PRIVATE ${MONGOCXX_LIBRARY_DIRS} ${BSONCXX_LIBRARY_DIRS})
    target_link_libraries(search_engine PRIVATE ${MONGOCXX_LIBRARIES} ${BSONCXX_LIBRARIES})
    target_compile_options(search_engine PRIVATE ${MONGOCXX_CFLAGS_OTHER} ${BSONCXX_CFLAGS_OTHER})
  endif()
endif()

# Warnings
if (MSVC)
  target_compile_options(search_engine PRIVATE /W4)
else()
  target_compile_options(search_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
