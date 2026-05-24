add_subdirectory(pixmaps ${DARKTABLE_DATADIR}/pixmaps)
add_subdirectory(styles ${DARKTABLE_DATADIR}/styles)

if(USE_OPENCL)
	add_subdirectory(kernels ${DARKTABLE_DATADIR}/kernels)
endif(USE_OPENCL)

FILE(GLOB THEME_FILES "themes/*.css")
FILE(COPY ${THEME_FILES} DESTINATION "${DARKTABLE_DATADIR}/themes")
install(FILES ${THEME_FILES} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable/themes COMPONENT DTApplication)

#
# web gallery export support files:
#
FILE(GLOB WEB_FILES "style/*")
install(FILES ${WEB_FILES} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable/style COMPONENT DTApplication)

install(DIRECTORY "pswp" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable COMPONENT DTApplication)

#
# latex book template support files:
#
FILE(GLOB WEB_FILES "latex/*")
install(FILES ${WEB_FILES} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable/latex COMPONENT DTApplication)

if(USE_LUA)
#
# lua system scripts
#
FILE(COPY lua luarc DESTINATION "${DARKTABLE_DATADIR}")
install(DIRECTORY "lua" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable COMPONENT DTApplication)
install(FILES luarc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable COMPONENT DTApplication)
endif(USE_LUA)

