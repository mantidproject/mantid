// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <utility>

#include "QENSFitting/ConvolutionModel.h"
#include "QENSFitting/FitOutput.h"
#include "QENSFitting/FitPlotModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TextAxis.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;

namespace {

auto &ads_instance = Mantid::API::AnalysisDataService::Instance();

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

IAlgorithm_sptr setupFitAlgorithm(const MatrixWorkspace_sptr &workspace, std::string const &functionString) {
  auto alg = AlgorithmManager::Instance().create("ConvolutionFitSequential");
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
  alg->setProperty("OutputWorkspace", "output");
  alg->setLogging(false);
  return alg;
}

template <typename WorkspaceType>
std::shared_ptr<WorkspaceType> getWorkspaceOutput(const IAlgorithm_sptr &algorithm, const std::string &propertyName) {
  return AnalysisDataService::Instance().retrieveWS<WorkspaceType>(algorithm->getProperty(propertyName));
}

} // namespace

class FitPlotModelTest : public CxxTest::TestSuite {
public:
  static FitPlotModelTest *createSuite() { return new FitPlotModelTest(); }

  static void destroySuite(FitPlotModelTest *suite) { delete suite; }

  void setUp() override {
    m_workspace = createWorkspaceWithInstrument(6, 5);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("Name", m_workspace);
    m_fittingData = std::make_unique<std::vector<FitData>>();
    m_fittingData->emplace_back(m_workspace, FunctionModelSpectra("0-5"));
    m_fitOutput = std::make_unique<FitOutput>();
    m_model = std::make_unique<FitPlotModel>(m_fittingData.get(), m_fitOutput.get());
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    m_model.reset();
    m_fittingData.reset();
    m_fitOutput.reset();
  }

  void test_that_FittingModel_instantiates_a_model_with_the_correct_starting_member_variables() {
    TS_ASSERT_EQUALS(m_model->getActiveWorkspaceID(), WorkspaceID{0});
    TS_ASSERT_EQUALS(m_model->getActiveWorkspaceIndex(), WorkspaceIndex{0});
    TS_ASSERT_EQUALS(m_model->numberOfWorkspaces(), WorkspaceID{1});
  }

  void test_that_getWorkspace_returns_a_workspace_with_the_correct_number_of_spectra() {
    TS_ASSERT_EQUALS(m_model->getWorkspace()->getNumberHistograms(), 6);
  }

  void test_that_getResultWorkspace_returns_a_nullptr_if_a_fit_has_not_yet_been_run() {
    TS_ASSERT(!m_model->getResultWorkspace());
  }

  void test_that_getResultWorkspace_returns_a_workspace_when_data_has_been_fit() {
    auto alg = setupFitAlgorithm(m_workspace, getFitFunctionString("Name"));
    alg->execute();
    auto group = getWorkspaceOutput<WorkspaceGroup>(alg, "OutputWorkspaceGroup");
    auto parameters = getWorkspaceOutput<ITableWorkspace>(alg, "OutputParameterWorkspace");
    auto result = getWorkspaceOutput<WorkspaceGroup>(alg, "OutputWorkspace");
    m_fitOutput->addOutput(group, parameters, result, FitDomainIndex{0});
    m_activeFunction = getFunction(getFitFunctionString("Name"), 1);
    m_model->setFitFunction(m_activeFunction);

    TS_ASSERT(m_model->getResultWorkspace());
  }

  void test_that_getGuessWorkspace_will_create_and_then_return_a_guess_workspace_with_the_correct_number_of_spectra() {
    /// Only creates a guess for the active spectra of the selected workspace
    m_activeFunction = getFunction(getFitFunctionString("Name"), 1);
    m_model->setFitFunction(m_activeFunction);
    TS_ASSERT(m_model->getGuessWorkspace());
    TS_ASSERT_EQUALS(m_model->getGuessWorkspace()->getNumberHistograms(), 1);
  }

  void test_that_getSpectra_returns_the_same_spectra_range_which_was_provided_as_input() {
    FunctionModelSpectra const spectra = FunctionModelSpectra(WorkspaceIndex{0}, WorkspaceIndex{5});
    FunctionModelSpectra const storedSpectra = m_model->getSpectra(WorkspaceID(0));

    TS_ASSERT_EQUALS(storedSpectra, spectra);
  }

