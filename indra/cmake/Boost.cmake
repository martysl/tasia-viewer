# -*- cmake -*-
include_guard()

add_library( ll::boost INTERFACE IMPORTED )
if( USE_CONAN )
  target_link_libraries( ll::boost INTERFACE CONAN_PKG::boost )
  target_compile_definitions( ll::boost INTERFACE BOOST_ALLOW_DEPRECATED_HEADERS BOOST_BIND_GLOBAL_PLACEHOLDERS )
  return()
endif()

# Build Boost from source via FetchContent. The autobuild Windows prebuilt for
# boost 1.86 is missing several Boost.Filesystem symbols that Boost.Wave's
# templates need (path_traits::convert, path_algorithms::append_v3, ...), so
# we build all of Boost ourselves to guarantee header/lib consistency on every
# platform.
include(FetchContent)

set(BOOST_INCLUDE_LIBRARIES
        context
        fiber
        filesystem
        program_options
        regex
        system
        thread
        url
        wave)
set(BOOST_ENABLE_CMAKE ON)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

# Boost's own sources will not compile cleanly with the viewer's strict warning
# flags. Strip /WX (and /W4) for the duration of the FetchContent subbuild and
# restore them afterwards so viewer code stays strict.
set(_ll_boost_saved_cxx_flags "${CMAKE_CXX_FLAGS}")
set(_ll_boost_saved_c_flags   "${CMAKE_C_FLAGS}")
if (MSVC)
  string(REGEX REPLACE "[/-]WX( |$)"   " "    CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "[/-]W[0-4]( |$)" "/W1 " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "[/-]WX( |$)"   " "    CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}")
else()
  string(REGEX REPLACE "(^| )-Werror( |$)" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "(^| )-Werror( |$)" " " CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}")
endif()

FetchContent_Declare(
        Boost
        URL      https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.tar.xz
        URL_HASH SHA256=2c5ec5edcdff47ff55e27ed9560b0a0b94b07bd07ed9928b476150e16b0efc57
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(Boost)

# Boost 1.86 bug: libs/context/include/boost/context/{fiber,continuation}_fcontext.hpp
# unconditionally define __NR_map_shadow_stack = 451 whenever __CET__ && __unix__
# are set. Modern glibc / kernel headers (GCC 15, linux-headers >= 6.6) already
# provide this macro with value 453, so the boost define triggers a redefinition
# warning that becomes fatal under the viewer's -Werror. Fixed upstream in
# Boost 1.87. Patch both headers idempotently to add an #ifndef guard.
foreach(_ctx_hdr fiber_fcontext.hpp continuation_fcontext.hpp)
    set(_hdr "${boost_SOURCE_DIR}/libs/context/include/boost/context/${_ctx_hdr}")
    if (EXISTS "${_hdr}")
        file(READ "${_hdr}" _content)
        string(REPLACE
                "#  define __NR_map_shadow_stack 451"
                "#  ifndef __NR_map_shadow_stack\n#    define __NR_map_shadow_stack 451\n#  endif"
                _patched "${_content}")
        if (NOT "${_patched}" STREQUAL "${_content}")
            file(WRITE "${_hdr}" "${_patched}")
            message(STATUS "Patched Boost ${_ctx_hdr} for __NR_map_shadow_stack redefinition")
        endif()
    endif()
endforeach()

set(CMAKE_CXX_FLAGS "${_ll_boost_saved_cxx_flags}")
set(CMAKE_C_FLAGS   "${_ll_boost_saved_c_flags}")
unset(_ll_boost_saved_cxx_flags)
unset(_ll_boost_saved_c_flags)

target_link_libraries( ll::boost INTERFACE
        Boost::context
        Boost::fiber
        Boost::filesystem
        Boost::program_options
        Boost::regex
        Boost::system
        Boost::thread
        Boost::url
        Boost::wave)

target_compile_definitions( ll::boost INTERFACE
        BOOST_ALLOW_DEPRECATED_HEADERS
        BOOST_BIND_GLOBAL_PLACEHOLDERS)

if (LINUX)
    set(BOOST_SYSTEM_LIBRARY ${BOOST_SYSTEM_LIBRARY} rt)
    set(BOOST_THREAD_LIBRARY ${BOOST_THREAD_LIBRARY} rt)
endif (LINUX)

