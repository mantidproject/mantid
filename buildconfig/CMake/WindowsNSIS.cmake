# ######################################################################################################################
# CPack configuration for Windows using NSIS
# ######################################################################################################################

# ######################################################################################################################
# General settings
# ######################################################################################################################
set(CPACK_GENERATOR "NSIS")
set(CPACK_INSTALL_PREFIX "/")
set(CPACK_PACKAGE_NAME "mantid${CPACK_PACKAGE_SUFFIX}")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME}")
set(CPACK_NSIS_INSTALL_ROOT "C:")
# have the properly capitalsed name for the start menu and install folder
set(CPACK_NSIS_DISPLAY_NAME "Mantid${CPACK_PACKAGE_SUFFIX_CAMELCASE}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Mantid${CPACK_PACKAGE_SUFFIX_CAMELCASE}Install")
# note: the PACKAGE icon uses PNG, the other two use ICO
set(PACKAGE_IMAGES_DIR "${CMAKE_CURRENT_SOURCE_DIR}\\\\images")
set(PACKAGE_ICON_BASENAME "mantidplot${CPACK_PACKAGE_SUFFIX}")
set(CPACK_PACKAGE_ICON "${PACKAGE_IMAGES_DIR}\\\\${PACKAGE_ICON_BASENAME}.png")
set(CPACK_NSIS_MUI_ICON "${PACKAGE_IMAGES_DIR}\\\\${PACKAGE_ICON_BASENAME}.ico")
set(CPACK_NSIS_MUI_UNIICON "${PACKAGE_IMAGES_DIR}\\\\${PACKAGE_ICON_BASENAME}.ico")

# ######################################################################################################################
# Deployment type - currently only works for Release!
# ######################################################################################################################
set(WINDOWS_DEPLOYMENT_TYPE
    "Release"
    CACHE STRING "Type of deployment used"
)
set_property(CACHE WINDOWS_DEPLOYMENT_TYPE PROPERTY STRINGS Release)
mark_as_advanced(WINDOWS_DEPLOYMENT_TYPE)

# ######################################################################################################################
# External dependency DLLs
# ######################################################################################################################
set(BOOST_DIST_DLLS boost_date_time-mt.dll boost_filesystem-mt.dll boost_python38-mt.dll boost_regex-mt.dll
                    boost_serialization-mt.dll boost_system-mt.dll
)

set(POCO_DIST_DLLS
    PocoCrypto64.dll
    PocoFoundation64.dll
    PocoJSON64.dll
    PocoNet64.dll
    PocoNetSSL64.dll
    PocoUtil64.dll
    PocoXML64.dll
)
set(OCC_DIST_DLLS
    TKBO.dll
    TKBRep.dll
    TKernel.dll
    TKG2d.dll
    TKG3d.dll
    TKGeomAlgo.dll
    TKGeomBase.dll
    TKMath.dll
    TKMesh.dll
    TKPrim.dll
    TKTopAlgo.dll
)
set(MISC_CORE_DIST_DLLS
    gslcblas.dll
    gsl.dll
    hdf5_cpp.dll
    hdf5_hl_cpp.dll
    hdf5_hl.dll
    hdf5.dll
    jsoncpp.dll
    lib3mf.dll
    libeay32.dll
    libNeXus-0.dll
    libNeXusCPP-0.dll
    librdkafka.dll
    librdkafkacpp.dll
    muparser.dll
    ssleay32.dll
    szip.dll
    tbb.dll
    tbbmalloc.dll
    tbbmalloc_proxy.dll
    tbb_preview.dll
    zlib.dll
)

set(BIN_DLLS ${BOOST_DIST_DLLS} ${POCO_DIST_DLLS} ${OCC_DIST_DLLS} ${MISC_CORE_DIST_DLLS})
foreach(DLL ${BIN_DLLS})
  install(FILES ${THIRD_PARTY_DIR}/bin/${DLL} DESTINATION bin)
endforeach()

# NSIS variables for shortcuts NOTE: DO NOT REMOVE THE BLANK LINES IN THE CPACK_NSIS_CREATE_ICONS_EXTRA COMMANDS AS THEY
# ARE REQUIRED
set(CPACK_NSIS_CREATE_ICONS_EXTRA "")
set(CPACK_NSIS_DELETE_ICONS_EXTRA "")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "")
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "")

