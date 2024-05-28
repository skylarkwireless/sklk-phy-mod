########################################################################
## Refplat test utility library
########################################################################
add_library(refplat_test STATIC refplat_test.cpp)

target_include_directories(refplat_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    refplat_test
    PUBLIC sklkphy-refplat sklkcpptest ${CMAKE_DL_LIBS}
)
