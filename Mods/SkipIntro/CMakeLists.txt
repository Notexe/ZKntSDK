find_path(CR_INCLUDE_DIRS "cr.h")

file(GLOB_RECURSE SRC_FILES
        CONFIGURE_DEPENDS
        Src/*.cpp
        Src/*.c
        Src/*.hpp
        Src/*.h
)

add_library(SkipIntro SHARED ${SRC_FILES})

set_target_properties(SkipIntro PROPERTIES PREFIX "" OUTPUT_NAME SkipIntro)

target_include_directories(SkipIntro PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
        ${CR_INCLUDE_DIRS}
)

target_link_libraries(SkipIntro PRIVATE ZKntSdkInterface imgui::imgui)
