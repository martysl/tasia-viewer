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

set(_msquic_saved_C_FLAGS_DEBUG          "${CMAKE_C_FLAGS_DEBUG}")
set(_msquic_saved_C_FLAGS_MINSIZEREL     "${CMAKE_C_FLAGS_MINSIZEREL}")
set(_msquic_saved_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
set(_msquic_saved_C_FLAGS_RELEASE        "${CMAKE_C_FLAGS_RELEASE}")
set(_msquic_saved_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}")
set(_msquic_saved_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL}")
set(_msquic_saved_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
set(_msquic_saved_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}")

# MsQuic C code does not compile under C++ (/TP forced by 00-Common.cmake).
# Save the global /TP compile option and replace with /TC for this sub-build.
if(WINDOWS)
    get_directory_property(_msquic_saved_COMPILE_OPTIONS COMPILE_OPTIONS)
    set_directory_properties(PROPERTIES COMPILE_OPTIONS "")
    add_compile_options(/TC)
endif()

FetchContent_MakeAvailable(msquic)

if(WINDOWS)
    set_directory_properties(PROPERTIES COMPILE_OPTIONS "")
    add_compile_options(${_msquic_saved_COMPILE_OPTIONS})
endif()

set(CMAKE_C_FLAGS_DEBUG          "${_msquic_saved_C_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_MINSIZEREL     "${_msquic_saved_C_FLAGS_MINSIZEREL}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${_msquic_saved_C_FLAGS_RELWITHDEBINFO}")
set(CMAKE_C_FLAGS_RELEASE        "${_msquic_saved_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG          "${_msquic_saved_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "${_msquic_saved_CXX_FLAGS_MINSIZEREL}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${_msquic_saved_CXX_FLAGS_RELWITHDEBINFO}")
set(CMAKE_CXX_FLAGS_RELEASE        "${_msquic_saved_CXX_FLAGS_RELEASE}")

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
