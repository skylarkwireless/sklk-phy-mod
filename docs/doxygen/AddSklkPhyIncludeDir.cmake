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
