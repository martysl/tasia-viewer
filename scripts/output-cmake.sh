#!/bin/bash

CMAKE_FILE="../CMakeLists.txt"
rm -f ${CMAKE_FILE}
echo "cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)" >> $CMAKE_FILE
echo "project(ViewerRoot)" >> $CMAKE_FILE

write_set_if_not_defined() {
    local var_name="$1"
    local var_value="$2"
    echo "if(NOT DEFINED ${var_name} OR ${var_name} STREQUAL \"\")" >> $CMAKE_FILE
    echo "    set(${var_name} \"${var_value}\")" >> $CMAKE_FILE
    echo "endif()" >> $CMAKE_FILE
}

write_set_if_not_defined "ENV{AUTOBUILD}" "$AUTOBUILD"
write_set_if_not_defined "ENV{LL_BUILD_RELEASE}" "$LL_BUILD_RELEASE"
write_set_if_not_defined "ENV{LL_BUILD_RELWITHDEBINFO}" "$LL_BUILD_RELWITHDEBINFO"
write_set_if_not_defined "ENV{LL_BUILD_DEBUG}" "$LL_BUILD_DEBUG"
write_set_if_not_defined "ENV{LL_BUILD}" "$LL_BUILD"
write_set_if_not_defined "ENV{revision}" "$revision"
write_set_if_not_defined "ENV{AUTOBUILD_BUILD_ID}" "$AUTOBUILD_BUILD_ID"
write_set_if_not_defined "ENV{PKG_CONFIG_LIBDIR}" "$PKG_CONFIG_LIBDIR"
write_set_if_not_defined "ENV{PKG_CONFIG_PATH}" "$PKG_CONFIG_PATH"
write_set_if_not_defined "ENV{PYTHON}" "$PYTHON"
write_set_if_not_defined "ENV{LL_BUILD_DARWIN_RELEASE_SWITCHES}" "$LL_BUILD_DARWIN_RELEASE_SWITCHES"
write_set_if_not_defined "ENV{LL_BUILD_WINDOWS_BASE}" "$LL_BUILD_WINDOWS_BASE"
write_set_if_not_defined "ENV{AUTOBUILD_PLATFORM}" "$AUTOBUILD_PLATFORM"
write_set_if_not_defined "ENV{AUTOBUILD_LOGLEVEL}" "$AUTOBUILD_LOGLEVEL"
write_set_if_not_defined "ENV{LL_BUILD_BASE_MACROS}" "$LL_BUILD_BASE_MACROS"
write_set_if_not_defined "ENV{LL_BUILD_DARWIN_BASE_SWITCHES}" "$LL_BUILD_DARWIN_BASE_SWITCHES"
write_set_if_not_defined "ENV{LL_BUILD_DARWIN_BASE}" "$LL_BUILD_DARWIN_BASE"
write_set_if_not_defined "ENV{LL_BUILD_DARWIN_RELEASE_MACROS}" "$LL_BUILD_DARWIN_RELEASE_MACROS"
write_set_if_not_defined "ENV{LL_BUILD_LINUX_RELEASEFS_OPEN}" "$LL_BUILD_LINUX_RELEASEFS_OPEN"
write_set_if_not_defined "ENV{LL_BUILD_WINDOWS_RELWITHDEBINFOFS}" "$LL_BUILD_WINDOWS_RELWITHDEBINFOFS"
write_set_if_not_defined "ENV{LL_BUILD_RELWITHDEBINFO_SWITCHES}" "$LL_BUILD_RELWITHDEBINFO_SWITCHES"
write_set_if_not_defined "ENV{AUTOBUILD_CPU_COUNT}" "$AUTOBUILD_CPU_COUNT"
write_set_if_not_defined "ENV{LL_BUILD_WINDOWS_RELEASEFS}" "$LL_BUILD_WINDOWS_RELEASEFS"
write_set_if_not_defined "ENV{LL_BUILD_DARWIN_RELEASEFS}" "$LL_BUILD_DARWIN_RELEASEFS"
write_set_if_not_defined "ENV{LL_BUILD_LINUX_RELWITHDEBINFOOS}" "$LL_BUILD_LINUX_RELWITHDEBINFOOS"
write_set_if_not_defined "ENV{LL_BUILD_WINDOWS_BASE_MACROS}" "$LL_BUILD_WINDOWS_BASE_MACROS"
write_set_if_not_defined "ENV{LL_BUILD_DARWIN_RELEASEFS_OPEN}" "$LL_BUILD_DARWIN_RELEASEFS_OPEN"
write_set_if_not_defined "ENV{AUTOBUILD_ADDRSIZE}" "$AUTOBUILD_ADDRSIZE"

