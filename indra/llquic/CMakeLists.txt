# -*- cmake -*-

project(llquic)

include(00-Common)
include(LLCommon)
include(Linking)
include(MsQuic)

set(llquic_SOURCE_FILES
    llquicapi.cpp
    llquicregistration.cpp
    llquicconfiguration.cpp
    llquicstream.cpp
    llquicconnection.cpp
    llquicglobal.cpp
    )

set(llquic_HEADER_FILES
    CMakeLists.txt
    llquicapi.h
    llquicregistration.h
    llquicconfiguration.h
    llquicstream.h
    llquicconnection.h
    llquicglobal.h
    )

set_source_files_properties(${llquic_HEADER_FILES}
    PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llquic_SOURCE_FILES ${llquic_HEADER_FILES})

add_library(llquic ${llquic_SOURCE_FILES})
target_include_directories(llquic INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(llquic
    llcommon
    fs::msquic
    )

if (WINDOWS)
    add_custom_command(
        OUTPUT ${SHARED_LIB_STAGING_DIR}/msquic.dll
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHARED_LIB_STAGING_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:msquic> ${SHARED_LIB_STAGING_DIR}/msquic.dll
        DEPENDS msquic
        COMMENT "Copying msquic.dll to shared libraries staging directory"
        VERBATIM)
    add_custom_target(copy_msquic_dll ALL DEPENDS ${SHARED_LIB_STAGING_DIR}/msquic.dll)
    add_dependencies(llquic copy_msquic_dll)
endif ()
