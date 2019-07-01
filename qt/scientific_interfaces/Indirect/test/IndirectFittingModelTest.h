// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDIRECTFITTINGMODELTEST_H_
#define MANTID_INDIRECTFITTINGMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectFittingModel.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidCurveFitting/Algorithms/ConvolutionFit.h"
#include "MantidCurveFitting/Algorithms/QENSFitSequential.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace Mantid::IndirectFitDataCreationHelper;

using ConvolutionFitSequential =
    Algorithms::ConvolutionFit<Algorithms::QENSFitSequential>;
using IDAWorkspaceIndex = MantidQt::CustomInterfaces::IDA::WorkspaceIndex;

namespace {

MultiDomainFunction_sptr getFunction(std::string const &functionString) {
  auto fun = FunctionFactory::Instance().createInitialized(functionString);
  return boost::dynamic_pointer_cast<MultiDomainFunction>(fun);
}

/// A dummy model used to inherit the methods which need testing
class DummyModel
    : public MantidQt::CustomInterfaces::IDA::IndirectFittingModel {
public:
  ~DummyModel(){};

private:
  std::string sequentialFitOutputName() const override { return ""; };
  std::string simultaneousFitOutputName() const override { return ""; };
  std::string singleFitOutputName(DatasetIndex index,
                                  IDAWorkspaceIndex spectrum) const override {
    UNUSED_ARG(index);
    UNUSED_ARG(spectrum);
    return "";
  };

  std::vector<std::string> getSpectrumDependentAttributes() const override {
    return std::vector<std::string>();
  }
};

std::unique_ptr<DummyModel> getEmptyModel() {
  return std::make_unique<DummyModel>();
}

std::unique_ptr<DummyModel>
createModelWithSingleWorkspace(std::string const &workspaceName,
                               int const &numberOfSpectra) {
  auto model = getEmptyModel();
  SetUpADSWithWorkspace ads(workspaceName, createWorkspace(numberOfSpectra));
  model->addWorkspace(workspaceName);
  return model;
}

void addWorkspacesToModel(std::unique_ptr<DummyModel> &model,
                          int const &numberOfSpectra) {
  UNUSED_ARG(model);
  UNUSED_ARG(numberOfSpectra);
}

template <typename Name, typename... Names>
void addWorkspacesToModel(std::unique_ptr<DummyModel> &model,
                          int const &numberOfSpectra, Name const &workspaceName,
                          Names const &... workspaceNames) {
  Mantid::API::AnalysisDataService::Instance().addOrReplace(
      workspaceName, createWorkspace(numberOfSpectra));
  model->addWorkspace(workspaceName);
  addWorkspacesToModel(model, numberOfSpectra, workspaceNames...);
}

template <typename Name, typename... Names>
std::unique_ptr<DummyModel>
createModelWithMultipleWorkspaces(int const &numberOfSpectra,
                                  Name const &workspaceName,
                                  Names const &... workspaceNames) {
  auto model = createModelWithSingleWorkspace(workspaceName, numberOfSpectra);
  addWorkspacesToModel(model, numberOfSpectra, workspaceNames...);
  return model;
}

std::unique_ptr<DummyModel> createModelWithSingleInstrumentWorkspace(
    std::string const &workspaceName, int const &xLength, int const &yLength) {
  auto model = getEmptyModel();
  SetUpADSWithWorkspace ads(workspaceName,
                            createWorkspaceWithInstrument(xLength, yLength));
  model->addWorkspace(workspaceName);
  return model;
}

void setFittingFunction(std::unique_ptr<DummyModel> &model,
                        std::string const &functionString) {
  model->setFitFunction(
      getFunction("composite=MultiDomainFunction;(" + functionString + ")"));
}

IAlgorithm_sptr setupFitAlgorithm(MatrixWorkspace_sptr workspace,
                                  std::string const &functionString) {
  auto alg = boost::make_shared<ConvolutionFitSequential>();
  alg->initialize();
  alg->setProperty("InputWorkspace", workspace);
  alg->setProperty("Function", functionString);
  alg->setProperty("StartX", 0.0);
  alg->setProperty("EndX", 3.0);
  alg->setProperty("SpecMin", 0);
  alg->setProperty("SpecMax", 5);
  alg->setProperty("ConvolveMembers", true);
  alg->setProperty("Minimizer", "Levenberg-Marquardt");
  alg->setProperty("MaxIterations", 500);
  alg->setProperty("OutputWorkspace", "output");
  alg->setLogging(false);
  return alg;
}

IAlgorithm_sptr getSetupFitAlgorithm(std::unique_ptr<DummyModel> &model,
                                     MatrixWorkspace_sptr workspace,
                                     std::string const &workspaceName) {
  std::string const function =
      "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
      "(composite=Convolution,FixResolution=true,NumDeriv=true;"
      "name=Resolution,Workspace=" +
      workspaceName +
      ",WorkspaceIndex=0;((composite=ProductFunction,NumDeriv="
      "false;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0."
      "0175)))";
  setFittingFunction(model, function);
  auto alg = setupFitAlgorithm(workspace, function);
  return alg;
}

IAlgorithm_sptr getExecutedFitAlgorithm(std::unique_ptr<DummyModel> &model,
                                        MatrixWorkspace_sptr workspace,
                                        std::string const &workspaceName) {
  auto const alg = getSetupFitAlgorithm(model, workspace, workspaceName);
  alg->execute();
  return alg;
}

std::unique_ptr<DummyModel> getModelWithFitOutputData() {
  auto model = createModelWithSingleInstrumentWorkspace("__ConvFit", 6, 5);
  auto const modelWorkspace = model->getWorkspace(DatasetIndex{0});

  auto const alg = getExecutedFitAlgorithm(model, modelWorkspace, "__ConvFit");
  model->addOutput(alg);
  return model;
}

} // namespace

class IndirectFittingModelTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  IndirectFittingModelTest() { FrameworkManager::Instance(); }

  static IndirectFittingModelTest *createSuite() {
    return new IndirectFittingModelTest();
  }

  static void destroySuite(IndirectFittingModelTest *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_model_is_instantiated_correctly() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 3);

    TS_ASSERT(model->getWorkspace(DatasetIndex{0}));
    TS_ASSERT_EQUALS(model->numberOfWorkspaces(), DatasetIndex{1});
    TS_ASSERT_EQUALS(model->getNumberOfSpectra(DatasetIndex{0}),
                     SpectrumRowIndex{3});
  }

  void test_that_a_workspace_is_stored_correctly_in_the_ADS() {
    SetUpADSWithWorkspace ads("WorkspaceName", createWorkspace(3));

    TS_ASSERT(ads.doesExist("WorkspaceName"));
    auto const storedWorkspace = ads.retrieveWorkspace("WorkspaceName");
    TS_ASSERT_EQUALS(storedWorkspace->getNumberHistograms(), 3);
  }

  void
  test_that_addWorkspace_will_add_a_workspace_to_the_fittingData_using_the_workspace_name() {
    auto model = getEmptyModel();
    auto const workspace = createWorkspace(3);
    SetUpADSWithWorkspace ads("WorkspaceName", workspace);

    model->addWorkspace("WorkspaceName");

    TS_ASSERT_EQUALS(model->getWorkspace(DatasetIndex{0}), workspace);
  }

  void
  test_that_addWorkspace_throws_when_provided_a_workspace_name_and_an_empty_spectraString() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 3);

    std::string const spectraString("");

    TS_ASSERT_THROWS(model->addWorkspace("WorkspaceName", spectraString),
                     const std::runtime_error &);
  }

  void
  test_that_addWorkspace_combines_an_input_workspace_with_a_workspace_that_already_exists_if_the_workspaces_have_the_same_name() {
    auto model = createModelWithMultipleWorkspaces(3, "Name", "Name");

    TS_ASSERT(model->getWorkspace(DatasetIndex{0}));
    TS_ASSERT(!model->getWorkspace(DatasetIndex{1}));
  }

  void
  test_that_addWorkspace_does_not_combine_an_input_workspace_with_a_workspace_that_already_exists_if_the_workspaces_are_differently_named() {
    auto model = getEmptyModel();
    auto const workspace1 = createWorkspace(3);
    auto const workspace2 = createWorkspace(3);
    SetUpADSWithWorkspace ads("WorkspaceName1", workspace1);
    ads.addOrReplace("WorkspaceName2", workspace2);

    model->addWorkspace("WorkspaceName1");
    model->addWorkspace("WorkspaceName2");

    TS_ASSERT_EQUALS(model->getWorkspace(DatasetIndex{0}), workspace1);
    TS_ASSERT_EQUALS(model->getWorkspace(DatasetIndex{1}), workspace2);
  }

  void
  test_that_hasWorkspace_returns_true_when_the_model_contains_a_workspace() {
    auto const model = createModelWithSingleWorkspace("WorkspaceName", 3);
    TS_ASSERT(model->hasWorkspace("WorkspaceName"));
  }

  void
  test_that_hasWorkspace_returns_false_when_the_model_does_not_contain_a_workspace() {
    auto const model = createModelWithSingleWorkspace("WorkspaceName", 3);
    TS_ASSERT(!model->hasWorkspace("WrongName"));
  }

  void
  test_that_getWorkspace_returns_a_nullptr_when_getWorkspace_is_provided_an_out_of_range_index() {
    auto const model = createModelWithSingleWorkspace("WorkspaceName", 3);
    TS_ASSERT_EQUALS(model->getWorkspace(DatasetIndex{1}), nullptr);
  }

  void
  test_that_getSpectra_returns_a_correct_spectra_when_the_index_provided_is_valid() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 3);

    Spectra const inputSpectra = Spectra("0-1");
    model->setSpectra(inputSpectra, DatasetIndex{0});
    Spectra const spectra = model->getSpectra(DatasetIndex{0});

    TS_ASSERT_EQUALS(spectra, inputSpectra);
  }

  void
  test_that_getSpectra_returns_an_empty_DiscontinuousSpectra_when_provided_an_out_of_range_index() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 3);

    Spectra const emptySpectra(Spectra(""));
    Spectra const spectra = model->getSpectra(DatasetIndex{3});

    TS_ASSERT_EQUALS(spectra, emptySpectra);
  }

  void
  test_that_getFittingRange_returns_correct_range_when_provided_a_valid_index_and_spectrum() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 1);

    model->setStartX(1.2, DatasetIndex{0}, IDAWorkspaceIndex{0});
    model->setEndX(5.6, DatasetIndex{0}, IDAWorkspaceIndex{0});

    TS_ASSERT_EQUALS(
        model->getFittingRange(DatasetIndex{0}, IDAWorkspaceIndex{0}).first,
        1.2);
    TS_ASSERT_EQUALS(
        model->getFittingRange(DatasetIndex{0}, IDAWorkspaceIndex{0}).second,
        5.6);
  }

  void
  test_that_getFittingRange_returns_empty_range_when_provided_an_out_of_range_dataIndex() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 1);

    model->setStartX(1.2, DatasetIndex{0}, IDAWorkspaceIndex{0});
    model->setEndX(5.6, DatasetIndex{0}, IDAWorkspaceIndex{0});

    TS_ASSERT_EQUALS(
        model->getFittingRange(DatasetIndex{1}, IDAWorkspaceIndex{0}).first,
        0.0);
    TS_ASSERT_EQUALS(
        model->getFittingRange(DatasetIndex{1}, IDAWorkspaceIndex{0}).second,
        0.0);
  }

  void
  test_that_getFittingRange_returns_empty_range_when_there_are_zero_spectra() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 1);

    model->setStartX(1.2, DatasetIndex{0}, IDAWorkspaceIndex{0});
    model->setEndX(5.6, DatasetIndex{0}, IDAWorkspaceIndex{0});
    Spectra const emptySpec("");
    model->setSpectra(emptySpec, DatasetIndex{0});

    TS_ASSERT_EQUALS(
        model->getFittingRange(DatasetIndex{0}, IDAWorkspaceIndex{0}).first,
        0.0);
    TS_ASSERT_EQUALS(
        model->getFittingRange(DatasetIndex{0}, IDAWorkspaceIndex{0}).second,
        0.0);
  }

  void
  test_that_getExcludeRegion_returns_correct_range_when_provided_a_valid_index_and_spectrum() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 1);

    model->setExcludeRegion("0,1,3,4", DatasetIndex{0}, IDAWorkspaceIndex{0});

    TS_ASSERT_EQUALS(
        model->getExcludeRegion(DatasetIndex{0}, IDAWorkspaceIndex{0}),
        "0.000,1.000,3.000,4.000");
  }

  void
  test_that_getExcludeRegion_returns_empty_range_when_provided_an_out_of_range_dataIndex() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 1);

    model->setExcludeRegion("0,1,3,4", DatasetIndex{0}, IDAWorkspaceIndex{0});

    TS_ASSERT_EQUALS(
        model->getExcludeRegion(DatasetIndex{1}, IDAWorkspaceIndex{0}), "");
  }

  void
  test_that_getExcludeRegion_returns_empty_range_when_there_are_zero_spectra() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 1);

    model->setExcludeRegion("0,1,3,4", DatasetIndex{0}, IDAWorkspaceIndex{0});
    Spectra const emptySpec("");
    model->setSpectra(emptySpec, DatasetIndex{0});

    TS_ASSERT_EQUALS(
        model->getExcludeRegion(DatasetIndex{1}, IDAWorkspaceIndex{0}), "");
  }

  void
  test_that_getExcludeRegion_returns_a_region_where_each_range_is_in_order_after_setExcludeRegion_is_given_an_unordered_region_string() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 1);

    model->setExcludeRegion("0,1,6,4", DatasetIndex{0}, IDAWorkspaceIndex{0});

    TS_ASSERT_EQUALS(
        model->getExcludeRegion(DatasetIndex{0}, IDAWorkspaceIndex{0}),
        "0.000,1.000,4.000,6.000");
  }

  void
  test_that_createDisplayName_returns_valid_string_when_provided_an_in_range_dataIndex() {
    auto const model = createModelWithSingleWorkspace("WorkspaceName", 1);

    std::string const formatString = "%1%_s%2%_Gaussian";
    std::string const rangeDelimiter = "_to_";

    TS_ASSERT_EQUALS(
        model->createOutputName(formatString, rangeDelimiter, DatasetIndex{0}),
        "WorkspaceName_s0_Gaussian_Results");
  }

  void
  test_that_createDisplayName_returns_string_with_red_removed_from_the_workspace_name() {
    auto const model = createModelWithSingleWorkspace("Workspace_3456_red", 1);

    std::string const formatString = "%1%_s%2%_Gaussian";
    std::string const rangeDelimiter = "_to_";

    TS_ASSERT_EQUALS(
        model->createOutputName(formatString, rangeDelimiter, DatasetIndex{0}),
        "Workspace_3456_s0_Gaussian_Results");
  }

  void
  test_that_createDisplayName_returns_correct_name_when_provided_a_valid_rangeDelimiter_and_formatString() {
    auto const model = createModelWithSingleWorkspace("Workspace_3456_red", 1);

    std::vector<std::string> const formatStrings{
        "%1%_s%2%_Gaussian", "%1%_f%2%,s%2%_MSD", "%1%_s%2%_TeixeiraWater"};
    std::string const rangeDelimiter = "_to_";

    TS_ASSERT_EQUALS(model->createOutputName(formatStrings[0], rangeDelimiter,
                                             DatasetIndex{0}),
                     "Workspace_3456_s0_Gaussian_Results");
    TS_ASSERT_EQUALS(model->createOutputName(formatStrings[1], rangeDelimiter,
                                             DatasetIndex{0}),
                     "Workspace_3456_f0+s0_MSD_Results");
    TS_ASSERT_EQUALS(model->createOutputName(formatStrings[2], rangeDelimiter,
                                             DatasetIndex{0}),
                     "Workspace_3456_s0_TeixeiraWater_Results");
  }

  void
  test_that_isMultiFit_returns_true_when_there_are_more_than_one_workspaces_stored_in_the_model() {
    auto const model =
        createModelWithMultipleWorkspaces(3, "Workspace1", "Workspace2");
    TS_ASSERT(model->isMultiFit());
  }

  void
  test_that_isMultiFit_returns_false_when_there_is_one_workspace_stored_in_the_model() {
    auto const model = createModelWithSingleWorkspace("Workspace1", 1);
    TS_ASSERT(!model->isMultiFit());
  }

  void
  test_that_isPreviouslyFit_returns_false_if_there_is_no_previous_fit_output_data() {
    auto const model = createModelWithSingleWorkspace("WorkspaceName", 1);
    TS_ASSERT(!model->isPreviouslyFit(DatasetIndex{0}, IDAWorkspaceIndex{0}));
  }

  void
  test_that_isPreviouslyFit_returns_false_if_the_dataIndex_is_out_of_range() {
    auto const model = createModelWithSingleWorkspace("WorkspaceName", 1);
    TS_ASSERT(!model->isPreviouslyFit(DatasetIndex{4}, IDAWorkspaceIndex{0}));
  }

  void
  test_that_setFitFunction_will_alter_the_activeFunction_to_the_function_specified() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 3);

    auto const function = getFunction("name=Convolution;name=Resolution");
    model->setFitFunction(function);

    TS_ASSERT_EQUALS(model->getFittingFunction(), function);
  }

  void test_that_ConvolutionSequentialFit_algorithm_initializes() {
    auto model = createModelWithSingleInstrumentWorkspace("Name", 6, 5);
    auto const modelWorkspace = model->getWorkspace(DatasetIndex{0});

    auto const alg = getSetupFitAlgorithm(model, modelWorkspace, "Name");

    TS_ASSERT(alg->isInitialized());
  }

  void test_that_ConvolutionSequentialFit_algorithm_executes_without_error() {
    auto model = createModelWithSingleInstrumentWorkspace("Name", 6, 5);
    auto const modelWorkspace = model->getWorkspace(DatasetIndex{0});

    auto const alg = getSetupFitAlgorithm(model, modelWorkspace, "Name");

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
  }

  void test_that_addOutput_adds_the_output_of_a_fit_into_the_model() {
    auto model = createModelWithSingleInstrumentWorkspace("__ConvFit", 6, 5);
    auto const modelWorkspace = model->getWorkspace(DatasetIndex{0});

    auto const alg =
        getExecutedFitAlgorithm(model, modelWorkspace, "__ConvFit");
    model->addOutput(alg);

    TS_ASSERT(model->getResultWorkspace());
    TS_ASSERT(model->getResultGroup());
  }

  void
  test_that_isInvalidFunction_returns_a_message_when_no_activeFunction_exists() {
    auto const model = createModelWithSingleWorkspace("WorkspaceName", 1);
    TS_ASSERT(model->isInvalidFunction());
  }

  void
  test_that_isInvalidFunction_returns_a_message_when_the_activeFunction_contains_zero_parameters_or_functions() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 3);

    auto const function = getFunction("name=Convolution;name=Resolution");
    model->setFitFunction(function);

    TS_ASSERT(model->isInvalidFunction());
  }

  void test_isInvalidFunction_returns_none_if_the_activeFunction_is_valid() {
    auto model = createModelWithSingleInstrumentWorkspace("Name", 6, 5);
    auto const modelWorkspace = model->getWorkspace(DatasetIndex{0});

    (void)getSetupFitAlgorithm(model, modelWorkspace, "Name");

    TS_ASSERT(!model->isInvalidFunction());
  }

  void
  test_that_numberOfWorkspace_returns_the_number_of_workspace_stored_by_model() {
    auto const model = createModelWithMultipleWorkspaces(
        3, "Workspace1", "Workspace2", "Workspace3");
    TS_ASSERT_EQUALS(model->numberOfWorkspaces(), DatasetIndex{3});
  }

  void test_that_getNumberOfSpectra_throws_if_dataIndex_is_out_of_range() {
    auto const model = createModelWithSingleWorkspace("WorkspaceName", 3);
    TS_ASSERT_THROWS(model->getNumberOfSpectra(DatasetIndex{1}),
                     const std::runtime_error &);
  }

  void
  test_that_getNumberOfSpectra_returns_the_number_of_spectra_stored_in_the_workspace_given() {
    auto const model = createModelWithSingleWorkspace("WorkspaceName", 3);
    TS_ASSERT_EQUALS(model->getNumberOfSpectra(DatasetIndex{0}),
                     SpectrumRowIndex{3});
  }

  void
  test_that_getFitParameterNames_returns_an_empty_vector_if_the_fitOutput_is_empty() {
    auto const model = createModelWithSingleWorkspace("WorkspaceName", 3);
    TS_ASSERT_EQUALS(model->getFitParameterNames(), std::vector<std::string>());
  }

  void
  test_that_getFitParameterNames_returns_a_vector_of_fit_parameters_if_the_fitOutput_contains_parameters() {
    auto model = createModelWithSingleInstrumentWorkspace("__ConvFit", 6, 5);
    auto const modelWorkspace = model->getWorkspace(DatasetIndex{0});

    auto const alg =
        getExecutedFitAlgorithm(model, modelWorkspace, "__ConvFit");
    model->addOutput(alg);

    TS_ASSERT(!model->getFitParameterNames().empty());
  }

  void test_getFittingFunction_returns_null_if_there_is_no_fitting_function() {
    auto const model = createModelWithSingleWorkspace("WorkspaceName", 3);
    TS_ASSERT_EQUALS(model->getFittingFunction(), nullptr);
  }

  void
  test_that_setFittingData_will_set_the_fittingData_to_the_data_provided() {
    auto const model = createModelWithSingleWorkspace("WorkspaceName", 3);
    TS_ASSERT_THROWS_NOTHING(model->setFittingData(model->clearWorkspaces()));
  }

  void
  test_that_setSpectra_will_set_the_spectra_to_the_provided_inputSpectra() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 10);

    Spectra const inputSpectra = Spectra("2,4,6-8");
    model->setSpectra(inputSpectra, DatasetIndex{0});
    Spectra const spectra = model->getSpectra(DatasetIndex{0});

    TS_ASSERT_EQUALS(spectra, inputSpectra);
  }

  void
  test_that_setSpectra_will_set_the_spectra_when_provided_a_spectra_pair() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 10);

    Spectra const inputSpectra =
        Spectra(IDAWorkspaceIndex{0}, IDAWorkspaceIndex{5});
    model->setSpectra(inputSpectra, DatasetIndex{0});
    Spectra const spectra = model->getSpectra(DatasetIndex{0});

    TS_ASSERT_EQUALS(spectra, inputSpectra);
  }

  void
  test_that_setSpectra_does_not_throw_when_provided_an_out_of_range_dataIndex() {
    auto const model = createModelWithSingleWorkspace("WorkspaceName", 5);
    TS_ASSERT_THROWS_NOTHING(model->getSpectra(DatasetIndex{1}));
  }

  void
  test_setStartX() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 5);

    model->setStartX(4.0, DatasetIndex{0}, IDAWorkspaceIndex{3});

    TS_ASSERT_EQUALS(
        model->getFittingRange(DatasetIndex{0}, IDAWorkspaceIndex{0}).first,
        0.0);
    TS_ASSERT_EQUALS(
        model->getFittingRange(DatasetIndex{0}, IDAWorkspaceIndex{3}).first,
        4.0);
  }

  void
  test_setEndX() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 5);

    model->setEndX(4.0, DatasetIndex{0}, IDAWorkspaceIndex{3});

    TS_ASSERT_EQUALS(
        model->getFittingRange(DatasetIndex{0}, IDAWorkspaceIndex{0}).second,
        10.0);
    TS_ASSERT_EQUALS(
        model->getFittingRange(DatasetIndex{0}, IDAWorkspaceIndex{3}).second,
        4.0);
  }

  void
  test_setExcludeRegion() {
    auto model = createModelWithSingleWorkspace("WorkspaceName", 5);

    model->setExcludeRegion("0,1,3,4", DatasetIndex{0}, IDAWorkspaceIndex{3});

    TS_ASSERT_EQUALS(
        model->getExcludeRegion(DatasetIndex{0}, IDAWorkspaceIndex{0}),
        "");
    TS_ASSERT_EQUALS(
        model->getExcludeRegion(DatasetIndex{0}, IDAWorkspaceIndex{3}),
        "0.000,1.000,3.000,4.000");
  }

  void
  test_that_removeWorkspace_will_remove_the_workspace_specified_in_the_model() {
    auto model = createModelWithMultipleWorkspaces(3, "Ws1", "Ws2", "Ws3");

    model->removeWorkspace(DatasetIndex{2});

    TS_ASSERT(model->getWorkspace(DatasetIndex{0}));
    TS_ASSERT(model->getWorkspace(DatasetIndex{1}));
    TS_ASSERT(!model->getWorkspace(DatasetIndex{2}));
  }

  void
  test_that_removeWorkspace_throws_when_provided_an_out_of_range_dataIndex() {
    auto model = createModelWithMultipleWorkspaces(3, "Ws1", "Ws2");
    TS_ASSERT_THROWS(model->removeWorkspace(DatasetIndex{2}),
                     const std::runtime_error &);
  }

  void test_that_clearWorkspaces_will_empty_the_fittingData() {
    auto model = createModelWithMultipleWorkspaces(3, "Ws1", "Ws2");

    model->clearWorkspaces();

    TS_ASSERT(!model->getWorkspace(DatasetIndex{0}));
    TS_ASSERT(!model->getWorkspace(DatasetIndex{1}));
    TS_ASSERT_EQUALS(model->numberOfWorkspaces(), DatasetIndex{0});
  }

  void
  test_that_setDefaultParameterValue_will_set_the_value_of_the_provided_parameter() {
    auto model = createModelWithSingleWorkspace("Name", 5);
    auto const modelWorkspace = model->getWorkspace(DatasetIndex{0});

    (void)getSetupFitAlgorithm(model, modelWorkspace, "Name");
    model->setDefaultParameterValue("Amplitude", 1.5, DatasetIndex{0});

    auto const parameters = model->getDefaultParameters(DatasetIndex{0});
    TS_ASSERT_EQUALS(parameters.at("f0.f1.f1.f0.Amplitude").value, 1.5);
  }

  void
  test_that_getParameterValues_returns_an_empty_map_if_the_dataIndex_is_out_of_range() {
    auto const model = getModelWithFitOutputData();
    TS_ASSERT(model->getParameterValues(DatasetIndex{1}, IDAWorkspaceIndex{0})
                  .empty());
  }

  void
  test_that_getParameterValues_returns_the_default_parameters_if_there_are_no_fit_parameters() {
    auto model = createModelWithSingleInstrumentWorkspace("__ConvFit", 6, 5);
    auto const modelWorkspace = model->getWorkspace(DatasetIndex{0});

    (void)getSetupFitAlgorithm(model, modelWorkspace, "__ConvFit");
    model->setDefaultParameterValue("Amplitude", 1.5, DatasetIndex{0});

    auto const parameters =
        model->getParameterValues(DatasetIndex{0}, IDAWorkspaceIndex{0});
    TS_ASSERT_EQUALS(parameters.at("f0.f1.f1.f0.Amplitude").value, 1.5);
  }

  void
  test_that_getParameterValues_returns_the_fit_parameters_after_a_fit_has_been_executed() {
    auto const model = getModelWithFitOutputData();

    auto const parameters =
        model->getParameterValues(DatasetIndex{0}, IDAWorkspaceIndex{0});
    TS_ASSERT_DELTA(parameters.at("f1.f1.f0.Amplitude").value, 1.0, 0.0001);
    TS_ASSERT_DELTA(parameters.at("f1.f1.f0.FWHM").value, 0.0175, 0.0001);
    TS_ASSERT(!parameters.empty());
  }

  void test_getFitParameters_returns_an_empty_map_when_there_is_no_fitOutput() {
    auto model = createModelWithSingleInstrumentWorkspace("__ConvFit", 6, 5);
    auto const modelWorkspace = model->getWorkspace(DatasetIndex{0});

    (void)getSetupFitAlgorithm(model, modelWorkspace, "__ConvFit");

    TS_ASSERT(
        model->getFitParameters(DatasetIndex{0}, IDAWorkspaceIndex{0}).empty());
  }

  void test_getFitParameters_returns_the_fitParameters_after_a_fit() {
    auto const model = getModelWithFitOutputData();

    auto const parameters =
        model->getFitParameters(DatasetIndex{0}, IDAWorkspaceIndex{0});
    TS_ASSERT_DELTA(parameters.at("f1.f1.f0.Amplitude").value, 1.0, 0.0001);
    TS_ASSERT_DELTA(parameters.at("f1.f1.f0.FWHM").value, 0.0175, 0.0001);
    TS_ASSERT(!parameters.empty());
  }

  void
  test_getDefaultParameters_returns_an_empty_map_when_the_dataIndex_is_out_of_range() {
    auto const model = getModelWithFitOutputData();
    TS_ASSERT(model->getDefaultParameters(DatasetIndex{1}).empty());
  }

  void
  test_getDefaultParameters_returns_the_default_parameters_which_have_been_set() {
    auto const model = getModelWithFitOutputData();

    model->setDefaultParameterValue("Amplitude", 1.5, DatasetIndex{0});

    auto const parameters = model->getDefaultParameters(DatasetIndex{0});
    TS_ASSERT(!parameters.empty());
    TS_ASSERT_DELTA(parameters.at("f0.f1.f1.f0.Amplitude").value, 1.5, 0.0001);
  }

  void test_that_getResultLocation_returns_a_location_for_the_output_data() {
    auto const model = getModelWithFitOutputData();
    TS_ASSERT(model->getResultLocation(DatasetIndex{0}, IDAWorkspaceIndex{0}));
  }

  void
  test_that_cleanFailedRun_removes_the_temporary_workspace_from_the_ADS_when_a_fit_fails() {
    /// Fails the fit algorithm on purpose by providing an invalid function
    auto model = createModelWithSingleInstrumentWorkspace("Name", 6, 5);
    auto const modelWorkspace = model->getWorkspace(DatasetIndex{0});
    SetUpADSWithWorkspace ads("Name", modelWorkspace);

    std::string const functionString =
        "name=Convolution;name=Resolution,Workspace=Name,WorkspaceIndex=0;";
    auto alg = setupFitAlgorithm(modelWorkspace, functionString);
    alg->execute();

    TS_ASSERT(ads.doesExist("__ConvolutionFitSequential_ws1"));
    model->cleanFailedRun(alg);
    TS_ASSERT(!ads.doesExist("__ConvolutionFitSequential_ws1"));
  }

  void
  test_that_cleanFailedSingleRun_removes_the_temporary_workspace_from_the_ADS_when_a_fit_fails_for_a_specific_workspaceIndex() {
    /// Fails the fit algorithm on purpose by providing an invalid function
    auto model = createModelWithSingleInstrumentWorkspace("Name", 6, 5);
    auto const modelWorkspace = model->getWorkspace(DatasetIndex{0});
    SetUpADSWithWorkspace ads("Name", modelWorkspace);

    std::string const functionString =
        "name=Convolution;name=Resolution,Workspace=Name,WorkspaceIndex=0;";
    auto alg = setupFitAlgorithm(modelWorkspace, functionString);
    alg->execute();

    TS_ASSERT(ads.doesExist("__ConvolutionFitSequential_ws1"));
    model->cleanFailedSingleRun(alg, DatasetIndex{0});
    TS_ASSERT(!ads.doesExist("__ConvolutionFitSequential_ws1"));
  }
};

#endif // MANTID_INDIRECTFITTINGMODELTEST_H