echo $CMAKE_ARGS

for arg in $CMAKE_ARGS; do
    if [[ $arg == -D* ]]; then
        key_value="${arg:2}"

        if [[ $key_value == *"="* ]]; then
            IFS='=' read -r key value <<< "$key_value"
            key="${key%%:*}"
            value="${value//\"/\\\"}"

            # Write CMake variable only if not already defined
            write_set_if_not_defined "$key" "$value"
        else
            key="${key_value%%:*}"
            echo "if(NOT DEFINED ${key//\"/\\\"})" >> $CMAKE_FILE
            echo "    set(${key//\"/\\\"})" >> $CMAKE_FILE
            echo "endif()" >> $CMAKE_FILE
        fi
    fi
done

echo "set(VENV_PATH \"\" CACHE PATH \"Path to the Python virtual environment if required\")" >> $CMAKE_FILE
echo "if(VENV_PATH AND EXISTS \"\${VENV_PATH}/bin/activate\")" >> $CMAKE_FILE
echo "    message(STATUS \"Using Python virtual environment at: \${VENV_PATH}\")" >> $CMAKE_FILE
echo "    set(ENV{VIRTUAL_ENV} \"\${VENV_PATH}\")" >> $CMAKE_FILE
echo "    set(ENV{PATH} \"\${VENV_PATH}/bin:\$ENV{PATH}\")" >> $CMAKE_FILE
echo "    execute_process(" >> $CMAKE_FILE
echo "            COMMAND \${VENV_PATH}/bin/python -c \"import sys; print('python{}.{}'.format(*sys.version_info[:2]))\"" >> $CMAKE_FILE
echo "            OUTPUT_VARIABLE python_version" >> $CMAKE_FILE
echo "            OUTPUT_STRIP_TRAILING_WHITESPACE" >> $CMAKE_FILE
echo "    )" >> $CMAKE_FILE
echo "    set(ENV{PYTHONPATH} \"\${VENV_PATH}/lib/\${python_version}/site-packages:\$ENV{PYTHONPATH}\")" >> $CMAKE_FILE
echo "    find_package(Python3 REQUIRED)" >> $CMAKE_FILE
echo "    set(Python3_EXECUTABLE \"\${VENV_PATH}/bin/python\")" >> $CMAKE_FILE
echo "else()" >> $CMAKE_FILE
echo "    message(STATUS \"No valid virtual environment specified or found.\")" >> $CMAKE_FILE
echo "endif()" >> $CMAKE_FILE

echo "if(CMAKE_BUILD_TYPE STREQUAL \"Debug\")" >> $CMAKE_FILE
echo "    add_compile_options(-Og -fsanitize=address)" >> $CMAKE_FILE
echo "    add_link_options(-fsanitize=address)" >> $CMAKE_FILE
echo "endif()" >> $CMAKE_FILE

echo "add_subdirectory(indra)" >> $CMAKE_FILE
echo "file(WRITE \"\${CMAKE_CURRENT_LIST_DIR}/revision.txt\" \"\$ENV{revision}\n\")" >> $CMAKE_FILE

echo "add_custom_target(copy_ca_bundle ALL" >> $CMAKE_FILE
echo "        COMMAND \${CMAKE_COMMAND} -E copy" >> $CMAKE_FILE
echo "        \"\${CMAKE_BINARY_DIR}/indra/newview/packaged/bin/ca-bundle.crt\"" >> $CMAKE_FILE
echo "        \"\${CMAKE_BINARY_DIR}/indra/newview/ca-bundle.crt\"" >> $CMAKE_FILE
echo "        COMMENT \"Copying ca-bundle.crt post-build\"" >> $CMAKE_FILE
echo "        VERBATIM" >> $CMAKE_FILE
echo ")" >> $CMAKE_FILE
echo "" >> $CMAKE_FILE
echo "add_dependencies(copy_ca_bundle copy_l_viewer_manifest)" >> $CMAKE_FILE