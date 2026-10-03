# The tests: googletest's, as a module, from cmake-everywhere.
cme_declare_port(NAME googletest-modules PROVIDES GTest
  GITHUB_REPOSITORY j4niwzis/googletest-modules
  GIT_TAG 706920f1d230f0316d271466d8885735be3e300e
  LICENSE BSD-3-Clause
  SYSTEM NEVER
  TARGETS gtest_main)
cme_options(googletest-modules
  "BUILD_GMOCK OFF" "INSTALL_GTEST OFF" "gtest_build_modules ON"
  "gtest_build_tests OFF" "gtest_build_samples OFF")
cme_find_package(GTest REQUIRED)
if(TARGET GTest::gtest_main)
  set(splice_gtest GTest::gtest_main)
else()
  set(splice_gtest gtest_main)
endif()

foreach(name IN ITEMS variant_test remap_test erased_call_test)
  add_executable(splice-${name} ${name}.cc)
  target_link_libraries(splice-${name} PRIVATE splice ${splice_gtest})
  add_test(NAME ${name} COMMAND splice-${name})
endforeach()
