find_package(SklkCppTest)
if (DEFINED SklkPhy_SRC)
    set(SklkPhyRefPlat_FOUND TRUE)
else()
    find_package(SklkPhyRefplat CONFIG)
endif ()

sklk_feature("sklk-phy-mod tests" SKLK_PHY_MOD_ENABLE_TESTS "Enable sklk-phy-mod tests" ON "SklkCppTest_FOUND;SklkPhyRefplat_FOUND" OFF)
if (NOT SKLK_PHY_MOD_ENABLE_TESTS)
    return()
endif ()

function(sklk_phy_mod_add_test)
    cmake_parse_arguments(PARSE_ARGV 0 SKLK_PHY_MOD_ADD_TEST "" "TARGET" "SOURCES;LIBRARIES;ARGS;ENVVARS")
    add_executable(${SKLK_PHY_MOD_ADD_TEST_TARGET} ${SKLK_PHY_MOD_ADD_TEST_SOURCES})
    target_link_libraries(${SKLK_PHY_MOD_ADD_TEST_TARGET} PUBLIC ${SKLK_PHY_MOD_ADD_TEST_LIBRARIES} sklkcpptest sklkcpptest-main)
    add_test(NAME ${SKLK_PHY_MOD_ADD_TEST_TARGET} COMMAND ${SKLK_PHY_MOD_ADD_TEST_TARGET} ${SKLK_PHY_MOD_ADD_TEST_ARGS})
    if(SKLK_PHY_MOD_ADD_TEST_ENVVARS)
        set_property(TEST ${SKLK_PHY_MOD_ADD_TEST_TARGET} PROPERTY ENVIRONMENT ${SKLK_PHY_MOD_ADD_TEST_ENVVARS})
    endif()
endfunction(sklk_phy_mod_add_test)

########################################################################
## Test common
########################################################################
set(mod_library "")

function(_get_all_targets RESULT_VAR DIR)
    get_property(SUBDIRS DIRECTORY "${DIR}" PROPERTY SUBDIRECTORIES)
    foreach (SUBDIR IN LISTS SUBDIRS)
        _get_all_targets(${RESULT_VAR} "${SUBDIR}")
    endforeach ()

    get_directory_property(SUB_TARGETS DIRECTORY "${DIR}" BUILDSYSTEM_TARGETS)
    set(${RESULT_VAR} ${${RESULT_VAR}} ${SUB_TARGETS} PARENT_SCOPE)
endfunction()

get_filename_component(SKLK_PHY_MOD_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
_get_all_targets(ALL_TARGETS ${SKLK_PHY_MOD_ROOT_DIR})

foreach (TARGET IN LISTS ALL_TARGETS)
    if(mod_library)
        continue()
    endif()

    get_target_property(TARGET_TYPE ${TARGET} TYPE)
    if (NOT TARGET_TYPE STREQUAL "SHARED_LIBRARY")
        continue()
    endif ()

    get_target_property(TARGET_IS_SKLK_PHY_MOD_LIBRARY ${TARGET} IS_SKLK_PHY_MOD_LIBRARY)
    if (TARGET_IS_SKLK_PHY_MOD_LIBRARY)
        set(mod_library ${TARGET})
    endif ()
endforeach()

if(NOT mod_library)
    message(FATAL_ERROR "Could not find mod library")
endif()

########################################################################
## Ref design loader test
########################################################################
sklk_phy_mod_add_test(
        TARGET test_ref_design_loader
        SOURCES test_loader.cpp
        LIBRARIES ${mod_library}
)

########################################################################
## Integration tests for all mod libraries
########################################################################
add_subdirectory(refplat_test)

sklk_phy_mod_add_test(
        TARGET test_refplat_one_connection
        SOURCES test_refplat_one_connection.cpp
        LIBRARIES refplat_test
        ENVVARS "MOD_LIBRARY=$<TARGET_FILE:${mod_library}>;PILOTS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/ndjsons"
)

sklk_phy_mod_add_test(
        TARGET test_refplat_two_connections
        SOURCES test_refplat_two_connections.cpp
        LIBRARIES refplat_test
        ENVVARS "MOD_LIBRARY=$<TARGET_FILE:${mod_library}>;PILOTS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/ndjsons"
)
