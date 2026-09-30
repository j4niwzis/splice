cmake_minimum_required(VERSION 4.3.4 FATAL_ERROR)

# splice: what the libraries of the family share -- splice::variant, a
# variant built in time linear in its alternatives, and splice::overloaded --
# as C++26 modules.
include(${CMAKE_CURRENT_LIST_DIR}/cmake/import-std.cmake)
# C++23 is all splice needs, and the standard is the one of what imports it:
# a module is built once, for the standard its target has, and a library of
# the family built as C++23 (skiff, skiff-widgets) imports it as that. Built
# alone, for its tests: C++26, as the family is.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(CMAKE_CXX_STANDARD 26)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

project(splice VERSION 0.1 LANGUAGES CXX)

option(SPLICE_TESTS "Build the tests" ${PROJECT_IS_TOP_LEVEL})
option(SPLICE_ERASE "Outside a release build, a variant's alternatives reached through tables of function pointers" ON)

if(NOT COMMAND cme_find_package)
  set(CME_WITHOUT_PROVIDER ON)
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/get_cme.cmake)
endif()

# The room for constant evaluation. Said as a usage requirement of every
# module, it also makes their settings differ from the std target's, so that
# they, what imports them and googletest share one std. A project using
# several libraries of the family sets one value for all of them.
set(SPLICE_CONSTEXPR_STEPS 300000000 CACHE STRING
  "clang's -fconstexpr-steps for splice and what imports it")

add_library(splice)
target_sources(splice PUBLIC FILE_SET CXX_MODULES BASE_DIRS src
  FILES src/variant.cc src/overloaded.cc src/splice.cc)
target_compile_features(splice PUBLIC cxx_std_23)
target_compile_options(splice PUBLIC
  $<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>:-fconstexpr-steps=${SPLICE_CONSTEXPR_STEPS}>)
# A variant's alternatives through tables outside a release build: the
# build's time, not the program's.
target_compile_definitions(splice PUBLIC
  $<$<AND:$<BOOL:${SPLICE_ERASE}>,$<NOT:$<CONFIG:Release>>>:SPLICE_ERASED>)
add_library(splice::splice ALIAS splice)

if(SPLICE_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
