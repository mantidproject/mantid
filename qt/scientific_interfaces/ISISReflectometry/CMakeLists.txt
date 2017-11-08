set ( SRC_FILES
    MeasurementItem.cpp
    QtReflEventTabView.cpp
    QtReflEventView.cpp
    QtReflMainWindowView.cpp
    QtReflRunsTabView.cpp
    QtReflSaveTabView.cpp
    QtReflSettingsTabView.cpp
    QtReflSettingsView.cpp
    ReflCatalogSearcher.cpp
    ReflDataProcessorPresenter.cpp
    ReflEventPresenter.cpp
    ReflEventTabPresenter.cpp
    ReflFromStdStringMap.cpp
    ReflGenericDataProcessorPresenterFactory.cpp
    ReflLegacyTransferStrategy.cpp
    ReflMainWindowPresenter.cpp
    ReflMeasureTransferStrategy.cpp
    ReflNexusMeasurementItemSource.cpp
    ReflRunsTabPresenter.cpp
    ReflSaveTabPresenter.cpp
    ReflSearchModel.cpp
    ReflSettingsPresenter.cpp
    ReflSettingsTabPresenter.cpp
    ReflTableSchema.cpp
    TransferResults.cpp
    Trim.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( INC_FILES
    DllConfig.h
    IReflEventPresenter.h
    IReflEventTabPresenter.h
    IReflEventView.h
    IReflMainWindowPresenter.h
    IReflMainWindowView.h
    IReflRunsTabPresenter.h
    IReflRunsTabView.h
    IReflSaveTabPresenter.h
    IReflSaveTabView.h
    IReflSearcher.h
    IReflSettingsPresenter.h
    IReflSettingsTabPresenter.h
    IReflSettingsView.h
    MeasurementItem.h
    PrecompiledHeader.h
    QtReflEventTabView.h
    QtReflEventView.h
    QtReflMainWindowView.h
    QtReflRunsTabView.h
    QtReflSaveTabView.h
    QtReflSettingsTabView.h
    QtReflSettingsView.h
    ReflCatalogSearcher.h
    ReflDataProcessorMainPresenter.h
    ReflDataProcessorPresenter.h
    ReflEventPresenter.h
    ReflEventTabPresenter.h
    ReflFromStdStringMap.h
    ReflGenericDataProcessorPresenterFactory.h
    ReflLegacyTransferStrategy.h
    ReflMainWindowPresenter.h
    ReflMeasureTransferStrategy.h
    ReflMeasurementItemSource.h
    ReflNexusMeasurementItemSource.h
    ReflRunsTabPresenter.h
    ReflSaveTabPresenter.h
    ReflSearchModel.h
    ReflSettingsPresenter.h
    ReflSettingsTabPresenter.h
    ReflTableSchema.h
    ReflTransferStrategy.h
    TransferResults.h
    Trim.h
)

set ( MOC_FILES
    ReflSearchModel.h
    QtReflEventTabView.h
    QtReflEventView.h
    QtReflRunsTabView.h
    QtReflSaveTabView.h
    QtReflSettingsTabView.h
    QtReflSettingsView.h
)

set ( UI_FILES
   ReflEventTabWidget.ui
   ReflEventWidget.ui
   ReflMainWindowWidget.ui
   ReflRunsTabWidget.ui
   ReflSaveTabWidget.ui
   ReflSettingsTabWidget.ui
   ReflSettingsWidget.ui
   ReflWindow.ui
)


qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} OPTIONS ${extra_options} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# Use a precompiled header where they are supported
enable_precompiled_headers( PrecompiledHeader.h  ALL_SRC )
add_library ( MantidScientificInterfacesISISReflectometry ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# Set the name of the generated library
set_target_properties ( MantidScientificInterfacesISISReflectometry PROPERTIES COMPILE_DEFINITIONS IN_MANTIDQT_ISISREFLECTOMETRY )
if ( MSVC_IDE )
#  # Add to the 'ScientificInterfaces' group in VS
  set_property ( TARGET MantidScientificInterfacesISISReflectometry PROPERTY FOLDER "ScientificInterfaces" )
endif()

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidScientificInterfacesISISReflectometry PROPERTIES INSTALL_RPATH "@loader_path/../../../Contents/MacOS")
endif ()

find_Package(Threads)

target_link_libraries ( MantidScientificInterfacesISISReflectometry LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidQtWidgetsCommon
            ${CORE_MANTIDLIBS}
            ${QT_LIBRARIES}
            ${QWT_LIBRARIES}
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            ${JSONCPP_LIBRARIES}
            ${CMAKE_THREAD_LIBS_INIT})

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidScientificInterfacesISISReflectometry ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR}/qtplugins/mantid )
