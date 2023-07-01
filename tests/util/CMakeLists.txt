########################################################################
## Refplat test utility library
########################################################################

add_library(refplat_test INTERFACE)

target_include_directories(refplat_test INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
        refplat_test
        INTERFACE sklkphy-refplat ${CMAKE_DL_LIBS}
)
