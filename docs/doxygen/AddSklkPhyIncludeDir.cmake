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


#########################################################################
# Required arguments
#########################################################################
set(_REQUIRED_ARGUMENTS
        SKLK_PHY_INCLUDE_DIRS
        DOXYFILE)

foreach (ARG_NAME IN LISTS _REQUIRED_ARGUMENTS)
    if (NOT ${ARG_NAME})
        message(FATAL_ERROR "Undefined argument ${ARG_NAME}")
    endif ()
endforeach ()

#########################################################################
# Variables declared:
#   SKLK_PHY_HEADER_DIR
#   SKLK_PHY_HEADER_DIR_PARENT
#########################################################################
find_path(SKLK_PHY_HEADER_DIR modding.hpp PATHS ${SKLK_PHY_INCLUDE_DIRS} PATH_SUFFIXES sklkphy NO_DEFAULT_PATH NO_CACHE)

if (NOT SKLK_PHY_HEADER_DIR)
    message(FATAL_ERROR "Could not find sklk-phy header directory")
endif ()

get_filename_component(SKLK_PHY_HEADER_DIR_PARENT ${SKLK_PHY_HEADER_DIR} DIRECTORY)

#########################################################################
# Write variables to Doxyfile
#########################################################################
configure_file("${DOXYFILE}" "${DOXYFILE}" @ONLY)
