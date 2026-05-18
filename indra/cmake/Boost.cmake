# -*- cmake -*-
include_guard()

add_library( ll::boost INTERFACE IMPORTED )
if( USE_CONAN )
  target_link_libraries( ll::boost INTERFACE CONAN_PKG::boost )
  target_compile_definitions( ll::boost INTERFACE BOOST_ALLOW_DEPRECATED_HEADERS BOOST_BIND_GLOBAL_PLACEHOLDERS )
  return()
endif()

# Build Boost from source via FetchContent. On Windows we use 1.86 to match
# the prebuilt colladadom (compiled against 1.86). On other platforms 1.87 is
# fine. Building from source guarantees header/lib consistency and includes all
# symbols (unlike the autobuild prebuilt 1.86 package which was incomplete).
include(FetchContent)

set(BOOST_INCLUDE_LIBRARIES
        asio
        assign
        context
        fiber
        filesystem
        graph
        hof
        iostreams
        json
        program_options
        ptr_container
        random
        regex
        signals2
        stacktrace
        system
        thread
        url
        wave)
set(BOOST_ENABLE_CMAKE ON)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

# Boost's own sources will not compile cleanly with the viewer's strict warning
# flags. Strip /WX (and /W4) for the duration of the FetchContent subbuild and
# restore them afterwards so viewer code stays strict.
# Also force /Zc:wchar_t- so that wchar_t == unsigned short, matching the
# prebuilt colladadom library (libcollada14dom23-s.lib). This ensures the
# mangled boost::filesystem::detail::path_traits::convert symbol matches.
set(_ll_boost_saved_cxx_flags "${CMAKE_CXX_FLAGS}")
set(_ll_boost_saved_c_flags   "${CMAKE_C_FLAGS}")
if (MSVC)
  string(REGEX REPLACE "[/-]WX( |$)"   " "    CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "[/-]W[0-4]( |$)" "/W1 " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "[/-]WX( |$)"   " "    CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}")
  # Force wchar_t = unsigned short to match prebuilt colladadom ABI
  string(REGEX REPLACE "[/-]Zc:wchar_t[ -]" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "[/-]Zc:wchar_t[ -]" " " CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:wchar_t-")
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /Zc:wchar_t-")
else()
  string(REGEX REPLACE "(^| )-Werror( |$)" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "(^| )-Werror( |$)" " " CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}")
endif()

# Use Boost 1.86 on Windows to match prebuilt colladadom (compiled against 1.86).
# Boost 1.87 changed the filesystem ABI (dropped path_traits::convert), which
# causes unresolved external symbols when linking libcollada14dom23-s.lib.
if (WINDOWS)
    set(_ll_boost_version "1.86.0")
    set(_ll_boost_hash "2c5ec5edcdff47ff55e27ed9560b0a0b94b07bd07ed9928b476150e16b0efc57")
else()
    set(_ll_boost_version "1.87.0")
    set(_ll_boost_hash "7da75f171837577a52bbf217e17f8ea576c7c246e4594d617bfde7fafd408be5")
endif()

set(_ll_boost_fetchcontent_args
        Boost
        URL      https://github.com/boostorg/boost/releases/download/boost-${_ll_boost_version}/boost-${_ll_boost_version}-cmake.tar.xz
        URL_HASH SHA256=${_ll_boost_hash}
)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
  list(APPEND _ll_boost_fetchcontent_args DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
endif()
FetchContent_Declare(${_ll_boost_fetchcontent_args})
unset(_ll_boost_fetchcontent_args)
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
        Boost::asio
        Boost::assign
        Boost::context
        Boost::fiber
        Boost::filesystem
        Boost::graph
        Boost::hof
        Boost::iostreams
        Boost::json
        Boost::program_options
        Boost::ptr_container
        Boost::random
        Boost::regex
        Boost::signals2
        Boost::stacktrace
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
