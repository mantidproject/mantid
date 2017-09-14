set ( SRC_FILES
	DataComparison.cpp
	DirectConvertToEnergy.cpp
	Homer.cpp
	MantidEV.cpp
	MantidEVWorker.cpp
	SampleTransmission.cpp
	StepScan.cpp
	UserInputValidator.cpp
	background.cpp
	deltaECalc.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( INC_FILES
	Background.h
	DataComparison.h
	DirectConvertToEnergy.h
	DllConfig.h
	Homer.h
	MantidEV.h
	MantidEVWorker.h
	SampleTransmission.h
	StepScan.h
	Updateable.h
	UserInputValidator.h
	deltaECalc.h
)

set ( MOC_FILES 
    Background.h
    DataComparison.h
    DirectConvertToEnergy.h
    Homer.h
    MantidEV.h
    SampleTransmission.h
    StepScan.h
)

set ( UI_FILES 
    DataComparison.ui
    DirectConvertToEnergy.ui
    MantidEV.ui
    SampleTransmission.ui
    StepScan.ui
)

mtd_add_qt_library (TARGET_NAME MantidScientificInterfacesGeneral
                    SRC ${SRC_FILES}
                    MOC ${MOC_FILES}
                    NOMOC ${INC_FILES}
                    UI ${UI_FILES}
                    DEFS IN_MANTIDQT_INTERFACESGENERAL
                    PERECOMPILED PrecompiledHeader.h
                    LINK_LIBRARIES
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${CORE_MANTIDLIBS}
                        ${QT_LIBRARIES}
                        ${QWT_LIBRARIES}
                        ${POCO_LIBRARIES}
                        ${Boost_LIBRARIES}
                        ${JSONCPP_LIBRARIES}
                    MTD_QT_LINK_LIBRARIES
                        MantidQtWidgetsCommon
                    INSTALL_DIR ${PLUGINS_DIR}/qtplugins/mantid
                    OSX_INSTALL_RPATH "@loader_path/../../../Contents/MacOS"
)

if ( MSVC_IDE )
#  # Add to the 'ScientificInterfaces' group in VS
  set_property ( TARGET MantidScientificInterfacesGeneralQt4 PROPERTY FOLDER "ScientificInterfaces" )
endif()
