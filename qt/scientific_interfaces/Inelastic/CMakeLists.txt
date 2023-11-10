set(SRC_FILES
    Analysis/ConvFitAddWorkspaceDialog.cpp
    Analysis/ConvFitModel.cpp
    Analysis/ConvFitDataView.cpp
    Analysis/ConvFitDataPresenter.cpp
    Analysis/IndirectDataAnalysis.cpp
    Analysis/IndirectDataAnalysisTab.cpp
    Analysis/IndirectDataAnalysisMSDFitTab.cpp
    Analysis/IndirectDataAnalysisIqtFitTab.cpp
    Analysis/IndirectDataAnalysisConvFitTab.cpp
    Analysis/IndirectDataAnalysisFqFitTab.cpp
    Analysis/FqFitAddWorkspaceDialog.cpp
    Analysis/FqFitDataPresenter.cpp
    Analysis/FqFitDataView.cpp
    Analysis/FqFitModel.cpp
    Analysis/IqtFitModel.cpp
    Analysis/MSDFitModel.cpp
    Analysis/IDAFunctionParameterEstimation.cpp
    Analysis/FitStatusWidget.cpp
    Analysis/FunctionTemplateBrowser.cpp
    Analysis/IndirectDockWidgetArea.cpp
    Analysis/IndirectEditResultsDialog.cpp
    Analysis/IndirectFitData.cpp
    Analysis/IndirectFitDataModel.cpp
    Analysis/IndirectFitDataPresenter.cpp
    Analysis/IndirectFitDataView.cpp
    Analysis/IndirectFitOutput.cpp
    Analysis/IndirectFitOutputOptionsModel.cpp
    Analysis/IndirectFitOutputOptionsPresenter.cpp
    Analysis/IndirectFitOutputOptionsView.cpp
    Analysis/IndirectFitPlotModel.cpp
    Analysis/IndirectFitPlotPresenter.cpp
    Analysis/IndirectFitPlotView.cpp
    Analysis/IndirectFitPropertyBrowser.cpp
    Analysis/IndirectFittingModel.cpp
    Analysis/FunctionBrowser/ConvTypes.cpp
    Analysis/FunctionBrowser/ConvFunctionModel.cpp
    Analysis/FunctionBrowser/ConvTemplateBrowser.cpp
    Analysis/FunctionBrowser/ConvTemplatePresenter.cpp
    Analysis/FunctionBrowser/IqtFunctionModel.cpp
    Analysis/FunctionBrowser/IqtTemplateBrowser.cpp
    Analysis/FunctionBrowser/IqtTemplatePresenter.cpp
    Analysis/FunctionBrowser/SingleFunctionTemplateModel.cpp
    Analysis/FunctionBrowser/SingleFunctionTemplateBrowser.cpp
    Analysis/FunctionBrowser/SingleFunctionTemplatePresenter.cpp
    Manipulation/InelasticDataManipulation.cpp
    Manipulation/InelasticDataManipulationElwinTab.cpp
    Manipulation/InelasticDataManipulationElwinTabModel.cpp
    Manipulation/InelasticDataManipulationElwinTabView.cpp
    Manipulation/InelasticDataManipulationIqtTab.cpp
    Manipulation/InelasticDataManipulationIqtTabView.cpp
    Manipulation/InelasticDataManipulationIqtTabModel.cpp
    Manipulation/InelasticDataManipulationMomentsTab.cpp
    Manipulation/InelasticDataManipulationMomentsTabModel.cpp
    Manipulation/InelasticDataManipulationMomentsTabView.cpp
    Manipulation/InelasticDataManipulationTab.cpp
    Manipulation/InelasticDataManipulationSqwTab.cpp
    Manipulation/InelasticDataManipulationSqwTabModel.cpp
    Manipulation/InelasticDataManipulationSqwTabView.cpp
    Manipulation/InelasticDataManipulationSymmetriseTab.cpp
    Manipulation/InelasticDataManipulationSymmetriseTabView.cpp
    Manipulation/InelasticDataManipulationSymmetriseTabModel.cpp
)

