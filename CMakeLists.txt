########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 3.1)
project (sklkphy-mod C CXX)
enable_testing()

# Enable asserts in release mode
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSKLK_PHY_DEBUG")
string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

########################################################################
# Find sklk dependencies
########################################################################
find_package(SklkMii CONFIG REQUIRED)
find_package(SklkJSON CONFIG REQUIRED)

if (DEFINED SklkPhy_SRC)
    message(STATUS "Using SklkPhy source in ${SklkPhy_SRC}")
    add_subdirectory(${SklkPhy_SRC} ${CMAKE_CURRENT_BINARY_DIR}/sklk-phy)
else ()
    find_package(SklkPhy CONFIG REQUIRED)
endif ()

#########################################################################
# Repo setup (needs sklk-mii)
#########################################################################
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)
include(SklkCommon)
sklk_init_repo()

#########################################################################
# Generate this repo's version strings
#########################################################################
set(SKLK_PHY_MOD_ABI_SUFFIX 0) # Automatically changed by CI
set(CPACK_PACKAGE_VERSION_MAJOR 3) # Change this manually to increment
set(GIT_MAIN_BRANCH_NAME "main")

include(SklkGenerateVersions)
sklk_cmake_generate_versions(SKLK_PHY_MOD_VERSION SKLK_PHY_MOD_LIBVER SKLK_PHY_MOD_EXTVER SKLK_PHY_MOD_ABI_SUFFIX SKLK_PHY_MOD_ABI_VERSION)

#########################################################################
# Packaging
#########################################################################
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "support@skylarkwireless.com")
SET(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
SET(CPACK_DEBIAN_PACKAGE_NAME "sklk_phy_mod")
SET(CPACK_PACKAGE_FILE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "sklk_phy, sklk_json, sklk_mii")
SET(CPACK_DEBIAN_PACKAGE_CONFLICTS "sklk_tools")

message(STATUS "make .deb: `make package`")
INCLUDE(CPack)

########################################################################
# SWIG CMake policies
########################################################################
cmake_policy(PUSH)

if (POLICY CMP0078)
    cmake_policy(SET CMP0078 NEW)
endif ()
if (POLICY CMP0086)
    cmake_policy(SET CMP0086 NEW)
endif()

########################################################################
# Install project config
########################################################################
include(CMakePackageConfigHelpers)
set(CMAKE_LIB_DEST ${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME})

write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/SklkPhyModConfigVersion.cmake
    VERSION ${SKLK_PHY_MOD_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(FILES
    ${PROJECT_SOURCE_DIR}/cmake/Modules/SklkPhyModConfig.cmake
    ${PROJECT_BINARY_DIR}/SklkPhyModConfigVersion.cmake
    DESTINATION ${CMAKE_LIB_DEST})

########################################################################
# components
########################################################################
sklk_feature("sklk-phy-mod tests" SKLK_PHY_MOD_ENABLE_TESTS "Enable sklk-phy-mod tests" ON "" OFF)

function(sklk_phy_mod_mark_mod_library TARGET)
    if (NOT TARGET ${TARGET})
        message(WARNING "${TARGET} is not a target")
        return()
    endif ()

    set_target_properties(${TARGET} PROPERTIES IS_SKLK_PHY_MOD_LIBRARY YES)
endfunction()

function(sklk_phy_mod_add_test)
    cmake_parse_arguments(PARSE_ARGV 0 SKLK_PHY_MOD_ADD_TEST "" "TARGET" "SOURCES;LIBRARIES;ARGS")
    add_executable(${SKLK_PHY_MOD_ADD_TEST_TARGET} ${SKLK_PHY_MOD_ADD_TEST_SOURCES})
    target_link_libraries(${SKLK_PHY_MOD_ADD_TEST_TARGET} ${SKLK_PHY_MOD_ADD_TEST_LIBRARIES})
    add_test(NAME ${SKLK_PHY_MOD_ADD_TEST_TARGET} COMMAND ${SKLK_PHY_MOD_ADD_TEST_TARGET} ${SKLK_PHY_MOD_ADD_TEST_ARGS})
endfunction(sklk_phy_mod_add_test)

add_subdirectory(ref_design)

if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_subdirectory(tests)
endif ()

sklk_feature("sklk-phy-mod doxygen documentation" SKLK_PHY_MOD_ENABLE_DOCS
    "Enable sklk-phy-mod doxygen documentation" OFF "" ON)

if (SKLK_PHY_MOD_ENABLE_DOCS)
    add_subdirectory(docs/doxygen)
endif ()

cmake_policy(POP)

#########################################################################
# summary
#########################################################################
if(NOT LIBRARY_BUILD)
    sklk_print_module_summary(SKLK_PHY_MOD_VERSION SKLK_PHY_MOD_ABI_VERSION)
endif()
