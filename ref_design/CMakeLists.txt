########################################################################
## Feature registration
########################################################################
sklk_feature("sklk-phy-mod ref design" SKLK_PHY_MOD_ENABLE_REF_DESIGN "Enable sklk-phy-mod ref design" ON "" OFF)
if (NOT SKLK_PHY_MOD_ENABLE_REF_DESIGN)
    return()
endif()

include(GNUInstallDirs)

set(MOD_LIB "sklkphy_mod_ref_design")

set(mod_sources
    csi_mod.cpp
    loader.cpp
    schedule_mod.cpp
    utils.cpp
    rpc.cpp
)
set(mod_public_includes
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
set(mod_private_includes ${ARMADILLO_INCLUDE_DIRS})
set(mod_public_libs
    sklkmii
    sklkjson
    sklkdsp
    sklkphy
    ${ARMADILLO_LIBRARIES}
)
set(mod_private_options "-Wvla")

sklk_add_library(
    NAME ${MOD_LIB}
    SOURCES ${mod_sources}
    PUBLIC_INCLUDES ${mod_public_includes}
    PUBLIC_LIBRARIES ${mod_public_libs}
    PRIVATE_INCLUDES ${mod_private_includes}
    PRIVATE_OPTIONS ${mod_private_options}
    INSTALL_EXPORT_VAR INSTALL_EXPORT
)
set_target_properties(${MOD_LIB} PROPERTIES SOVERSION ${SKLK_PHY_MOD_ABI_VERSION})
set_target_properties(${MOD_LIB} PROPERTIES VERSION ${SKLK_PHY_MOD_LIBVER})
sklk_phy_mod_mark_mod_library(${MOD_LIB})

install(TARGETS ${MOD_LIB}
        EXPORT SklkPhyModExport
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} # .so file
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} # .lib file
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # .dll file
        )

#export target to project config
install(EXPORT SklkPhyModExport DESTINATION ${CMAKE_LIB_DEST})
