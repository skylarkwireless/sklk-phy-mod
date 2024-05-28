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
find_package(SklkDSP CONFIG REQUIRED)

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
# Find other dependencies
#########################################################################
find_package(Armadillo REQUIRED)

#########################################################################
# Generate this repo's version strings
#########################################################################
set(SKLK_PHY_MOD_ABI_SUFFIX 0) # Automatically changed by CI
set(CPACK_PACKAGE_VERSION_MAJOR 3) # Change this manually to increment
set(GIT_MAIN_BRANCH_NAME "main")

include(SklkGenerateVersions)
sklk_cmake_generate_versions(SKLK_PHY_MOD_VERSION SKLK_PHY_MOD_LIBVER SKLK_PHY_MOD_EXTVER SKLK_PHY_MOD_ABI_SUFFIX SKLK_PHY_MOD_ABI_VERSION)

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
# ref_design
########################################################################
function(sklk_phy_mod_mark_mod_library TARGET)
    if (NOT TARGET ${TARGET})
        message(WARNING "${TARGET} is not a target")
        return()
    endif ()

    set_target_properties(${TARGET} PROPERTIES IS_SKLK_PHY_MOD_LIBRARY YES)
endfunction()

add_subdirectory(ref_design)

########################################################################
# components
########################################################################

add_subdirectory(tests)

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