# General package icon
install(FILES "${PACKAGE_IMAGES_DIR}\\\\${PACKAGE_ICON_BASENAME}.ico" DESTINATION bin)
# python wrapper
install(
  FILES ${PROJECT_BINARY_DIR}/mantidpython.bat.install
  DESTINATION bin
  RENAME mantidpython.bat
)

# Extra plugin and include locations
set(CPACK_NSIS_ADDITIONAL_INCLUDE_DIR "!addincludedir ${THIRD_PARTY_DIR}/share/nsis-plugins/Include")
set(CPACK_NSIS_ADDITIONAL_PLUGIN_ANSI_DIR
    "!addplugindir /x86-ansi ${THIRD_PARTY_DIR}/share/nsis-plugins/Plugins/x86-ansi"
)
set(CPACK_NSIS_ADDITIONAL_PLUGIN_UNICODE_DIR
    "!addplugindir /x86-unicode ${THIRD_PARTY_DIR}/share/nsis-plugins/Plugins/x86-unicode"
)

# Notebook
set(WINDOWS_NSIS_MANTIDNOTEBOOK_ICON_NAME "mantid_notebook${CPACK_PACKAGE_SUFFIX}")
set(MANTIDNOTEBOOK_LINK_NAME "Mantid Notebook ${CPACK_PACKAGE_SUFFIX_CAMELCASE}.lnk")
install(FILES "${PACKAGE_IMAGES_DIR}\\\\${WINDOWS_NSIS_MANTIDNOTEBOOK_ICON_NAME}.ico" DESTINATION bin)
set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "
  CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${MANTIDNOTEBOOK_LINK_NAME}' '$INSTDIR\\\\bin\\\\mantidpython.bat' 'notebook --notebook-dir=%userprofile%' '$INSTDIR\\\\bin\\\\${WINDOWS_NSIS_MANTIDNOTEBOOK_ICON_NAME}.ico'
"
)
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "
  Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\${MANTIDNOTEBOOK_LINK_NAME}\\\"
"
)
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
    "
  CreateShortCut '$DESKTOP\\\\${MANTIDNOTEBOOK_LINK_NAME}' '$INSTDIR\\\\bin\\\\mantidpython.bat' 'notebook --notebook-dir=%userprofile%' '$INSTDIR\\\\bin\\\\${WINDOWS_NSIS_MANTIDNOTEBOOK_ICON_NAME}.ico'
"
)
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
    "
  Delete \\\"$DESKTOP\\\\${MANTIDNOTEBOOK_LINK_NAME}\\\"
"
)

# Workbench
if(ENABLE_WORKBENCH)
  include(WindowsNSISQt5)
  set(WINDOWS_NSIS_MANTIDWORKBENCH_ICON_NAME "mantid_workbench${CPACK_PACKAGE_SUFFIX}")
  install(FILES "${PACKAGE_IMAGES_DIR}\\\\${WINDOWS_NSIS_MANTIDWORKBENCH_ICON_NAME}.ico" DESTINATION bin)
  set(MANTIDWORKBENCH_LINK_NAME "Mantid Workbench ${CPACK_PACKAGE_SUFFIX_CAMELCASE}.lnk")
  set(CPACK_NSIS_CREATE_ICONS_EXTRA
      "
    CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${MANTIDWORKBENCH_LINK_NAME}' '\\\"$INSTDIR\\\\bin\\\\MantidWorkbench.exe\\\"' '' '$INSTDIR\\\\bin\\\\${WINDOWS_NSIS_MANTIDWORKBENCH_ICON_NAME}.ico'

    ${CPACK_NSIS_CREATE_ICONS_EXTRA}
  "
  )
  set(CPACK_NSIS_DELETE_ICONS_EXTRA
      "
    Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\${MANTIDWORKBENCH_LINK_NAME}\\\"
    ${CPACK_NSIS_DELETE_ICONS_EXTRA}
  "
  )
  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
      "
    CreateShortCut '$DESKTOP\\\\${MANTIDWORKBENCH_LINK_NAME}' '\\\"$INSTDIR\\\\bin\\\\MantidWorkbench.exe\\\"' '' '$INSTDIR\\\\bin\\\\${WINDOWS_NSIS_MANTIDWORKBENCH_ICON_NAME}.ico'

    ${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}
  "
  )
  set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
      "
    Delete \\\"$DESKTOP\\\\${MANTIDWORKBENCH_LINK_NAME}\\\"
    ${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS}
  "
  )
endif()
