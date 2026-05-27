find_path(CR_INCLUDE_DIRS "cr.h")

file(GLOB_RECURSE SRC_FILES
        CONFIGURE_DEPENDS
        Src/*.cpp
        Src/*.c
        Src/*.hpp
        Src/*.h
)

add_library(NoPause SHARED ${SRC_FILES})

set_target_properties(NoPause PROPERTIES PREFIX "" OUTPUT_NAME NoPause)

target_include_directories(NoPause PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
        ${CR_INCLUDE_DIRS}
)

target_link_libraries(NoPause PRIVATE ZKntSdkInterface imgui::imgui)
