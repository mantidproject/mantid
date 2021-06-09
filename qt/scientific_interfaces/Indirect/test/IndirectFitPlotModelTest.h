// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <utility>

#include "ConvFitModel.h"
#include "IndirectFitPlotModel.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TextAxis.h"
#include "MantidCurveFitting/Algorithms/ConvolutionFit.h"
#include "MantidCurveFitting/Algorithms/QENSFitSequential.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;

using ConvolutionFitSequential = Algorithms::ConvolutionFit<Algorithms::QENSFitSequential>;

namespace {

auto &ads_instance = Mantid::API::AnalysisDataService::Instance();

/// The name of the conjoined input and guess workspaces
std::string const INPUT_AND_GUESS_NAME = "__QENSInputAndGuess";

// Resolution
const std::vector<std::pair<std::string, size_t>>
    exampleResolution(1, std::make_pair<std::string, size_t>("irs26173_graphite_002_res", 0));

std::string getFitFunctionString(std::string const &workspaceName) {
  return "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
         "(composite=Convolution,FixResolution=true,NumDeriv=true;"
         "name=Resolution,Workspace=" +
         workspaceName +
         ",WorkspaceIndex=0;((composite=ProductFunction,NumDeriv="
         "false;name=Lorentzian,Amplitude=1,PeakCentre=1,FWHM=0."
         "0175)))";
}

MultiDomainFunction_sptr getFunction(std::string const &functionString, size_t numDomains) {

  return FunctionFactory::Instance().createInitializedMultiDomainFunction(functionString, numDomains);
}

/// A dummy class used to create a model to pass to IndirectFitPlotModel's
/// constructor
class DummyModel : public MantidQt::CustomInterfaces::IDA::IndirectFittingModel {
public:
  ~DummyModel(){};

private:
  std::string sequentialFitOutputName() const override { return ""; };
  std::string simultaneousFitOutputName() const override { return ""; };
  std::string singleFitOutputName(WorkspaceID workspaceID, WorkspaceIndex spectrum) const override {
    UNUSED_ARG(index);
    UNUSED_ARG(spectrum);
    return "";
  };
};
/// A dummy empty Conv class used to create a model to pass to
/// IndirectFitPlotModel's constructor
class DummyEmptyConvModel : public MantidQt::CustomInterfaces::IDA::ConvFitModel {
public:
  ~DummyEmptyConvModel() {}

  std::vector<std::pair<std::string, size_t>> getResolutionsForFit() const override { return m_resolutions; };

private:
  std::vector<std::pair<std::string, size_t>> m_resolutions;
};

/// A dummy Conv class (with resolution data) used to create a model to pass to
/// IndirectFitPlotModel's constructor
class DummyConvModel : public MantidQt::CustomInterfaces::IDA::ConvFitModel {
public:
  DummyConvModel() : m_resolutions(exampleResolution) {}
  ~DummyConvModel() {}

