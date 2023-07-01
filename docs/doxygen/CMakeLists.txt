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


find_package(Doxygen REQUIRED)

set(REF_DESIGN_PATH ${PROJECT_SOURCE_DIR}/ref_design)

#########################################################################
# Doxygen config
#########################################################################

set(DOXYGEN_PROJECT_NAME "Skylark Wireless Modding API")

set(DOXYGEN_GENERATE_LATEX YES)
set(DOXYGEN_EXTRACT_ALL YES)

set(DOXYGEN_INLINE_SIMPLE_STRUCTS YES)
set(DOXYGEN_MACRO_EXPANSION YES)
set(DOXYGEN_SOURCE_BROWSER YES)
set(DOXYGEN_VERBATIM_HEADERS NO)

set(DOXYGEN_STRIP_FROM_PATH
    @SKLK_PHY_HEADER_DIR_PARENT@    # Filled in AddSklkPhyIncludeDir.cmake
    ${PROJECT_SOURCE_DIR}
    pages)

set(DOXYGEN_FILE_PATTERNS "*.dox" "*.cpp" "*.hpp")

set(DOXYGEN_EXAMPLE_PATH ${REF_DESIGN_PATH})
set(DOXYGEN_EXAMPLE_PATTERNS "*.cpp" "*.hpp")

set(DOXYGEN_EXCLUDE_SYMBOLS
    SKLK_PHY_MOD_MESSAGE_QUEUE*         # Helper macros
    sklk_phy_ue sklk_phy_ue_radio sklk_phy_ue_stream sklk_phy_weight_page_id_t)

#########################################################################
# Doxygen target
#########################################################################
set(SKLK_PHY_MOD_DOCS_TARGET sklk-phy-mod-docs)

doxygen_add_docs(
    ${SKLK_PHY_MOD_DOCS_TARGET}
    pages
    ${REF_DESIGN_PATH}
    @SKLK_PHY_HEADER_DIR@/modding.hpp       # Filled in AddSklkPhyIncludeDir.cmake
    @SKLK_PHY_HEADER_DIR@/weight_page.hpp
)

#########################################################################
# HACK: Fill in sklk-phy include directory in Doxyfile
#########################################################################
get_target_property(SKLK_PHY_INCLUDE_DIRS sklkphy INTERFACE_INCLUDE_DIRECTORIES)

set(DOXYFILE "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.${SKLK_PHY_MOD_DOCS_TARGET}")
add_custom_target(add-sklk-phy-include-dir VERBATIM
    COMMAND ${CMAKE_COMMAND} -D "SKLK_PHY_INCLUDE_DIRS=${SKLK_PHY_INCLUDE_DIRS}" -D "DOXYFILE=${DOXYFILE}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/AddSklkPhyIncludeDir.cmake")

add_dependencies(${SKLK_PHY_MOD_DOCS_TARGET} add-sklk-phy-include-dir)
