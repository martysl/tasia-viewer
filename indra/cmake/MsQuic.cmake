# -*- cmake -*-
include_guard(GLOBAL)

include(FetchContent)

set(MSQUIC_GIT_TAG "v2.5.7" CACHE STRING "MsQuic git tag to fetch")

set(QUIC_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)
set(QUIC_BUILD_TEST     OFF CACHE BOOL "" FORCE)
set(QUIC_BUILD_PERF     OFF CACHE BOOL "" FORCE)
set(QUIC_ENABLE_LOGGING OFF CACHE BOOL "" FORCE)
set(QUIC_BUILD_SHARED   OFF CACHE BOOL "" FORCE)

if (WINDOWS)
  set(QUIC_TLS_LIB "schannel" CACHE STRING "" FORCE)
else ()
  set(QUIC_TLS_LIB "quictls" CACHE STRING "" FORCE)
endif ()

FetchContent_Declare(
  msquic
  GIT_REPOSITORY https://github.com/microsoft/msquic.git
  GIT_TAG        ${MSQUIC_GIT_TAG}
  GIT_SUBMODULES_RECURSE TRUE
  GIT_SHALLOW    FALSE
)

# ---------------------------------------------------------------------------
# Populate MsQuic manually so we can patch its CMakeLists.txt before building.
# MsQuic v2.5.7 unconditionally adds /GL (LTCG) to CMAKE_C_FLAGS_RELEASE
# and /LTCG to linker flags. With LTCG enabled, the linker checks all objects
# and libraries for matching MSVC-version stamps and fails with LNK1257 when
# autobuild pre-compiled packages (libpng16.lib) use a different MSVC version
# than the FetchContent build. We strip /GL and /LTCG from the fetched source
# to work around this.
# ---------------------------------------------------------------------------
FetchContent_GetProperties(msquic)
if(NOT msquic_POPULATED)
    FetchContent_Populate(msquic)

    # Patch MsQuic's CMakeLists.txt to remove /GL and /LTCG flags.
    set(_msquic_patch_file "${msquic_SOURCE_DIR}/CMakeLists.txt")
    file(READ "${_msquic_patch_file}" _msquic_content)
    string(REPLACE "/GL " "" _msquic_content "${_msquic_content}")
    string(REPLACE "/LTCG " "" _msquic_content "${_msquic_content}")
    string(REPLACE "/LTCG\"" "" _msquic_content "${_msquic_content}")   # end of string
    file(WRITE "${_msquic_patch_file}" "${_msquic_content}")
endif()

# We need to suppress the /TP global compile option that 00-Common.cmake adds.
# MsQuic is C code and won't compile under C++. Save/restore COMPILE_OPTIONS
# around the subdirectory add.
if(WINDOWS)
    get_directory_property(_msquic_saved_COMPILE_OPTIONS COMPILE_OPTIONS)
    set_directory_properties(PROPERTIES COMPILE_OPTIONS "")
    add_compile_options(/TC)
endif()

add_subdirectory(${msquic_SOURCE_DIR} ${msquic_BINARY_DIR})

if(WINDOWS)
    set_directory_properties(PROPERTIES COMPILE_OPTIONS "")
    add_compile_options(${_msquic_saved_COMPILE_OPTIONS})
endif()

if (UNIX AND NOT APPLE)
    find_program(MSQUIC_NM_EXE nm)
    if (NOT MSQUIC_NM_EXE)
        message(FATAL_ERROR "MsQuic: 'nm' is required to localize bundled OpenSSL symbols")
    endif()

    set(_msquic_marker  "${msquic_BINARY_DIR}/msquic_localized.stamp")
    set(_msquic_archive "${msquic_BINARY_DIR}/bin/$<IF:$<CONFIG:Debug>,Debug,Release>/libmsquic.a")

    add_custom_command(
        OUTPUT  "${_msquic_marker}"
        COMMAND ${CMAKE_COMMAND}
                -DMSQUIC_AR_PATH=${_msquic_archive}
                -DAR=${CMAKE_AR}
                -DNM=${MSQUIC_NM_EXE}
                -DOBJCOPY=${CMAKE_OBJCOPY}
                -DMARKER=${_msquic_marker}
                -P "${CMAKE_CURRENT_LIST_DIR}/MsQuicLocalize.cmake"
        DEPENDS msquic_lib "${CMAKE_CURRENT_LIST_DIR}/MsQuicLocalize.cmake"
        COMMENT "Renaming bundled quictls/OpenSSL symbols inside libmsquic.a"
        VERBATIM)

    add_custom_target(msquic_localized ALL DEPENDS "${_msquic_marker}")
    add_dependencies(msquic msquic_localized)
endif ()

add_library(fs::msquic INTERFACE IMPORTED)
target_link_libraries(fs::msquic INTERFACE msquic msquic::base_link)
target_include_directories(fs::msquic SYSTEM INTERFACE
    ${msquic_SOURCE_DIR}/src/inc)