#
# Install (and generate when necessary) other system shares
#
if(NOT WIN32)
  if("avif" IN_LIST DT_SUPPORTED_EXTENSIONS)
    list(APPEND DESKTOP_MIME_TYPES image/avif)
  endif()
  if("cr3" IN_LIST DT_SUPPORTED_EXTENSIONS)
    list(APPEND DESKTOP_MIME_TYPES image/x-canon-cr3)
  endif()
  if("exr" IN_LIST DT_SUPPORTED_EXTENSIONS)
    list(APPEND DESKTOP_MIME_TYPES image/x-exr image/aces)
  endif()
  if("heif" IN_LIST DT_SUPPORTED_EXTENSIONS)
    list(APPEND DESKTOP_MIME_TYPES image/heif image/heic)
  endif()
  if("hej2" IN_LIST DT_SUPPORTED_EXTENSIONS)
    list(APPEND DESKTOP_MIME_TYPES image/hej2k)
  endif()
  if("avci" IN_LIST DT_SUPPORTED_EXTENSIONS)
    list(APPEND DESKTOP_MIME_TYPES image/avci)
  endif()
  if("jp2" IN_LIST DT_SUPPORTED_EXTENSIONS)
    list(APPEND DESKTOP_MIME_TYPES image/jp2)
  endif()
  if("jxl" IN_LIST DT_SUPPORTED_EXTENSIONS)
    list(APPEND DESKTOP_MIME_TYPES image/jxl)
  endif()
  if("webp" IN_LIST DT_SUPPORTED_EXTENSIONS)
    list(APPEND DESKTOP_MIME_TYPES image/webp)
  endif()
  if("qoi" IN_LIST DT_SUPPORTED_EXTENSIONS)
    list(APPEND DESKTOP_MIME_TYPES image/qoi)
  endif()
  if("fits" IN_LIST DT_SUPPORTED_EXTENSIONS)
    list(APPEND DESKTOP_MIME_TYPES image/fits)
  endif()
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/org.darktable.darktable.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/org.darktable.darktable.desktop.in)
  file(GLOB PO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../po/*.po")
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/org.darktable.darktable.desktop
    COMMAND sh -c "${intltool_merge_BIN} --desktop-style \"${CMAKE_CURRENT_SOURCE_DIR}/../po\" \"${CMAKE_CURRENT_BINARY_DIR}/org.darktable.darktable.desktop.in\" \"${CMAKE_CURRENT_BINARY_DIR}/org.darktable.darktable.desktop\""
    MAIN_DEPENDENCY ${CMAKE_CURRENT_BINARY_DIR}/org.darktable.darktable.desktop.in
    DEPENDS ${PO_FILES}
    )
  add_custom_target(darktable.desktop_file ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.darktable.darktable.desktop)
  if(${VALIDATE_DESKTOP_FILE})
    add_custom_target(
      validate_darktable_desktop ALL
      COMMAND ${desktop_file_validate_BIN} --warn-kde ${CMAKE_CURRENT_BINARY_DIR}/org.darktable.darktable.desktop
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.darktable.darktable.desktop
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
      COMMENT "Checking validity of org.darktable.darktable.desktop"
      )
    add_dependencies(darktable.desktop_file validate_darktable_desktop)
  endif()
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.darktable.darktable.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications COMPONENT DTApplication)

  if(${VALIDATE_APPDATA_FILE})
    add_custom_command(
      OUTPUT ${DARKTABLE_SHAREDIR}/metainfo/org.darktable.darktable.appdata.xml
      SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/org.darktable.darktable.appdata.xml.in
      COMMAND ${CMAKE_COMMAND} -E make_directory ${DARKTABLE_SHAREDIR}/metainfo
      COMMAND sh -c "${intltool_merge_BIN} --xml-style \"${CMAKE_CURRENT_SOURCE_DIR}/../po\" \"${CMAKE_CURRENT_SOURCE_DIR}/org.darktable.darktable.appdata.xml.in\" \"${DARKTABLE_SHAREDIR}/metainfo/org.darktable.darktable.appdata.xml\""
      COMMAND ${appstream_util_BIN} validate --nonet ${DARKTABLE_SHAREDIR}/metainfo/org.darktable.darktable.appdata.xml
      MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/org.darktable.darktable.appdata.xml.in
      DEPENDS ${PO_FILES}
    )
  else()
    add_custom_command(
      OUTPUT ${DARKTABLE_SHAREDIR}/metainfo/org.darktable.darktable.appdata.xml
      COMMAND ${CMAKE_COMMAND} -E make_directory ${DARKTABLE_SHAREDIR}/metainfo
      COMMAND sh -c "${intltool_merge_BIN} --xml-style \"${CMAKE_CURRENT_SOURCE_DIR}/../po\" \"${CMAKE_CURRENT_SOURCE_DIR}/org.darktable.darktable.appdata.xml.in\" \"${DARKTABLE_SHAREDIR}/metainfo/org.darktable.darktable.appdata.xml\""
      MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/org.darktable.darktable.appdata.xml.in
      DEPENDS ${PO_FILES}
    )
  endif()

  add_custom_target(darktable.appdata_file ALL DEPENDS ${DARKTABLE_SHAREDIR}/metainfo/org.darktable.darktable.appdata.xml)

  install(FILES ${DARKTABLE_SHAREDIR}/metainfo/org.darktable.darktable.appdata.xml DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo COMPONENT DTApplication)
endif(NOT WIN32)

#
# Install watermarks
#
FILE(GLOB WATERMARKS "watermarks/*.svg")
FILE(COPY ${WATERMARKS} DESTINATION "${DARKTABLE_DATADIR}/watermarks")
install(FILES ${WATERMARKS} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable/watermarks COMPONENT DTApplication)

#
# Install gdb command file for backtrace generation
#
FILE(COPY gdb_commands DESTINATION "${DARKTABLE_DATADIR}")
install(FILES gdb_commands DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable COMPONENT DTApplication)

#
# Install shortcutsrc with extra key+mouse combinations, and any localized versions of it
#
FILE(GLOB LOCALIZED_SHORTCUTS "shortcutsrc.*")
FILE(COPY shortcutsrc ${LOCALIZED_SHORTCUTS} DESTINATION "${DARKTABLE_DATADIR}")
install(FILES shortcutsrc ${LOCALIZED_SHORTCUTS} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable COMPONENT DTApplication)

#
# Install noiseprofiles
#
if(${VALIDATE_JSON})
  add_custom_target(
    validate_noiseprofiles_json ALL
    COMMAND ${jsonschema_BIN} -i ${CMAKE_CURRENT_SOURCE_DIR}/noiseprofiles.json ${CMAKE_CURRENT_SOURCE_DIR}/noiseprofiles.schema
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/noiseprofiles.json ${CMAKE_CURRENT_SOURCE_DIR}/noiseprofiles.schema
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    COMMENT "Checking validity of noiseprofiles.json"
  )
endif()
FILE(COPY noiseprofiles.json DESTINATION "${DARKTABLE_DATADIR}")
install(FILES noiseprofiles.json DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable COMPONENT DTApplication)

#
# Install wb presets
#
if(${VALIDATE_JSON})
  add_custom_target(
    validate_wbpresets_json ALL
    COMMAND ${jsonschema_BIN} -i ${CMAKE_CURRENT_SOURCE_DIR}/wb_presets.json ${CMAKE_CURRENT_SOURCE_DIR}/wb_presets.schema
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/wb_presets.json ${CMAKE_CURRENT_SOURCE_DIR}/wb_presets.schema
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    COMMENT "Checking validity of wb_presets.json"
  )
endif()
FILE(COPY wb_presets.json DESTINATION "${DARKTABLE_DATADIR}")
install(FILES wb_presets.json DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable COMPONENT DTApplication)

if(USE_AI)
  #
  # Install AI models registry
  #
  FILE(COPY ai_models.json DESTINATION "${DARKTABLE_DATADIR}")
  install(FILES ai_models.json DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable COMPONENT DTApplication)
  FILE(COPY ort_gpu.json DESTINATION "${DARKTABLE_DATADIR}")
  install(FILES ort_gpu.json DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable COMPONENT DTApplication)

  #
  # Built-in AI models (optional, enabled by default in debug builds)
  #
  option(DARKTABLE_AI_BUILTIN_MODELS "Bundle AI models into the installation" OFF)

  if(DARKTABLE_AI_BUILTIN_MODELS)
    # Model download mirror (e.g. for users in China)
    set(DARKTABLE_AI_MODEL_MIRROR "" CACHE STRING "Mirror base URL for AI model downloads")

    # Built-in model list
    set(_BUILTIN_MODELS "rating-aesthetic-v1")

    foreach(_MODEL_ID ${_BUILTIN_MODELS})
      set(_MODEL_ASSET "${_MODEL_ID}.dtmodel")
      set(_MODEL_ZIP "${CMAKE_CURRENT_BINARY_DIR}/${_MODEL_ASSET}")

      # 1. Check for locally-provided model (generated by setup-build.sh)
      set(_LOCAL_MODEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/models/${_MODEL_ID}")
      set(_LOCAL_MODEL_ZIP "${CMAKE_CURRENT_SOURCE_DIR}/models/${_MODEL_ASSET}")

      if(EXISTS "${_LOCAL_MODEL_DIR}/config.json" AND EXISTS "${_LOCAL_MODEL_DIR}/model.onnx")
        # Already extracted in source tree
        file(MAKE_DIRECTORY "${DARKTABLE_DATADIR}/models")
        file(COPY "${_LOCAL_MODEL_DIR}" DESTINATION "${DARKTABLE_DATADIR}/models")
        message(STATUS "AI model ${_MODEL_ID} bundled (from source tree)")
        install(DIRECTORY "${_LOCAL_MODEL_DIR}"
          DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable/models
          COMPONENT DTApplication)
      elseif(EXISTS "${_LOCAL_MODEL_ZIP}")
        # Pre-packaged .dtmodel in source tree
        file(COPY "${_LOCAL_MODEL_ZIP}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
        set(_MODEL_ZIP "${CMAKE_CURRENT_BINARY_DIR}/${_MODEL_ASSET}")
      elseif(EXISTS "${_MODEL_ZIP}")
        # Already in build dir
      else()
        # Try to download
        if(DARKTABLE_AI_MODEL_MIRROR AND NOT DARKTABLE_AI_MODEL_MIRROR STREQUAL "")
          set(_MODEL_URL "${DARKTABLE_AI_MODEL_MIRROR}/${_MODEL_ASSET}")
        else()
          set(_MODEL_URL "https://github.com/darktable-org/darktable-ai/releases/download/nightly-5.5.0/${_MODEL_ASSET}")
        endif()
        message(STATUS "Downloading AI model ${_MODEL_ID} from ${_MODEL_URL} ...")
        file(DOWNLOAD "${_MODEL_URL}" "${_MODEL_ZIP}.tmp"
          SHOW_PROGRESS STATUS _DOWNLOAD_STATUS TIMEOUT 300)
        list(GET _DOWNLOAD_STATUS 0 _DOWNLOAD_RC)
        if(_DOWNLOAD_RC EQUAL 0)
          file(RENAME "${_MODEL_ZIP}.tmp" "${_MODEL_ZIP}")
        else()
          list(GET _DOWNLOAD_STATUS 1 _DOWNLOAD_ERR)
          message(STATUS "AI model ${_MODEL_ID} download failed: ${_DOWNLOAD_ERR}")
          file(REMOVE "${_MODEL_ZIP}.tmp")
        endif()
      endif()

      # Extract .dtmodel if we have one
      if(EXISTS "${_MODEL_ZIP}")
        file(MAKE_DIRECTORY "${DARKTABLE_DATADIR}/models")
        execute_process(
          COMMAND ${CMAKE_COMMAND} -E tar xf "${_MODEL_ZIP}"
          WORKING_DIRECTORY "${DARKTABLE_DATADIR}/models"
          RESULT_VARIABLE _EXTRACT_RC
        )
        if(_EXTRACT_RC EQUAL 0)
          message(STATUS "AI model ${_MODEL_ID} bundled")
          install(DIRECTORY "${DARKTABLE_DATADIR}/models/${_MODEL_ID}"
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable/models
            COMPONENT DTApplication)
        else()
          message(WARNING "Failed to extract ${_MODEL_ASSET}")
        endif()
      endif()
    endforeach()
  endif()
endif(USE_AI)

#
# Transform darktableconfig.xml into darktablerc
#

set(DEFCONFIG_APPLE "false")
set(DEFCONFIG_NONAPPLE "true")
set(DEFCONFIG_OPENCL "true")

if(APPLE)
  # OpenCL support on OS X isn't stable enough to enable it by default
  set(DEFCONFIG_OPENCL "false")
  set(DEFCONFIG_APPLE "true")
  set(DEFCONFIG_NONAPPLE "false")
endif(APPLE)


set(DEFCONFIG_AUDIOPLAYER "aplay")
if(APPLE)
  # OS X doesn't have aplay, but afplay should do
  set(DEFCONFIG_AUDIOPLAYER "afplay")
endif(APPLE)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/darktableconfig.dtd ${DARKTABLE_DATADIR}/darktableconfig.dtd COPYONLY)

# get_property(DT_PLUGIN_IOPS GLOBAL PROPERTY DT_PLUGIN_IOPS)
get_property(DT_PLUGIN_IOPS_VISIBLE_BY_DEFAULT GLOBAL PROPERTY DT_PLUGIN_IOPS_VISIBLE_BY_DEFAULT)

set(DARKTABLECONFIG_IOP_ENTRIES "")
foreach(DT_PLUGIN_IOP IN LISTS DT_PLUGIN_IOPS_VISIBLE_BY_DEFAULT)
  set(DARKTABLECONFIG_IOP_ENTRIES "${DARKTABLECONFIG_IOP_ENTRIES}
  <dtconfig>
    <name>plugins/darkroom/${DT_PLUGIN_IOP}/visible</name>
    <type>bool</type>
    <default>true</default>
    <shortdescription/>
    <longdescription/>
  </dtconfig>")
endforeach()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/darktableconfig.xml.in ${CMAKE_CURRENT_BINARY_DIR}/darktableconfig.xml)

if(USE_XMLLINT)
  add_custom_target(
    validate_darktableconfig_xml ALL
    COMMAND ${Xmllint_BIN} --nonet --valid --noout ${CMAKE_CURRENT_BINARY_DIR}/darktableconfig.xml
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/darktableconfig.xml
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    COMMENT "Checking validity of ${CMAKE_CURRENT_BINARY_DIR}/darktableconfig.xml"
  )
endif(USE_XMLLINT)

if(NOT ${Xsltproc_BIN} STREQUAL "Xsltproc_BIN-NOTFOUND")
  add_custom_command(
    DEPENDS ${CMAKE_SOURCE_DIR}/tools/generate_darktablerc.xsl ${DARKTABLE_DATADIR}/darktableconfig.dtd ${CMAKE_CURRENT_BINARY_DIR}/darktableconfig.xml
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/darktablerc
    COMMAND ${Xsltproc_BIN} --nonet ${CMAKE_SOURCE_DIR}/tools/generate_darktablerc.xsl ${CMAKE_CURRENT_BINARY_DIR}/darktableconfig.xml > ${CMAKE_CURRENT_BINARY_DIR}/darktablerc
  )
else(NOT ${Xsltproc_BIN} STREQUAL "Xsltproc_BIN-NOTFOUND")
  if(NOT ${Saxon_BIN} STREQUAL "Saxon_BIN-NOTFOUND")
    add_custom_command(
      DEPENDS ${CMAKE_SOURCE_DIR}/tools/generate_darktablerc.xsl ${DARKTABLE_DATADIR}/darktableconfig.dtd ${CMAKE_CURRENT_BINARY_DIR}/darktableconfig.xml
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/darktablerc
      COMMAND ${Saxon_BIN} ${CMAKE_CURRENT_BINARY_DIR}/darktableconfig.xml ${CMAKE_SOURCE_DIR}/tools/generate_darktablerc.xsl > ${CMAKE_CURRENT_BINARY_DIR}/darktablerc
    )
  else(NOT ${Saxon_BIN} STREQUAL "Saxon_BIN-NOTFOUND")
    message(FATAL_ERROR "Don't know how to generate darktablerc")
  endif(NOT ${Saxon_BIN} STREQUAL "Saxon_BIN-NOTFOUND")
endif(NOT ${Xsltproc_BIN} STREQUAL "Xsltproc_BIN-NOTFOUND")

add_custom_target(
  darktablerc_file ALL
  DEPENDS ${CMAKE_SOURCE_DIR}/tools/generate_darktablerc.xsl ${DARKTABLE_DATADIR}/darktableconfig.dtd ${CMAKE_CURRENT_BINARY_DIR}/darktableconfig.xml ${CMAKE_CURRENT_BINARY_DIR}/darktablerc
)
if(USE_XMLLINT)
  add_dependencies(darktablerc_file validate_darktableconfig_xml)
endif(USE_XMLLINT)

# first of all install darktablerc file into share as template
# postinst script should copy this into users homedirectory
# $HOME/.config/darktable/darktable.rc
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/darktablerc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable COMPONENT DTApplication)

#
# Install bash completion file. users/packagers can symlink that to /etc/bash_completion.d/
#
if ((NOT WIN32) OR BUILD_MSYS2_INSTALL)
  install(FILES darktable.bash DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/darktable COMPONENT DTApplication)
endif()
