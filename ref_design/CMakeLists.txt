# /*
# *   Copyright [2023] [Skylark Wireless LLC]
# *
# *   Licensed under the Apache License, Version 2.0 (the "License");
# *   you may not use this file except in compliance with the License.
# *   You may obtain a copy of the License at
# *
# *       http://www.apache.org/licenses/LICENSE-2.0
# *
# *   Unless required by applicable law or agreed to in writing, software
# *   distributed under the License is distributed on an "AS IS" BASIS,
# *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# *   See the License for the specific language governing permissions and
# *   limitations under the License.
# */

########################################################################
## Dependencies
########################################################################
find_package(yaml-cpp CONFIG)

if (NOT TARGET armadillo)
    add_library(armadillo SHARED IMPORTED GLOBAL)
    find_path(ARMADILLO_INCUDE_DIR NAMES armadillo)
    find_library(ARMADILLO_LIBRARY armadillo required)
    set_property(TARGET armadillo PROPERTY IMPORTED_LOCATION ${ARMADILLO_LIBRARY})
    set_property(TARGET armadillo PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${ARMADILLO_INCUDE_DIR})
    set_property(TARGET armadillo APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS ARMA_USE_CXX11)
    message(STATUS "ARMADILLO_INCUDE_DIR=${ARMADILLO_INCUDE_DIR}")
    message(STATUS "ARMADILLO_LIBRARY=${ARMADILLO_LIBRARY}")
endif ()

########################################################################
## Feature registration
########################################################################
sklk_feature("sklk-phy-mod ref design" SKLK_PHY_MOD_ENABLE_REF_DESIGN "Enable sklk-phy-mod ref design" ON "" OFF)
if (NOT SKLK_PHY_MOD_ENABLE_REF_DESIGN)
    return()
endif()

set(MOD_LIB "sklkphy_mod_ref_design")

set(mod_sources
        csi_mod.cpp
        loader.cpp
        schedule_mod.cpp
        utils.cpp
        rpc.cpp
)

add_library(${MOD_LIB} SHARED ${mod_sources})

sklk_phy_mod_mark_mod_library(${MOD_LIB})

target_link_libraries(${MOD_LIB} PRIVATE "-Wl,--no-undefined")

set_target_properties(${MOD_LIB} PROPERTIES SOVERSION ${SKLK_PHY_MOD_ABI_VERSION})
set_target_properties(${MOD_LIB} PROPERTIES VERSION ${SKLK_PHY_MOD_LIBVER})

target_link_libraries(${MOD_LIB} PUBLIC
        json
        json_rpc_cxx
        json_schema_validator
        pthread
        sklkmii
        sklkjson
        sklkphy
        armadillo)

target_include_directories(${MOD_LIB} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_compile_options(${MOD_LIB} PRIVATE -Wno-error) #oai ported headers have warnings
target_compile_options(${MOD_LIB} PRIVATE -Wvla)

install(TARGETS ${MOD_LIB}
        EXPORT SklkPhyModExport
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} # .so file
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} # .lib file
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # .dll file
        )

#export target to project config
install(EXPORT SklkPhyModExport DESTINATION ${CMAKE_LIB_DEST})

########################################################################
## Feature registration
########################################################################
if (SKLK_PHY_MOD_ENABLE_TESTS)
    sklk_phy_mod_add_test(TARGET test_ref_design_loader SOURCES test_loader.cpp LIBRARIES ${MOD_LIB})
endif()