  std::vector<std::pair<std::string, size_t>> getResolutionsForFit() const override { return m_resolutions; };

private:
  std::vector<std::pair<std::string, size_t>> m_resolutions;
};

void setFittingFunction(IndirectFittingModel *model, std::string const &functionString, bool setFitFunction,
                        size_t numDomains) {
  if (setFitFunction)
    model->setFitFunction(getFunction(functionString, numDomains));
}

IndirectFittingModel *getEmptyDummyModel() { return new DummyModel(); }

void addWorkspaceToModel(IndirectFittingModel *model, int const &numberOfSpectra, std::string workspaceName) {
  ads_instance.addOrReplace(workspaceName, createWorkspace(numberOfSpectra));
  model->addWorkspace(workspaceName);
}
template <class FitModel>
IndirectFittingModel *createModelWithMultipleWorkspaces(int const &numberOfSpectra, bool setFitFunction,
                                                        const std::vector<std::string> &workspaceNames) {
  auto model = new FitModel;
  for (auto name : workspaceNames) {
    addWorkspaceToModel(model, numberOfSpectra, name);
  }
  setFittingFunction(model, getFitFunctionString(workspaceNames[0]), setFitFunction,
                     numberOfSpectra * workspaceNames.size());
  return model;
}

IndirectFittingModel *createModelWithSingleInstrumentWorkspace(std::string const &workspaceName, int const &xLength,
                                                               int const &yLength) {
  auto model = getEmptyDummyModel();
  SetUpADSWithWorkspace ads(workspaceName, createWorkspaceWithInstrument(xLength, yLength));
  model->addWorkspace(workspaceName);
  return model;
}

IAlgorithm_sptr setupFitAlgorithm(const MatrixWorkspace_sptr &workspace, std::string const &functionString) {
  auto alg = std::make_shared<ConvolutionFitSequential>();
  alg->initialize();
  alg->setProperty("InputWorkspace", workspace);
  alg->setProperty("Function", functionString);
  alg->setProperty("StartX", "0.0");
  alg->setProperty("EndX", "3.0");
  alg->setProperty("SpecMin", 0);
  alg->setProperty("SpecMax", 5);
  alg->setProperty("ConvolveMembers", true);
  alg->setProperty("Minimizer", "Levenberg-Marquardt");
  alg->setProperty("MaxIterations", 500);
  alg->setProperty("OutputWorkspace", "OutputResults");
  alg->setLogging(false);
  return alg;
}

IAlgorithm_sptr getSetupFitAlgorithm(IndirectFittingModel *model, const MatrixWorkspace_sptr &workspace,
                                     std::string const &workspaceName) {
  setFittingFunction(model, getFitFunctionString(workspaceName), true, 20);
  auto alg = setupFitAlgorithm(std::move(workspace), getFitFunctionString(workspaceName));
  return alg;
}

IAlgorithm_sptr getExecutedFitAlgorithm(IndirectFittingModel *model, MatrixWorkspace_sptr workspace,
                                        std::string const &workspaceName) {
  auto const alg = getSetupFitAlgorithm(model, std::move(workspace), workspaceName);
  alg->execute();
  return alg;
}

IndirectFittingModel *getModelWithFitOutputData() {
  auto model = createModelWithSingleInstrumentWorkspace("__ConvFit", 6, 5);
  auto const modelWorkspace = model->getWorkspace(WorkspaceID{0});

  auto const alg = getExecutedFitAlgorithm(model, modelWorkspace, "__ConvFit");
  model->addOutput(alg);
  return model;
}

template <class FitModel = DummyModel> IndirectFitPlotModel getFitPlotModel(bool setFitFunction = true) {
  return IndirectFitPlotModel(createModelWithMultipleWorkspaces<FitModel>(
      10, setFitFunction, std::vector<std::string>({"Workspace1", "Workspace2"})));
}

IndirectFitPlotModel getFitPlotModelWithFitData() { return IndirectFitPlotModel(getModelWithFitOutputData()); }

} // namespace

class IndirectFitPlotModelTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  IndirectFitPlotModelTest() { FrameworkManager::Instance(); }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_that_IndirectFittingModel_instantiates_a_model_with_the_correct_starting_member_variables() {
    auto const model = getFitPlotModel();