set(MOC_FILES
    Analysis/ConvFitAddWorkspaceDialog.h
    Analysis/ConvFitDataView.h
    Analysis/ConvFitDataPresenter.h
    Analysis/IndirectDataAnalysis.h
    Analysis/IndirectDataAnalysisTab.h
    Analysis/IndirectDataAnalysisMSDFitTab.h
    Analysis/IndirectDataAnalysisIqtFitTab.h
    Analysis/IndirectDataAnalysisConvFitTab.h
    Analysis/IndirectDataAnalysisFqFitTab.h
    Analysis/FqFitAddWorkspaceDialog.h
    Analysis/FqFitDataPresenter.h
    Analysis/FqFitDataView.h
    Analysis/FitStatusWidget.h
    Analysis/FunctionTemplateBrowser.h
    Analysis/IIndirectFitDataView.h
    Analysis/IIndirectFitOutputOptionsView.h
    Analysis/IIndirectFitPlotView.h
    Analysis/IndirectDockWidgetArea.h
    Analysis/IndirectEditResultsDialog.h
    Analysis/IndirectFitDataPresenter.h
    Analysis/IndirectFitDataView.h
    Analysis/IndirectFitOutputOptionsPresenter.h
    Analysis/IndirectFitOutputOptionsView.h
    Analysis/IndirectFitPlotView.h
    Analysis/IndirectFitPlotPresenter.h
    Analysis/IndirectFitPropertyBrowser.h
    Analysis/FunctionBrowser/ConvTemplateBrowser.h
    Analysis/FunctionBrowser/ConvTemplatePresenter.h
    Analysis/FunctionBrowser/IqtTemplateBrowser.h
    Analysis/FunctionBrowser/IqtTemplatePresenter.h
    Analysis/FunctionBrowser/SingleFunctionTemplateBrowser.h
    Analysis/FunctionBrowser/SingleFunctionTemplatePresenter.h
    Manipulation/InelasticDataManipulation.h
    Manipulation/InelasticDataManipulationElwinTab.h
    Manipulation/InelasticDataManipulationElwinTabView.h
    Manipulation/InelasticDataManipulationIqtTab.h
    Manipulation/InelasticDataManipulationIqtTabView.h
    Manipulation/InelasticDataManipulationMomentsTab.h
    Manipulation/InelasticDataManipulationMomentsTabView.h
    Manipulation/InelasticDataManipulationSqwTab.h
    Manipulation/InelasticDataManipulationSqwTabView.h
    Manipulation/InelasticDataManipulationSymmetriseTab.h
    Manipulation/InelasticDataManipulationSymmetriseTabView.h
    Manipulation/InelasticDataManipulationTab.h
)

set(INC_FILES
    DllConfig.h
    PrecompiledHeader.h
    Analysis/IDAFunctionParameterEstimation.h
    Analysis/FqFitModel.h
    Analysis/IFQFitObserver.h
    Analysis/IqtFitModel.h
    Analysis/MSDFitModel.h
    Analysis/ConvFitModel.h
    Analysis/FQFitConstants.h
    Analysis/IIndirectFitDataModel.h
    Analysis/IndirectFitDataModel.h
    Analysis/IIndirectFitOutput.h
    Analysis/IIndirectFitOutputOptionsModel.h
    Analysis/IIndirectFittingModel.h
    Analysis/IndirectFitData.h
    Analysis/IndirectFitOutput.h
    Analysis/IndirectFitOutputOptionsModel.h
    Analysis/IndirectFitPlotModel.h
    Analysis/IndirectFittingModel.h
    Analysis/ParameterEstimation.h
    Analysis/FunctionBrowser/ConvFunctionModel.h
    Manipulation/InelasticDataManipulationElwinTabModel.h
    Manipulation/InelasticDataManipulationIqtTabModel.h
    Manipulation/InelasticDataManipulationMomentsTabModel.h
    Manipulation/InelasticDataManipulationSqwTabModel.h
    Manipulation/InelasticDataManipulationSymmetriseTabModel.h
)

set(UI_FILES
    Analysis/ConvFitAddWorkspaceDialog.ui
    Analysis/FqFitAddWorkspaceDialog.ui
    Analysis/IndirectDataAnalysis.ui
    Analysis/IndirectEditResultsDialog.ui
    Analysis/IndirectFitDataView.ui
    Analysis/IndirectFitPreviewPlot.ui
    Analysis/IndirectFitOutputOptions.ui
    Analysis/IndirectFitTab.ui
    Analysis/IndirectSpectrumSelector.ui
    Manipulation/InelasticDataManipulation.ui
    Manipulation/InelasticDataManipulationElwinTab.ui
    Manipulation/InelasticDataManipulationIqtTab.ui
    Manipulation/InelasticDataManipulationMomentsTab.ui
    Manipulation/InelasticDataManipulationSqwTab.ui
    Manipulation/InelasticDataManipulationSymmetriseTab.ui
)

set(RES_FILES ../Indirect/IndirectInterfaceResources.qrc)

# XML is required to parse the settings file
find_package(
  Qt5 ${QT_MIN_VERSION}
  COMPONENTS Concurrent Xml
  REQUIRED
)

mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesInelastic
  QT_VERSION 5
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
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
                   MantidScientificInterfacesIndirect
  INSTALL_DIR_BASE ${WORKBENCH_PLUGINS_DIR}
  OSX_INSTALL_RPATH @loader_path/../../MacOS @loader_path/../../Frameworks @loader_path/../../plugins/qt5
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt5/"
)

# Testing target
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  add_subdirectory(test)
endif()