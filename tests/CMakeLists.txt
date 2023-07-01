if (NOT SKLK_PHY_MOD_ENABLE_TESTS)
    return()
endif ()

if (NOT DEFINED SklkPhy_SRC)
    find_package(SklkPhyRefplat CONFIG REQUIRED)
endif ()

add_subdirectory(util)

########################################################################
## Integration tests for all mod libraries
########################################################################

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
    get_target_property(TARGET_TYPE ${TARGET} TYPE)

    if (NOT TARGET_TYPE STREQUAL "SHARED_LIBRARY")
        continue()
    endif ()

    get_target_property(TARGET_IS_SKLK_PHY_MOD_LIBRARY ${TARGET} IS_SKLK_PHY_MOD_LIBRARY)

    if (NOT TARGET_IS_SKLK_PHY_MOD_LIBRARY)
        continue()
    endif ()

    # Refplat integration tests
    sklk_phy_mod_add_test(
            TARGET test_refplat_one_connection_${TARGET}
            SOURCES test_refplat_one_connection.cpp
            LIBRARIES refplat_test
            ARGS $<TARGET_FILE:${TARGET}> "${CMAKE_CURRENT_SOURCE_DIR}/ndjsons"
    )

    sklk_phy_mod_add_test(
            TARGET test_refplat_two_connections_${TARGET}
            SOURCES test_refplat_two_connections.cpp
            LIBRARIES refplat_test
            ARGS $<TARGET_FILE:${TARGET}> "${CMAKE_CURRENT_SOURCE_DIR}/ndjsons"
    )
endforeach ()
