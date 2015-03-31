PROJECT(PeaksFilter)
include_directories( SYSTEM ${PARAVIEW_INCLUDE_DIRS} )
ADD_PARAVIEW_PLUGIN(MantidParaViewPeaksFilterSMPlugin "1.0"
SERVER_MANAGER_XML PeaksFilter.xml
SERVER_MANAGER_SOURCES vtkPeaksFilter.cxx
GUI_RESOURCE_FILES PeaksFilterGUI.xml)
# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewPeaksFilterSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins")

target_link_libraries( MantidParaViewPeaksFilterSMPlugin LINK_PUBLIC ${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewPeaksFilterSMPlugin  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewPeaksFilterSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})
