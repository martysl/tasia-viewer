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
        URL      https://github.com/boostorg/boost/releases/download/boost-1.87.0/boost-1.87.0-cmake.tar.xz
        URL_HASH SHA256=7da75f171837577a52bbf217e17f8ea576c7c246e4594d617bfde7fafd408be5
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(Boost)

set(CMAKE_CXX_FLAGS "${_ll_boost_saved_cxx_flags}")
set(CMAKE_C_FLAGS   "${_ll_boost_saved_c_flags}")
unset(_ll_boost_saved_cxx_flags)
unset(_ll_boost_saved_c_flags)

# Mark every Boost target's INTERFACE include dirs as SYSTEM so warnings
# originating inside Boost headers (array-bounds / stringop-overflow / dangling-
# reference false positives on GCC 11+, MSVC C47xx warnings, etc.) do not trip
# the viewer's -Werror / /WX. This mirrors how the old autobuild Boost package
# was consumed (via -isystem) before we switched to FetchContent.
function(_ll_mark_boost_system_includes _dir)
    get_property(_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_t IN LISTS _targets)
        get_target_property(_inc ${_t} INTERFACE_INCLUDE_DIRECTORIES)
        if (_inc)
            set_target_properties(${_t} PROPERTIES
                    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_inc}")
        endif()
    endforeach()
    get_property(_subs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
    foreach(_s IN LISTS _subs)
        _ll_mark_boost_system_includes("${_s}")
    endforeach()
endfunction()
_ll_mark_boost_system_includes("${boost_SOURCE_DIR}")

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

