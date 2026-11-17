include_guard(GLOBAL)
set(OSDK_STOY_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}")

# Prefer the vendored ./OSDK.toolchain, then an explicit OSDK_ROOT /
# exported OSDK, then an installed sample tree.  (The repo-local
# toolchain is the source of truth; a stale $OSDK export must not
# shadow it - it silently broke the include path on reconfigure.)
if(NOT DEFINED OSDK_ROOT)
    get_filename_component(_osdk_local "${OSDK_STOY_MODULE_DIR}/OSDK.toolchain" ABSOLUTE)
    if(EXISTS "${_osdk_local}/bin/compiler")
        set(OSDK_ROOT "${_osdk_local}")
    elseif(DEFINED ENV{OSDK} AND NOT "$ENV{OSDK}" STREQUAL "")
        set(OSDK_ROOT "$ENV{OSDK}")
    else()
        get_filename_component(_osdk_toy_dir "${OSDK_STOY_MODULE_DIR}" DIRECTORY)
        get_filename_component(_osdk_installed_root "${_osdk_toy_dir}" DIRECTORY)
        if(EXISTS "${_osdk_installed_root}/bin/compiler")
            set(OSDK_ROOT "${_osdk_installed_root}")
        else()
            message(FATAL_ERROR "Set OSDK_ROOT or the OSDK environment variable to an installed OSDK directory")
        endif()
    endif()
endif()

file(TO_CMAKE_PATH "${OSDK_ROOT}" OSDK_ROOT)
find_program(OSDK_CPP NAMES clang cc gcc cpp REQUIRED)

function(osdk_add_stoy)
    cmake_parse_arguments(ARG "" "NAME;ADDRESS;TAP_NAME;DISK_NAME;PICTURE;PICTURE_MODE;PICTURE_OUTPUT" "SOURCES;COMPILER_FLAGS;LINK_FLAGS;XA_FLAGS" ${ARGN})
    if(NOT ARG_NAME OR NOT ARG_SOURCES)
        message(FATAL_ERROR "osdk_add_stoy requires NAME and SOURCES")
    endif()
    if(NOT ARG_ADDRESS)
        set(ARG_ADDRESS "$600")
    endif()
    if(NOT ARG_TAP_NAME)
        set(ARG_TAP_NAME "${ARG_NAME}")
    endif()

    set(_sources)
    set(_dependencies)
    foreach(_source IN LISTS ARG_SOURCES)
        set(_source_path "${CMAKE_CURRENT_SOURCE_DIR}/${_source}")
        list(APPEND _sources "${_source_path}")
        if(EXISTS "${_source_path}")
            list(APPEND _dependencies "${_source_path}")
        endif()
    endforeach()

    set(_tap "${CMAKE_CURRENT_BINARY_DIR}/build/${ARG_NAME}.tap")

    add_custom_command(
        OUTPUT "${_tap}"
        COMMAND "${CMAKE_COMMAND}"
            "-DOSDK_ROOT=${OSDK_ROOT}"
            "-DOSDK_CPP=${OSDK_CPP}"
            "-DSAMPLE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            "-DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/build"
            "-DNAME=${ARG_NAME}"
            "-DTAP_NAME=${ARG_TAP_NAME}"
            "-DADDRESS=${ARG_ADDRESS}"
            "-DSOURCES=${_sources}"
            "-DCOMPILER_FLAGS=${ARG_COMPILER_FLAGS}"
            "-DLINK_FLAGS=${ARG_LINK_FLAGS}"
            "-DXA_FLAGS=${ARG_XA_FLAGS}"
            "-DPICTURE=${ARG_PICTURE}"
            "-DPICTURE_MODE=${ARG_PICTURE_MODE}"
            "-DPICTURE_OUTPUT=${ARG_PICTURE_OUTPUT}"
            "-DDISK_NAME=${ARG_DISK_NAME}"
            -P "${OSDK_STOY_MODULE_DIR}/OSDKStoyBuild.cmake"
        DEPENDS ${_dependencies} "${OSDK_STOY_MODULE_DIR}/OSDKStoyBuild.cmake"
        COMMENT "Building ${ARG_NAME}.tap with OSDK"
        VERBATIM
    )

    add_custom_target(${ARG_NAME} ALL DEPENDS "${_tap}")

    if(ARG_DISK_NAME)
        add_custom_target(${ARG_NAME}_dsk ALL)
        add_dependencies(${ARG_NAME}_dsk ${ARG_NAME})
    endif()

    # ------------------------------------------------------------------
    # Automatically add a Run_<NAME> target
    # ------------------------------------------------------------------
    # Preferred location of the Oricutron binary (as used in your builds)
    if(WIN32)
        set(_oricutron_bin "${CMAKE_CURRENT_BINARY_DIR}/Oricutron/Oricutron.exe")
    else()
        set(_oricutron_bin "${CMAKE_CURRENT_BINARY_DIR}/Oricutron/Oricutron")
    endif()

    # Fallback: search on PATH if the local binary does not exist yet
    if(NOT EXISTS "${_oricutron_bin}")
        find_program(_oricutron_bin NAMES oricutron Oricutron)
    endif()

    if(_oricutron_bin)
        add_custom_target(Run_${ARG_NAME}
            COMMAND "${_oricutron_bin}" "${_tap}"
            DEPENDS ${ARG_NAME}
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            COMMENT "Running ${ARG_NAME} in Oricutron"
            USES_TERMINAL
            VERBATIM
        )

        # Optional: nicer grouping in IDEs
        set_target_properties(${ARG_NAME} PROPERTIES FOLDER "Stoys")
        set_target_properties(Run_${ARG_NAME} PROPERTIES FOLDER "Stoys/Run")

        message(STATUS "Stoy ${ARG_NAME}: added target Run_${ARG_NAME}")
    else()
        message(WARNING "osdk_add_stoy(${ARG_NAME}): Oricutron not found – Run_${ARG_NAME} target skipped")
    endif()
endfunction()
