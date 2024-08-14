get_filename_component(SUB_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" NAME)

set(TEST_FILES
    ConvolutionModelTest.h
    ConvolutionDataPresenterTest.h
    ConvFunctionTemplateModelTest.h
    FunctionQDataPresenterTest.h
    FunctionQModelTest.h
    FunctionQParametersTest.h
    FitDataPresenterTest.h
    FitOutputTest.h
    FitOutputOptionsModelTest.h
    FitOutputOptionsPresenterTest.h
    FitPlotModelTest.h
    FitPlotPresenterTest.h
    FittingModelTest.h
    FittingPresenterTest.h
    InelasticFitPropertyBrowserTest.h
    IqtFitModelTest.h
    ParameterEstimationTest.h
)

set(TEST_HELPERS MockObjects.h)

list(TRANSFORM TEST_FILES PREPEND ${SUB_DIRECTORY}/)
list(TRANSFORM TEST_HELPERS PREPEND ${SUB_DIRECTORY}/)

set(ALL_TEST_FILES
    ${ALL_TEST_FILES} ${TEST_FILES}
    PARENT_SCOPE
)
set(ALL_TEST_HELPERS
    ${ALL_TEST_HELPERS} ${TEST_HELPERS}
    PARENT_SCOPE
)