    TS_ASSERT_EQUALS(model.getActiveDataIndex(), WorkspaceID{0});
    TS_ASSERT_EQUALS(model.getActiveSpectrum(), WorkspaceIndex{0});
    TS_ASSERT_EQUALS(model.numberOfWorkspaces(), WorkspaceID{2});
  }

  void test_that_getWorkspace_returns_a_workspace_with_the_correct_number_of_spectra() {
    auto const model = getFitPlotModel();
    TS_ASSERT_EQUALS(model.getWorkspace()->getNumberHistograms(), 10);
  }

  void test_that_getGuessWorkspace_will_create_and_then_return_a_guess_workspace_with_the_correct_number_of_spectra() {
    /// Only creates a guess for the active spectra of the selected workspace
    auto const model = getFitPlotModel();

    TS_ASSERT(model.getGuessWorkspace());
    TS_ASSERT_EQUALS(model.getGuessWorkspace()->getNumberHistograms(), 1);
  }

  void test_that_getGuessWorkspace_returns_a_workspace_with_the_correct_range() {
    auto model = getFitPlotModel();
    model.setStartX(3);
    model.setEndX(8);

    TS_ASSERT_EQUALS(model.getGuessWorkspace()->x(0)[0], 3);
    TS_ASSERT_EQUALS(model.getGuessWorkspace()->x(0).back(), 8);
  }

  void test_that_getResultWorkspace_returns_a_nullptr_if_a_fit_has_not_yet_been_run() {
    auto const model = getFitPlotModel();
    TS_ASSERT(!model.getResultWorkspace());
  }

  void test_that_getResultWorkspace_returns_a_workspace_when_data_has_been_fit() {
    auto const model = getFitPlotModelWithFitData();
    TS_ASSERT(model.getResultWorkspace());
  }

  void test_that_getSpectra_returns_the_same_spectra_range_which_was_provided_as_input() {
    auto const model = getFitPlotModel();

    FunctionModelSpectra const spectra = FunctionModelSpectra(WorkspaceIndex{0}, WorkspaceIndex{9});
    FunctionModelSpectra const storedSpectra = model.getSpectra();

    TS_ASSERT_EQUALS(storedSpectra, spectra);
  }

  void test_that_appendGuessToInput_returns_a_workspace_that_is_the_combination_of_the_input_and_guess_workspaces() {
    auto const model = getFitPlotModel();
    auto const guess = model.getGuessWorkspace();

    auto const resultWorkspace = model.appendGuessToInput(guess);

    TS_ASSERT(AnalysisDataService::Instance().doesExist(INPUT_AND_GUESS_NAME));
    TS_ASSERT_EQUALS(resultWorkspace->getAxis(1)->label(0), "Sample");
    TS_ASSERT_EQUALS(resultWorkspace->getAxis(1)->label(1), "Guess");
    /// Only two spectra because the guessWorkspace will only ever have one
    /// spectra, and then spectra are extracted from the input workspace
    /// between
    /// m_activeSpectrum and m_activeSpectrum and so only 1 spectrum is
    /// extracted. 1 + 1 = 2
    TS_ASSERT_EQUALS(resultWorkspace->getNumberHistograms(), 2);
  }

  void test_that_getActiveDataIndex_returns_the_index_which_it_has_been_set_to() {
    auto model = getFitPlotModel();

    model.setActiveIndex(WorkspaceID{2});

    TS_ASSERT_EQUALS(model.getActiveDataIndex(), WorkspaceID{2});
  }

  void test_that_getActiveSpectrum_returns_the_spectrum_which_it_has_been_set_to() {
    auto model = getFitPlotModel();

    model.setActiveSpectrum(WorkspaceIndex{3});

    TS_ASSERT_EQUALS(model.getActiveSpectrum(), WorkspaceIndex{3});
  }

  void test_that_getFitDataName_returns_the_correctly_calculated_name() {
    auto const model = getFitPlotModel();

    TS_ASSERT_EQUALS(model.getFitDataName(), "Workspace1 (0-9)");
    TS_ASSERT_EQUALS(model.getFitDataName(WorkspaceID{1}), "Workspace2 (0-9)");
  }

  void test_that_getFitDataName_does_not_throw_when_provided_an_out_of_range_index() {
    auto const model = getFitPlotModel();
    TS_ASSERT_THROWS_NOTHING(model.getFitDataName(WorkspaceID{10000000}));
  }

  void test_that_getLastFitDataName_returns_the_name_for_the_last_workspace_in_the_model() {
    auto const model = getFitPlotModel();
    TS_ASSERT_EQUALS(model.getLastFitDataName(), "Workspace2 (0-9)");
  }

  void test_that_getRange_returns_the_range_which_is_set() {
    auto model = getFitPlotModel();

    model.setStartX(2.2);
    model.setEndX(8.8);

    TS_ASSERT_EQUALS(model.getRange().first, 2.2);
    TS_ASSERT_EQUALS(model.getRange().second, 8.8);
  }

  void test_that_setStartX_does_not_set_the_StartX_when_the_provided_value_is_larger_than_the_EndX() {
    auto model = getFitPlotModel();

    model.setEndX(2.2);
    model.setStartX(8.8);

    TS_ASSERT_EQUALS(model.getRange().first, 0.0);
    TS_ASSERT_EQUALS(model.getRange().second, 2.2);
  }

  void test_that_setEndX_does_not_set_the_EndX_when_the_provided_value_is_smaller_than_the_StartX() {
    auto model = getFitPlotModel();

    model.setStartX(8.8);
    model.setEndX(2.2);

    TS_ASSERT_EQUALS(model.getRange().first, 8.8);
    TS_ASSERT_EQUALS(model.getRange().second, 10.0);
  }

  void test_that_getWorkspaceRange_returns_the_defaulted_values_before_a_fit() {
    auto const model = getFitPlotModel();

    TS_ASSERT_EQUALS(model.getWorkspaceRange().first, 0.0);
    TS_ASSERT_EQUALS(model.getWorkspaceRange().second, 10.0);
  }

  void test_that_getResultRange_returns_the_different_values_to_the_values_before_the_fit() {
    auto const model = getFitPlotModelWithFitData();

    TS_ASSERT_DIFFERS(model.getResultRange().first, 0.0);
    TS_ASSERT_DIFFERS(model.getResultRange().second, 10.0);
  }

  void test_that_getFirstHWHM_returns_half_the_value_of_the_FWHM_in_the_fitting_function() {
    auto const model = getFitPlotModel();
    TS_ASSERT_EQUALS(model.getFirstHWHM(), 0.0175 / 2);
  }

  void test_that_getFirstPeakCentre_returns_the_value_of_the_first_PeakCentre_in_the_fitting_function() {
    auto const model = getFitPlotModel();
    TS_ASSERT_EQUALS(model.getFirstPeakCentre(), 1.0);
  }

  void test_that_getFirstBackgroundLevel_returns_the_value_of_the_first_background_level_in_the_fitting_function() {
    auto const model = getFitPlotModel();
    TS_ASSERT_EQUALS(model.getFirstBackgroundLevel(), 0.0);
  }

  void test_that_calculateHWHMMaximum_returns_the_value_expected() {
    auto const model = getFitPlotModel();

    auto const hwhm = model.getFirstHWHM();
    auto const peakCentre = model.getFirstPeakCentre().get_value_or(0.);

    auto const minimum = peakCentre + *hwhm;
    TS_ASSERT_EQUALS(model.calculateHWHMMaximum(minimum), 0.99125);
  }

  void test_that_calculateHWHMMinimum_returns_the_value_expected() {
    auto const model = getFitPlotModel();

    auto const hwhm = model.getFirstHWHM();
    auto const peakCentre = model.getFirstPeakCentre().get_value_or(0.);

    auto const maximum = peakCentre - *hwhm;
    TS_ASSERT_EQUALS(model.calculateHWHMMinimum(maximum), 1.00875);
  }

  void test_that_canCalculateGuess_returns_false_when_there_is_no_fitting_function() {
    auto const model = getFitPlotModel(false);
    TS_ASSERT(!model.canCalculateGuess());
  }

  void test_that_canCalculateGuess_returns_false_when_required_resolution_not_loaded() {
    auto const model = getFitPlotModel<DummyEmptyConvModel>();
    TS_ASSERT(!model.canCalculateGuess());
  }

  void test_that_canCalculateGuess_returns_true_when_required_resolution_loaded() {
    auto const model = getFitPlotModel<DummyConvModel>();
    TS_ASSERT(model.canCalculateGuess());
  }

  void test_that_canCalculateGuess_returns_true_when_there_is_a_fitting_function_and_a_model_with_a_workspace() {
    auto const model = getFitPlotModel();
    TS_ASSERT(model.canCalculateGuess());
  }

  void test_that_setFWHM_will_change_the_value_of_the_FWHM_in_the_fitting_function() {
    auto model = getFitPlotModel();

    auto const fwhm = 1.1;
    model.setFWHM(fwhm);

    TS_ASSERT_EQUALS(model.getFirstHWHM(), fwhm / 2);
  }

  void test_that_setBackground_will_change_the_value_of_A0_in_the_fitting_function() {
    auto model = getFitPlotModel();

    auto const background = 0.12;
    model.setBackground(background);

    TS_ASSERT_EQUALS(model.getFirstBackgroundLevel(), background);
  }

  void test_that_deleteExternalGuessWorkspace_removes_the_guess_workspace_from_the_ADS() {
    auto model = getFitPlotModel();

    auto const guess = model.getGuessWorkspace();
    (void)model.appendGuessToInput(guess);

    TS_ASSERT(AnalysisDataService::Instance().doesExist(INPUT_AND_GUESS_NAME));
    model.deleteExternalGuessWorkspace();
    TS_ASSERT(!AnalysisDataService::Instance().doesExist(INPUT_AND_GUESS_NAME));
  }

  void test_that_deleteExternalGuessWorkspace_does_not_throw_if_the_guess_workspace_does_not_exist() {
    auto model = getFitPlotModel();
    TS_ASSERT_THROWS_NOTHING(model.deleteExternalGuessWorkspace());
  }
};
