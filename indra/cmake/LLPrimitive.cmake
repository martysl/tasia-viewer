# -*- cmake -*-

# these should be moved to their own cmake file
include(Prebuilt)
include(Linking)
include(Boost)

include_guard()

add_library( ll::minizip-ng INTERFACE IMPORTED )
add_library( ll::libxml INTERFACE IMPORTED )
add_library( ll::colladadom INTERFACE IMPORTED )

# ND, needs fixup in collada conan pkg
if( USE_CONAN )
  target_include_directories( ll::colladadom SYSTEM INTERFACE
    "${CONAN_INCLUDE_DIRS_COLLADADOM}/collada-dom/"
    "${CONAN_INCLUDE_DIRS_COLLADADOM}/collada-dom/1.4/" )
endif()

use_system_binary( colladadom )

use_prebuilt_binary(colladadom)
use_prebuilt_binary(minizip-ng) # needed for colladadom
use_prebuilt_binary(libxml2)

# Use find_library for cross-platform support (Apple Silicon compat).
# Misfitz fix: use the same colladadom + boost pattern across all platforms
# instead of platform-specific blocks that could miss ll::boost on Windows.
find_library(MINIZIPNG_LIBRARY
    NAMES
    minizip.lib
    libminizip.a
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)
target_link_libraries(ll::minizip-ng INTERFACE ${MINIZIPNG_LIBRARY})

find_library(LIBXML2_LIBRARY
    NAMES
    libxml2.lib
    libxml2.a
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)
target_link_libraries(ll::libxml INTERFACE ${LIBXML2_LIBRARY})

if (WINDOWS)
    target_link_libraries( ll::libxml INTERFACE Bcrypt.lib)
endif()

target_include_directories( ll::colladadom SYSTEM INTERFACE
        ${LIBS_PREBUILT_DIR}/include/collada
        ${LIBS_PREBUILT_DIR}/include/collada/1.4
        )

find_library(COLLADADOM_LIBRARY
    NAMES
    libcollada14dom23-s.lib
    collada14dom
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

# ll::boost is required on all platforms because the prebuilt colladadom
# references boost::filesystem symbols (path_traits::convert, etc.).
target_link_libraries(ll::colladadom INTERFACE
        ${COLLADADOM_LIBRARY}
        Boost::filesystem
        Boost::system
        ll::boost
        ll::libxml
        ll::minizip-ng)

# <FS> GLIB uses pcre, so we need to keep it for Linux builds
if (LINUX)
   add_library( ll::pcre INTERFACE IMPORTED )
   use_prebuilt_binary(pcre)
   target_link_libraries( ll::pcre INTERFACE pcrecpp pcre )
endif ()
# </FS>