  void test_that_getActiveWorkspaceID_returns_the_index_which_it_has_been_set_to() {

    m_model->setActiveIndex(WorkspaceID{2});

    TS_ASSERT_EQUALS(m_model->getActiveWorkspaceID(), WorkspaceID{2});
  }

  void test_that_getActiveWorkspaceIndex_returns_the_spectrum_which_it_has_been_set_to() {

    m_model->setActiveSpectrum(WorkspaceIndex{3});

    TS_ASSERT_EQUALS(m_model->getActiveWorkspaceIndex(), WorkspaceIndex{3});
  }

  void test_that_getActiveDomainIndex_returns_the_spectrum_which_it_has_been_set_to() {
    m_fittingData->emplace_back(m_workspace, FunctionModelSpectra("0-5"));

    m_model->setActiveIndex(WorkspaceID{1});
    m_model->setActiveSpectrum(WorkspaceIndex{3});

    TS_ASSERT_EQUALS(m_model->getActiveDomainIndex(), FitDomainIndex{9});
  }

  void test_that_getRange_returns_the_range_which_is_set() {
    m_fittingData->at(0).setStartX(2.2);
    m_fittingData->at(0).setEndX(8.8);

    TS_ASSERT_EQUALS(m_model->getRange().first, 2.2);
    TS_ASSERT_EQUALS(m_model->getRange().second, 8.8);
  }

  void test_that_setStartX_does_not_set_the_StartX_when_the_provided_value_is_larger_than_the_EndX() {
    m_fittingData->at(0).setEndX(5.0);
    m_fittingData->at(0).setStartX(6.0);

    TS_ASSERT_EQUALS(m_model->getRange().first, 5.0);
    TS_ASSERT_EQUALS(m_model->getRange().second, 5.0);
  }

  void test_that_setEndX_does_not_set_the_EndX_when_the_provided_value_is_smaller_than_the_StartX() {
    m_fittingData->at(0).setStartX(4.0);
    m_fittingData->at(0).setEndX(3.0);

    TS_ASSERT_EQUALS(m_model->getRange().first, 4.0);
    TS_ASSERT_EQUALS(m_model->getRange().second, 4.0);
  }

  void test_that_getWorkspaceRange_returns_the_defaulted_values_before_a_fit() {
    TS_ASSERT_EQUALS(m_model->getWorkspaceRange().first, 1.25);
    TS_ASSERT_EQUALS(m_model->getWorkspaceRange().second, 4.25);
  }

  void test_that_getResultRange_returns_the_different_values_to_the_values_before_the_fit() {
    auto alg = setupFitAlgorithm(m_workspace, getFitFunctionString("Name"));
    alg->execute();
    auto group = getWorkspaceOutput<WorkspaceGroup>(alg, "OutputWorkspaceGroup");
    auto parameters = getWorkspaceOutput<ITableWorkspace>(alg, "OutputParameterWorkspace");
    auto result = getWorkspaceOutput<WorkspaceGroup>(alg, "OutputWorkspace");
    m_fitOutput->addOutput(group, parameters, result, FitDomainIndex{0});
    m_activeFunction = getFunction(getFitFunctionString("Name"), 1);
    m_model->setFitFunction(m_activeFunction);

    TS_ASSERT_DIFFERS(m_model->getResultRange().first, 0.0);
    TS_ASSERT_DIFFERS(m_model->getResultRange().second, 10.0);
  }

  void test_that_getFirstHWHM_returns_half_the_value_of_the_FWHM_in_the_fitting_function() {
    m_activeFunction = getFunction(getFitFunctionString("Name"), 1);
    m_model->setFitFunction(m_activeFunction);
    TS_ASSERT_EQUALS(m_model->getFirstHWHM(), 0.0175 / 2);
  }

