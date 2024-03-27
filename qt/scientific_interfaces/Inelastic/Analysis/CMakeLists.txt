get_filename_component(SUB_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" NAME)

set(SRC_FILES
    ConvFitAddWorkspaceDialog.cpp
    ConvFitModel.cpp
    ConvFitDataView.cpp
    ConvFitDataPresenter.cpp
    DataAnalysis.cpp
    DataAnalysisTab.cpp
    DataAnalysisTabFactory.cpp
    DockWidgetArea.cpp
    EditResultsDialog.cpp
    FitStatusWidget.cpp
    FitData.cpp
    FitDataModel.cpp
    FitDataPresenter.cpp
    FitDataView.cpp
    FitOutput.cpp
    FitOutputOptionsModel.cpp
    FitOutputOptionsPresenter.cpp
    FitOutputOptionsView.cpp
    FitPlotModel.cpp
    FitPlotPresenter.cpp
    FitPlotView.cpp
    FittingModel.cpp
    FqFitAddWorkspaceDialog.cpp
    FqFitDataPresenter.cpp
    FqFitDataView.cpp
    FqFitModel.cpp
    FunctionBrowser/ConvFunctionTemplateModel.cpp
    FunctionBrowser/FitTypes.cpp
    FunctionBrowser/FunctionTemplateView.cpp
    FunctionBrowser/FunctionTemplatePresenter.cpp
    FunctionBrowser/FqFunctionModel.cpp
    FunctionBrowser/IqtFunctionTemplateModel.cpp
    FunctionBrowser/IqtFunctionTemplateView.cpp
    FunctionBrowser/IqtTemplatePresenter.cpp
    FunctionBrowser/MSDFunctionModel.cpp
    FunctionBrowser/SingleFunctionTemplateModel.cpp
    FunctionBrowser/SingleFunctionTemplateView.cpp
    FunctionBrowser/SingleFunctionTemplatePresenter.cpp
    FunctionBrowser/MultiFunctionTemplateModel.cpp
    FunctionBrowser/MultiFunctionTemplatePresenter.cpp
    FunctionBrowser/MultiFunctionTemplateView.cpp
    IDAFunctionParameterEstimation.cpp
    InelasticFitPropertyBrowser.cpp
    IqtFitModel.cpp
    MSDFitModel.cpp
)

set(INC_FILES
    ConvFitDataPresenter.h
    ConvFitModel.h
    DataAnalysisTabFactory.h
    FitData.h
    FitDataModel.h
    FitDataPresenter.h
    FitOutput.h
    FitOutputOptionsModel.h
    FitOutputOptionsPresenter.h
    FitPlotModel.h
    FitPlotPresenter.h
    FittingModel.h
    FitTabConstants.h
    FqFitDataPresenter.h
    FqFitModel.h
    FunctionBrowser/ConvFunctionTemplateModel.h
    FunctionBrowser/FitTypes.h
    FunctionBrowser/FunctionTemplatePresenter.h
    FunctionBrowser/FqFunctionModel.h
    FunctionBrowser/ITemplatePresenter.h
    FunctionBrowser/IqtFunctionTemplateModel.h
    FunctionBrowser/IqtTemplatePresenter.h
    FunctionBrowser/MSDFunctionModel.h
    FunctionBrowser/ParamID.h
    FunctionBrowser/SingleFunctionTemplatePresenter.h
    FunctionBrowser/MultiFunctionTemplateModel.h
    FunctionBrowser/MultiFunctionTemplatePresenter.h
    FunctionBrowser/TemplateSubType.h
    IDAFunctionParameterEstimation.h
    IFitDataModel.h
    IFitDataView.h
    IFitOutput.h
    IFitOutputOptionsModel.h
    IFitOutputOptionsView.h
    IFitPlotView.h
    IFittingModel.h
    IqtFitModel.h
    MSDFitModel.h
    ParameterEstimation.h
)

set(MOC_FILES
    ConvFitAddWorkspaceDialog.h
    ConvFitDataView.h
    DataAnalysis.h
    DataAnalysisTab.h
    DockWidgetArea.h
    EditResultsDialog.h
    FqFitAddWorkspaceDialog.h
    FqFitDataView.h
    FitStatusWidget.h
    FitDataView.h
    FitOutputOptionsView.h
    FitPlotView.h
    FunctionBrowser/FunctionTemplateView.h
    FunctionBrowser/IqtFunctionTemplateView.h
    FunctionBrowser/SingleFunctionTemplateView.h
    FunctionBrowser/MultiFunctionTemplateView.h
    InelasticFitPropertyBrowser.h
)

set(UI_FILES
    ConvFitAddWorkspaceDialog.ui
    DataAnalysis.ui
    EditResultsDialog.ui
    FitDataView.ui
    FitPreviewPlot.ui
    FitOutputOptions.ui
    FitTab.ui
    FqFitAddWorkspaceDialog.ui
    SpectrumSelector.ui
)

list(TRANSFORM SRC_FILES PREPEND ${SUB_DIRECTORY}/)
list(TRANSFORM INC_FILES PREPEND ${SUB_DIRECTORY}/)
list(TRANSFORM MOC_FILES PREPEND ${SUB_DIRECTORY}/)
list(TRANSFORM UI_FILES PREPEND ${SUB_DIRECTORY}/)

set(ALL_SRC_FILES
    ${ALL_SRC_FILES} ${SRC_FILES}
    PARENT_SCOPE
)
set(ALL_INC_FILES
    ${ALL_INC_FILES} ${INC_FILES}
    PARENT_SCOPE
)
set(ALL_MOC_FILES
    ${ALL_MOC_FILES} ${MOC_FILES}
    PARENT_SCOPE
)
set(ALL_UI_FILES
    ${ALL_UI_FILES} ${UI_FILES}
    PARENT_SCOPE
)
