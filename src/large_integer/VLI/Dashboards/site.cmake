set(CTEST_SITE "${PREDEFINED_CTEST_SITE}")
set(CTEST_BUILD_NAME "${PREDEFINED_CTEST_BUILD_NAME}")
set(CTEST_BUILD_CONFIGURATION Debug)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_SOURCE_DIRECTORY "${PREDEFINED_CTEST_SOURCE_DIRECTORY}")
set(CTEST_BINARY_DIRECTORY "${PREDEFINED_CTEST_BINARY_DIRECTORY}")
set(CTEST_UPDATE_COMMAND "svn")
set(CTEST_PROJECT_SUBPROJECTS vli)

set(subproject ${CTEST_PROJECT_SUBPROJECTS})
set_property(GLOBAL PROPERTY SubProject ${subproject})
set_property(GLOBAL PROPERTY Label ${subproject})

include(${CTEST_SCRIPT_DIRECTORY}/cmake_common.cmake)