  void test_that_getFirstPeakCentre_returns_the_value_of_the_first_PeakCentre_in_the_fitting_function() {
    m_activeFunction = getFunction(getFitFunctionString("Name"), 1);
    m_model->setFitFunction(m_activeFunction);
    TS_ASSERT_EQUALS(m_model->getFirstPeakCentre(), 1.0);
  }

  void test_that_getFirstBackgroundLevel_returns_the_value_of_the_first_background_level_in_the_fitting_function() {
    m_activeFunction = getFunction(getFitFunctionString("Name"), 1);
    m_model->setFitFunction(m_activeFunction);
    TS_ASSERT_EQUALS(m_model->getFirstBackgroundLevel(), 0.0);
  }

  void test_that_calculateHWHMMaximum_returns_the_value_expected() {
    auto alg = setupFitAlgorithm(m_workspace, getFitFunctionString("Name"));
    alg->execute();
    auto group = getWorkspaceOutput<WorkspaceGroup>(alg, "OutputWorkspaceGroup");
    auto parameters = getWorkspaceOutput<ITableWorkspace>(alg, "OutputParameterWorkspace");
    auto result = getWorkspaceOutput<WorkspaceGroup>(alg, "OutputWorkspace");
    m_fitOutput->addOutput(group, parameters, result, FitDomainIndex{0});
    m_activeFunction = getFunction(getFitFunctionString("Name"), 1);
    m_model->setFitFunction(m_activeFunction);

    auto const hwhm = m_model->getFirstHWHM();
    auto const peakCentre = m_model->getFirstPeakCentre().value_or(0.);

    auto const minimum = peakCentre + *hwhm;
    TS_ASSERT_EQUALS(m_model->calculateHWHMMaximum(minimum), 0.99125);
  }

  void test_that_calculateHWHMMinimum_returns_the_value_expected() {
    auto alg = setupFitAlgorithm(m_workspace, getFitFunctionString("Name"));
    alg->execute();
    auto group = getWorkspaceOutput<WorkspaceGroup>(alg, "OutputWorkspaceGroup");
    auto parameters = getWorkspaceOutput<ITableWorkspace>(alg, "OutputParameterWorkspace");
    auto result = getWorkspaceOutput<WorkspaceGroup>(alg, "OutputWorkspace");
    m_fitOutput->addOutput(group, parameters, result, FitDomainIndex{0});
    m_activeFunction = getFunction(getFitFunctionString("Name"), 1);
    m_model->setFitFunction(m_activeFunction);

    auto const hwhm = m_model->getFirstHWHM();
    auto const peakCentre = m_model->getFirstPeakCentre().value_or(0.);

    auto const maximum = peakCentre - *hwhm;
    TS_ASSERT_EQUALS(m_model->calculateHWHMMinimum(maximum), 1.00875);
  }

  void test_that_canCalculateGuess_returns_false_when_there_is_no_fitting_function() {
    TS_ASSERT(!m_model->canCalculateGuess());
  }

  void test_that_canCalculateGuess_returns_true_when_there_is_a_fitting_function_and_a_model_with_a_workspace() {
    m_activeFunction = getFunction(getFitFunctionString("Name"), 1);
    m_model->setFitFunction(m_activeFunction);
    TS_ASSERT(m_model->canCalculateGuess());
  }

  void test_that_setFWHM_will_change_the_value_of_the_FWHM_in_the_fitting_function() {
    m_activeFunction = getFunction(getFitFunctionString("Name"), 1);
    m_model->setFitFunction(m_activeFunction);
    auto const fwhm = 0.0175;
    TS_ASSERT_EQUALS(*m_model->getFirstHWHM(), fwhm / 2);
  }

  void test_that_setBackground_will_change_the_value_of_A0_in_the_fitting_function() {
    m_activeFunction = getFunction(getFitFunctionString("Name"), 1);
    m_model->setFitFunction(m_activeFunction);
    auto const background = 0.0;
    TS_ASSERT_EQUALS(*m_model->getFirstBackgroundLevel(), background);
  }

  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  std::unique_ptr<FitPlotModel> m_model;
  std::unique_ptr<std::vector<FitData>> m_fittingData;
  std::unique_ptr<FitOutput> m_fitOutput;
  MultiDomainFunction_sptr m_activeFunction;
};
