set(ALL_SRC_FILES
    Analysis/ConvFitAddWorkspaceDialog.cpp
    Analysis/ConvFitModel.cpp
    Analysis/ConvFitDataView.cpp
    Analysis/ConvFitDataPresenter.cpp
    Analysis/DataAnalysis.cpp
    Analysis/DataAnalysisTab.cpp
    Analysis/DataAnalysisTabFactory.cpp
    Analysis/DockWidgetArea.cpp
    Analysis/EditResultsDialog.cpp
    Analysis/FitStatusWidget.cpp
    Analysis/FitData.cpp
    Analysis/FitDataModel.cpp
    Analysis/FitDataPresenter.cpp
    Analysis/FitDataView.cpp
    Analysis/FitOutput.cpp
    Analysis/FitOutputOptionsModel.cpp
    Analysis/FitOutputOptionsPresenter.cpp
    Analysis/FitOutputOptionsView.cpp
    Analysis/FitPlotModel.cpp
    Analysis/FitPlotPresenter.cpp
    Analysis/FitPlotView.cpp
    Analysis/InelasticFitPropertyBrowser.cpp
    Analysis/FittingModel.cpp
    Analysis/FqFitAddWorkspaceDialog.cpp
    Analysis/FqFitDataPresenter.cpp
    Analysis/FqFitDataView.cpp
    Analysis/FqFitModel.cpp
    Analysis/IqtFitModel.cpp
    Analysis/MSDFitModel.cpp
    Analysis/IDAFunctionParameterEstimation.cpp
    Analysis/FunctionBrowser/ConvFunctionTemplateModel.cpp
    Analysis/FunctionBrowser/FitTypes.cpp
    Analysis/FunctionBrowser/FunctionTemplateView.cpp
    Analysis/FunctionBrowser/FunctionTemplatePresenter.cpp
    Analysis/FunctionBrowser/FqFunctionModel.cpp
    Analysis/FunctionBrowser/IqtFunctionTemplateModel.cpp
    Analysis/FunctionBrowser/IqtFunctionTemplateView.cpp
    Analysis/FunctionBrowser/IqtTemplatePresenter.cpp
    Analysis/FunctionBrowser/MSDFunctionModel.cpp
    Analysis/FunctionBrowser/SingleFunctionTemplateModel.cpp
    Analysis/FunctionBrowser/SingleFunctionTemplateView.cpp
    Analysis/FunctionBrowser/SingleFunctionTemplatePresenter.cpp
    Analysis/FunctionBrowser/MultiFunctionTemplateModel.cpp
    Analysis/FunctionBrowser/MultiFunctionTemplateView.cpp
    Analysis/FunctionBrowser/MultiFunctionTemplatePresenter.cpp
)

set(ALL_MOC_FILES
    Analysis/ConvFitAddWorkspaceDialog.h
    Analysis/ConvFitDataView.h
    Analysis/DataAnalysis.h
    Analysis/DataAnalysisTab.h
    Analysis/DockWidgetArea.h
    Analysis/EditResultsDialog.h
    Analysis/FqFitAddWorkspaceDialog.h
    Analysis/FqFitDataView.h
    Analysis/FitStatusWidget.h
    Analysis/FitDataView.h
    Analysis/FitOutputOptionsView.h
    Analysis/FitPlotView.h
    Analysis/InelasticFitPropertyBrowser.h
    Analysis/FunctionBrowser/FunctionTemplateView.h
    Analysis/FunctionBrowser/IqtFunctionTemplateView.h
    Analysis/FunctionBrowser/SingleFunctionTemplateView.h
    Analysis/FunctionBrowser/MultiFunctionTemplateView.h
)

set(ALL_INC_FILES
    DllConfig.h
    PrecompiledHeader.h
    Analysis/ConvFitDataPresenter.h
    Analysis/DataAnalysisTabFactory.h
    Analysis/FqFitDataPresenter.h
    Analysis/IDAFunctionParameterEstimation.h
    Analysis/FqFitModel.h
    Analysis/IqtFitModel.h
    Analysis/MSDFitModel.h
    Analysis/ConvFitModel.h
    Analysis/FitTabConstants.h
    Analysis/IFitDataModel.h
    Analysis/IFitDataView.h
    Analysis/IFitOutput.h
    Analysis/IFitOutputOptionsModel.h
    Analysis/IFitOutputOptionsView.h
    Analysis/IFitPlotView.h
    Analysis/IFittingModel.h
    Analysis/FitData.h
    Analysis/FitDataModel.h
    Analysis/FitDataPresenter.h
    Analysis/FitOutput.h
    Analysis/FitOutputOptionsModel.h
    Analysis/FitOutputOptionsPresenter.h
    Analysis/FitPlotModel.h
    Analysis/FitPlotPresenter.h
    Analysis/FittingModel.h
    Analysis/ParameterEstimation.h
    Analysis/FunctionBrowser/ConvFunctionTemplateModel.h
    Analysis/FunctionBrowser/FitTypes.h
    Analysis/FunctionBrowser/FunctionTemplatePresenter.h
    Analysis/FunctionBrowser/FqFunctionModel.h
    Analysis/FunctionBrowser/ITemplatePresenter.h
    Analysis/FunctionBrowser/IqtTemplatePresenter.h
    Analysis/FunctionBrowser/MSDFunctionModel.h
    Analysis/FunctionBrowser/ParamID.h
    Analysis/FunctionBrowser/SingleFunctionTemplatePresenter.h
    Analysis/FunctionBrowser/MultiFunctionTemplateModel.h
    Analysis/FunctionBrowser/MultiFunctionTemplatePresenter.h
    Analysis/FunctionBrowser/TemplateSubType.h
)

set(ALL_UI_FILES
    Analysis/ConvFitAddWorkspaceDialog.ui
    Analysis/FqFitAddWorkspaceDialog.ui
    Analysis/DataAnalysis.ui
    Analysis/EditResultsDialog.ui
    Analysis/FitDataView.ui
    Analysis/FitPreviewPlot.ui
    Analysis/FitOutputOptions.ui
    Analysis/FitTab.ui
    Analysis/SpectrumSelector.ui
)

set(RES_FILES Common/InterfaceResources.qrc)

add_subdirectory(BayesFitting)
add_subdirectory(Common)
add_subdirectory(Corrections)
add_subdirectory(Manipulation)

# XML is required to parse the settings file
find_package(
  Qt5 ${QT_MIN_VERSION}
  COMPONENTS Concurrent Xml
  REQUIRED
)

mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesInelastic
  QT_VERSION 5
  SRC ${ALL_SRC_FILES}
  MOC ${ALL_MOC_FILES}
  NOMOC ${ALL_INC_FILES}
  UI ${ALL_UI_FILES}
  RES ${RES_FILES}
  DEFS IN_MANTIDQT_INELASTIC PRECOMPILED PrecompiledHeader.h
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
  LINK_LIBS ${CORE_MANTIDLIBS}
            Mantid::PythonInterfaceCore
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            Python::Python
            Qt5::Concurrent
            Qt5::Xml
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsPlotting MantidQtWidgetsMplCpp MantidQtIcons
  INSTALL_DIR_BASE ${WORKBENCH_PLUGINS_DIR}
  OSX_INSTALL_RPATH @loader_path/../../MacOS @loader_path/../../Frameworks @loader_path/../../plugins/qt5
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt5/"
)

# Testing target
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  add_subdirectory(test)
endif()
