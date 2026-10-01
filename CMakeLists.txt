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

# ---- What this library is, said by the library ----
#
# A consumer writes find_package(splice) and this says the rest: where it
# comes from, what it needs, and what it produces. cmake-everywhere installs
# the declaration beside the library, under share/cmake-everywhere/ports, so
# a prefix that has splice has its description too -- what lets a machine
# answer for it, instead of everyone fetching it again.
if(COMMAND cme_declare_port)
  cme_declare_port(
    NAME splice
    PROVIDES splice
    VERSION ${PROJECT_VERSION}
    GITHUB_REPOSITORY j4niwzis/splice
    GIT_TAG main
    LICENSE GPL-3.0-only
    TARGETS splice::splice)
endif()

# ---- Installing it ----
#
# The library, its module interface units -- a consumer compiles its own
# binary interfaces from them -- and a package, so that find_package(splice)
# on a machine that has it installed finds it there, as splice::splice: what
# the libraries of the family link, here and from a prefix alike.
option(SPLICE_INSTALL "Install splice and its CMake package" ${PROJECT_IS_TOP_LEVEL})
if(SPLICE_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)
  install(TARGETS splice
    EXPORT spliceTargets
    FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_LIBDIR}/splice/modules
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(EXPORT spliceTargets
    NAMESPACE splice::
    CXX_MODULES_DIRECTORY modules
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/splice)
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/spliceConfig.cmake"
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/spliceTargets.cmake\")\n"
    "if(NOT TARGET splice)\n  add_library(splice ALIAS splice::splice)\nendif()\n")
  write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/spliceConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/spliceConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/spliceConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/splice)
endif()

if(SPLICE_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
