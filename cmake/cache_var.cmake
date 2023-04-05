# Check if mmkv is being used directly or via add_subdirectory, but allow overriding
if(NOT DEFINED MMKV_MAIN_PROJECT)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(MMKV_MAIN_PROJECT ON)
    else()
        set(MMKV_MAIN_PROJECT OFF)
    endif()
endif()

option(MMKV_BUILD_STATIC_LIBS "Build static library of mmkv" ON)
# to install files to proper destination actually.
option(MMKV_INSTALL "Generate the install target" ${MMKV_MAIN_PROJECT})

option(MMKV_TESTS "Generate mmkv test targets" OFF)

# User can determine whether to build all tests when build target all
# e.g. cmake --build */MMKV/build [--target all -j 2]
# If this option is OFF, user should specify target manually.
option(MMKV_BUILD_ALL_TESTS "Build tests when --target all(default) is specified" OFF)
