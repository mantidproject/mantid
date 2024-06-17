// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <utility>

#include "QENSFitting/FittingModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;

namespace {

MultiDomainFunction_sptr getFunction(std::string const &functionString) {
  return FunctionFactory::Instance().createInitializedMultiDomainFunction(functionString, 1);
}

auto &ads_instance = Mantid::API::AnalysisDataService::Instance();

void setFittingFunction(std::unique_ptr<FittingModel> &model, std::string const &functionString) {
  model->setFitFunction(getFunction(functionString));
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

IAlgorithm_sptr getSetupFitAlgorithm(std::unique_ptr<FittingModel> &model, const MatrixWorkspace_sptr &workspace,
                                     std::string const &workspaceName) {
  std::string const function = "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
                               "(composite=Convolution,FixResolution=true,NumDeriv=true;"
                               "name=Resolution,Workspace=" +
                               workspaceName +
                               ",WorkspaceIndex=0;((composite=ProductFunction,NumDeriv="
                               "false;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0."
                               "0175)))";
  setFittingFunction(model, function);
  auto alg = setupFitAlgorithm(std::move(workspace), function);
  return alg;
}

IAlgorithm_sptr getExecutedFitAlgorithm(std::unique_ptr<FittingModel> &model, MatrixWorkspace_sptr workspace,
                                        std::string const &workspaceName) {
  auto const alg = getSetupFitAlgorithm(model, std::move(workspace), workspaceName);
  alg->execute();
  return alg;
}

} // namespace

class FittingModelTest : public CxxTest::TestSuite {
public:
  static FittingModelTest *createSuite() { return new FittingModelTest(); }

  static void destroySuite(FittingModelTest *suite) { delete suite; }

  void setUp() override { m_model = std::make_unique<FittingModel>(); }

  void tearDown() override {
    m_model.reset();
    AnalysisDataService::Instance().clear();
  }

  void test_model_is_instantiated_correctly() {
    TS_ASSERT(!m_model->getWorkspace(0));
    TS_ASSERT_EQUALS(m_model->getNumberOfWorkspaces(), 0);
  }

  void test_that_a_workspace_is_stored_correctly_in_the_ADS() {
    SetUpADSWithWorkspace ads("WorkspaceName", createWorkspace(3));

    TS_ASSERT(ads.doesExist("WorkspaceName"));
    auto const storedWorkspace = ads.retrieveWorkspace("WorkspaceName");
    TS_ASSERT_EQUALS(storedWorkspace->getNumberHistograms(), 3);
  }

  void test_that_getWorkspace_returns_a_nullptr_when_getWorkspace_is_provided_an_out_of_range_index() {
    addWorkspaceToModel("WorkspaceName", 3, "0-2");
    TS_ASSERT_EQUALS(m_model->getWorkspace(1), nullptr);
  }

  void test_that_isMultiFit_returns_true_when_there_are_more_than_one_workspaces_stored_in_the_model() {
    addWorkspaceToModel("Workspace1", 3, "0-2");
    addWorkspaceToModel("Workspace2", 3, "0-2");
    TS_ASSERT(m_model->isMultiFit());
  }

  void test_that_isMultiFit_returns_false_when_there_is_one_workspace_stored_in_the_model() {
    addWorkspaceToModel("Workspace1", 3, "0-2");
    TS_ASSERT(!m_model->isMultiFit());
  }

  void test_that_isPreviouslyFit_returns_false_if_there_is_no_previous_fit_output_data() {
    addWorkspaceToModel("WorkspaceName", 3, "0-2");
    TS_ASSERT(!m_model->isPreviouslyFit(0, 0));
  }

  void test_that_isPreviouslyFit_returns_false_if_the_dataIndex_is_out_of_range() {
    addWorkspaceToModel("WorkspaceName", 3, "0-2");
    TS_ASSERT(!m_model->isPreviouslyFit(4, 0));
  }

  void test_getFitFunction_returns_null_if_there_is_no_fitting_function() {
    TS_ASSERT_EQUALS(m_model->getFitFunction(), nullptr);
  }

  void test_that_setFitFunction_will_alter_the_activeFunction_to_the_function_specified() {

    auto const function = getFunction("name=Convolution;name=Resolution");
    m_model->setFitFunction(function);

    TS_ASSERT_EQUALS(m_model->getFitFunction(), function);
  }

  void test_that_ConvolutionSequentialFit_algorithm_initializes() {
    addInstrumentWorkspaceToModel("WorkspaceName", 6, 5, "0-5");
    auto const modelWorkspace = m_model->getWorkspace(0);

    auto const alg = getSetupFitAlgorithm(m_model, modelWorkspace, "WorkspaceName");

    TS_ASSERT(alg->isInitialized());
  }

  void test_that_ConvolutionSequentialFit_algorithm_executes_without_error() {
    addInstrumentWorkspaceToModel("WorkspaceName", 6, 5, "0-5");
    auto const modelWorkspace = m_model->getWorkspace(0);

    auto const alg = getSetupFitAlgorithm(m_model, modelWorkspace, "WorkspaceName");

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
  }

  void test_that_addOutput_adds_the_output_of_a_fit_into_the_model() {
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    auto const modelWorkspace = m_model->getWorkspace(0);

    auto const alg = getExecutedFitAlgorithm(m_model, modelWorkspace, "__Convolution");
    m_model->addOutput(alg);

    TS_ASSERT(m_model->getResultWorkspace());
    TS_ASSERT(m_model->getResultGroup());
  }

  void test_that_addOutput_adds_the_output_of_a_single_fit_into_the_model() {
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    auto const modelWorkspace = m_model->getWorkspace(0);

    auto const alg = getExecutedFitAlgorithm(m_model, modelWorkspace, "__Convolution");
    m_model->addOutput(alg);

    TS_ASSERT(m_model->getResultWorkspace());
    TS_ASSERT(m_model->getResultGroup());
  }

  void test_that_isPreviouslyFit_returns_true_if_the_spectrum_has_been_fitted_previously() {
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    addFitOutputDataToModel();
    TS_ASSERT(m_model->isPreviouslyFit(WorkspaceID(0), WorkspaceIndex(0)));
  }

  void test_that_isInvalidFunction_returns_a_message_when_no_activeFunction_exists() {
    addWorkspaceToModel("WorkspaceName", 3, "0-2");
    TS_ASSERT(m_model->isInvalidFunction());
  }

  void test_that_isInvalidFunction_returns_a_message_when_the_activeFunction_contains_zero_parameters_or_functions() {
    addWorkspaceToModel("WorkspaceName", 3, "0-2");
    auto const function = getFunction("name=Convolution;name=Resolution");
    m_model->setFitFunction(function);

    TS_ASSERT(m_model->isInvalidFunction());
  }

  void test_isInvalidFunction_returns_none_if_the_activeFunction_is_valid() {
    addInstrumentWorkspaceToModel("WorkspaceName", 6, 5, "0-2");
    auto const modelWorkspace = m_model->getWorkspace(0);

    (void)getSetupFitAlgorithm(m_model, modelWorkspace, "WorkspaceName");

    TS_ASSERT(!m_model->isInvalidFunction());
  }

  void test_that_getNumberOfWorkspace_returns_the_number_of_workspace_stored_by_model() {
    addWorkspaceToModel("Workspace1", 3, "0-2");
    addWorkspaceToModel("Workspace2", 3, "0-2");
    addWorkspaceToModel("Workspace3", 3, "0-2");
    TS_ASSERT_EQUALS(m_model->getNumberOfWorkspaces(), 3);
  }

  void test_that_getFitParameterNames_returns_an_empty_vector_if_the_fitOutput_is_empty() {
    addWorkspaceToModel("Workspace1", 3, "0-2");
    TS_ASSERT_EQUALS(m_model->getFitParameterNames(), std::vector<std::string>());
  }

  void test_that_getFitParameterNames_returns_a_vector_of_fit_parameters_if_the_fitOutput_contains_parameters() {
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-2");
    auto const modelWorkspace = m_model->getWorkspace(0);

    auto const alg = getExecutedFitAlgorithm(m_model, modelWorkspace, "__Convolution");
    m_model->addOutput(alg);

    TS_ASSERT(!m_model->getFitParameterNames().empty());
  }

  void test_that_clearWorkspaces_will_empty_the_fittingData() {
    addWorkspaceToModel("Workspace1", 3, "0-2");
    addWorkspaceToModel("Workspace2", 3, "0-2");
    m_model->clearWorkspaces();

    TS_ASSERT(!m_model->getWorkspace(0));
    TS_ASSERT(!m_model->getWorkspace(1));
    TS_ASSERT_EQUALS(m_model->getNumberOfWorkspaces(), 0);
  }

  void test_that_setDefaultParameterValue_will_set_the_value_of_the_provided_parameter() {
    addWorkspaceToModel("WorkspaceName", 3, "0-2");
    auto const modelWorkspace = m_model->getWorkspace(0);

    (void)getSetupFitAlgorithm(m_model, modelWorkspace, "WorkspaceName");
    m_model->setDefaultParameterValue("Amplitude", 1.5, 0);

    auto const parameters = m_model->getDefaultParameters(0);
    TS_ASSERT_EQUALS(parameters.at("f0.f1.f1.f0.Amplitude").value, 1.5);
  }

  void test_that_getParameterValues_returns_an_empty_map_if_the_dataIndex_is_out_of_range() {
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    addFitOutputDataToModel();
    TS_ASSERT(m_model->getParameterValues(1, 0).empty());
  }

  void test_that_getParameterValues_returns_the_default_parameters_if_there_are_no_fit_parameters() {
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    auto const modelWorkspace = m_model->getWorkspace(0);

    (void)getSetupFitAlgorithm(m_model, modelWorkspace, "__Convolution");
    m_model->setDefaultParameterValue("Amplitude", 1.5, 0);

    auto const parameters = m_model->getParameterValues(0, 0);
    TS_ASSERT_EQUALS(parameters.at("f0.f1.f1.f0.Amplitude").value, 1.5);
  }

  void test_that_getParameterValues_returns_the_fit_parameters_after_a_fit_has_been_executed() {
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    addFitOutputDataToModel();
    auto const parameters = m_model->getParameterValues(0, 0);
    TS_ASSERT_DELTA(parameters.at("f1.f1.f0.Amplitude").value, 1.0, 0.0001);
    TS_ASSERT_DELTA(parameters.at("f1.f1.f0.FWHM").value, 0.0175, 0.0001);
    TS_ASSERT(!parameters.empty());
  }

  void test_getFitParameters_returns_an_empty_map_when_there_is_no_fitOutput() {
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    auto const modelWorkspace = m_model->getWorkspace(0);

    (void)getSetupFitAlgorithm(m_model, modelWorkspace, "__Convolution");

    TS_ASSERT(m_model->getFitParameters(0, 0).empty());
  }

  void test_getFitParameters_returns_the_fitParameters_after_a_fit() {
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    addFitOutputDataToModel();
    auto const parameters = m_model->getFitParameters(0, 0);
    TS_ASSERT_DELTA(parameters.at("f1.f1.f0.Amplitude").value, 1.0, 0.0001);
    TS_ASSERT_DELTA(parameters.at("f1.f1.f0.FWHM").value, 0.0175, 0.0001);
    TS_ASSERT(!parameters.empty());
  }

  void test_getDefaultParameters_returns_an_empty_map_when_the_dataIndex_is_out_of_range() {
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    addFitOutputDataToModel();
    TS_ASSERT(m_model->getDefaultParameters(1).empty());
  }

  void test_getDefaultParameters_returns_the_default_parameters_which_have_been_set() {
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    addFitOutputDataToModel();

    m_model->setDefaultParameterValue("Amplitude", 1.5, 0);

    auto const parameters = m_model->getDefaultParameters(0);
    TS_ASSERT(!parameters.empty());
    TS_ASSERT_DELTA(parameters.at("f0.f1.f1.f0.Amplitude").value, 1.5, 0.0001);
  }

  void test_that_getResultLocation_returns_a_location_for_the_output_data() {
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    addFitOutputDataToModel();
    TS_ASSERT(m_model->getResultLocation(0, 0));
  }

  void test_that_cleanFailedRun_removes_the_temporary_workspace_from_the_ADS_when_a_fit_fails() {
    /// Fails the fit algorithm on purpose by providing an invalid function
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    auto const modelWorkspace = m_model->getWorkspace(0);
    SetUpADSWithWorkspace ads("Name", modelWorkspace);

    std::string const functionString = "name=Convolution;name=Resolution,Workspace=Name,WorkspaceIndex=0;";
    auto alg = setupFitAlgorithm(modelWorkspace, functionString);
    alg->execute();

    TS_ASSERT(ads.doesExist("__ConvolutionFitSequential_ws1"));
    m_model->cleanFailedRun(alg);
    TS_ASSERT(!ads.doesExist("__ConvolutionFitSequential_ws1"));
  }

  void
  test_that_cleanFailedRun_removes_the_temporary_workspace_from_the_ADS_when_a_fit_fails_for_a_single_workspaceIndex() {
    /// Fails the fit algorithm on purpose by providing an invalid function
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    auto const modelWorkspace = m_model->getWorkspace(0);
    SetUpADSWithWorkspace ads("Name", modelWorkspace);

    std::string const functionString = "name=Convolution;name=Resolution,Workspace=Name,WorkspaceIndex=0;";
    auto alg = setupFitAlgorithm(modelWorkspace, functionString);
    alg->execute();

    TS_ASSERT(ads.doesExist("__ConvolutionFitSequential_ws1"));
    m_model->cleanFailedRun(alg);
    TS_ASSERT(!ads.doesExist("__ConvolutionFitSequential_ws1"));
  }

  void test_that_getDefaultParameters_returns_full_list_of_names_for_multi_domain_functions() {
    addWorkspaceToModel("Workspace1", 3, "0-2");
    auto const function = getFunction("composite=MultiDomainFunction,NumDeriv=true;(composite=Convolution,"
                                      "NumDeriv=true,FixResolution=true,$domains=i;name=Resolution,"
                                      "WorkspaceIndex=0,X=(),Y=();(name=Lorentzian,Amplitude=1,PeakCentre=0,"
                                      "FWHM=1,constraints=(0<Amplitude,0<FWHM);name=Lorentzian,Amplitude=1,"
                                      "PeakCentre=0,FWHM=1,constraints=(0<Amplitude,0<FWHM)));");
    m_model->setFitFunction(function);
    m_model->setDefaultParameterValue("Amplitude", 1.5, 0);

    auto paramMap = m_model->getDefaultParameters(0);
    TS_ASSERT(paramMap.find("f0.f0.f1.f0.Amplitude") != paramMap.end());
    TS_ASSERT(paramMap.find("f0.f0.f1.f1.Amplitude") != paramMap.end());
    TS_ASSERT(paramMap.at("f0.f0.f1.f0.Amplitude").value == 1.5);
    TS_ASSERT(paramMap.at("f0.f0.f1.f1.Amplitude").value == 1.5);
  }

  void test_set_fitting_mode_fucntions() {
    addWorkspaceToModel("Workspace1", 3, "0-2");
    auto sequential = FittingMode::SEQUENTIAL;
    auto simultaneous = FittingMode::SIMULTANEOUS;
    m_model->setFittingMode(sequential);
    TS_ASSERT_EQUALS(m_model->getFittingMode(), sequential);
    m_model->setFittingMode(simultaneous);
    TS_ASSERT_EQUALS(m_model->getFittingMode(), simultaneous);
  }

  void test_updateFitTypeString_does_not_throw() {
    addWorkspaceToModel("Workspace1", 3, "0-2");
    TS_ASSERT_THROWS_NOTHING(m_model->updateFitTypeString());
  }

  void test_getResultLocation_returns_none_when_out_of_index() {
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    addFitOutputDataToModel();
    TS_ASSERT(!m_model->getResultLocation(WorkspaceID{1}, WorkspaceIndex{0}));
  }

  void test_getResultWorkspace_does_not_throw() {
    addInstrumentWorkspaceToModel("__Convolution", 6, 5, "0-5");
    addFitOutputDataToModel();
    TS_ASSERT_THROWS_NOTHING(m_model->getResultWorkspace());
  }

  void test_getFittingAlgorithm_does_not_throw() {
    addWorkspaceToModel("wsName", 3, "0");
    std::string const function = "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
                                 "(composite=Convolution,FixResolution=true,NumDeriv=true;"
                                 "name=Resolution,Workspace=wsName,WorkspaceIndex=0;((composite="
                                 "ProductFunction,NumDeriv="
                                 "false;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0."
                                 "0175)))";
    setFittingFunction(m_model, function);
    TS_ASSERT_THROWS_NOTHING(m_model->getFittingAlgorithm(FittingMode::SEQUENTIAL));
  }

  void test_getSingleFit_does_not_throw() {
    addWorkspaceToModel("wsName", 3, "0");
    std::string const function = "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
                                 "(composite=Convolution,FixResolution=true,NumDeriv=true;"
                                 "name=Resolution,Workspace=wsName,WorkspaceIndex=0;((composite="
                                 "ProductFunction,NumDeriv="
                                 "false;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0."
                                 "0175)))";
    setFittingFunction(m_model, function);
    TS_ASSERT_THROWS_NOTHING(m_model->getSingleFittingAlgorithm());
  }

  void test_getSingleFunction_does_not_throw() {
    addWorkspaceToModel("wsName", 3, "0");
    std::string const function = "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
                                 "(composite=Convolution,FixResolution=true,NumDeriv=true;"
                                 "name=Resolution,Workspace=wsName,WorkspaceIndex=0;((composite="
                                 "ProductFunction,NumDeriv="
                                 "false;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0."
                                 "0175)))";
    setFittingFunction(m_model, function);
    TS_ASSERT_THROWS_NOTHING(m_model->getSingleFunction(WorkspaceID{0}, WorkspaceIndex{0}));
  }

  void test_getOutputBasename_returns_correct_sequential_name() {
    addWorkspaceToModel("wsName", 3, "0-2");
    std::string outputString = "wsName_FitType_seq_FitString_0-2";
    TS_ASSERT_EQUALS(m_model->getOutputBasename(), outputString);
  }

  void test_that_single_function_correctly_identified() {
    auto const function = getFunction("composite=MultiDomainFunction,NumDeriv=true;(composite=Convolution,"
                                      "NumDeriv=true,FixResolution=true,$domains=i;name=Resolution,"
                                      "WorkspaceIndex=0,X=(),Y=();(name=ExpDecay,Height=1,Lifetime=1;));");
    m_model->setFitFunction(function);
    m_model->updateFitTypeString();
    TS_ASSERT_EQUALS("1E", m_model->getFitString());
  }

  void test_that_single_layer_composite_function_handled_correctly() {
    auto const function = getFunction("composite=MultiDomainFunction,NumDeriv=true;(composite=Convolution,"
                                      "NumDeriv=true,FixResolution=true,$domains=i;name=Resolution,"
                                      "WorkspaceIndex=0,X=(),Y=();(name=ExpDecay,Height=1,Lifetime=1;name=StretchExp,"
                                      "Height=1,Lifetime=1,Stretching=1;));");
    m_model->setFitFunction(function);
    m_model->updateFitTypeString();
    auto const fitString = m_model->getFitString();
    TS_ASSERT(fitString.find("1E") != std::string::npos);
    TS_ASSERT(fitString.find("1S") != std::string::npos);
  }

  void test_that_no_matched_name_is_correct() {
    auto const function = getFunction("composite=MultiDomainFunction,NumDeriv=true;(composite=Convolution,"
                                      "NumDeriv=true,FixResolution=true,$domains=i;name=Resolution,"
                                      "WorkspaceIndex=0,X=(),Y=();(name=ExpDecayMuon,A=1,Lambda=1;));");
    m_model->setFitFunction(function);
    m_model->updateFitTypeString();
    TS_ASSERT_EQUALS("", m_model->getFitString());
  }

  void test_that_multi_layer_composite_function_handled_correctly() {
    auto const function = getFunction(
        "composite=MultiDomainFunction,NumDeriv=true;(composite=Convolution,"
        "NumDeriv=true,FixResolution=true,$domains=i;name=Resolution,"
        "WorkspaceIndex=0,X=(),Y=();(name=ExpDecay,Height=1,Lifetime=1;name=ExpDecay,Height=1,Lifetime=1;));");
    m_model->setFitFunction(function);
    m_model->updateFitTypeString();
    TS_ASSERT_EQUALS("2E", m_model->getFitString());
  }

private:
  void addWorkspaceToModel(std::string workspaceName, int numberOfSpectra, std::string const &spectra) {
    SetUpADSWithWorkspace ads(workspaceName, createWorkspace(numberOfSpectra));
    m_model->getFitDataModel()->addWorkspace(workspaceName, FunctionModelSpectra(spectra));
    m_model->addDefaultParameters();
  }

  void addInstrumentWorkspaceToModel(std::string workspaceName, int xLength, int yLength, std::string const &spectra) {
    SetUpADSWithWorkspace ads(workspaceName, createWorkspaceWithInstrument(xLength, yLength));
    m_model->getFitDataModel()->addWorkspace(workspaceName, FunctionModelSpectra(spectra));
    m_model->addDefaultParameters();
  }

  void addFitOutputDataToModel() {
    auto const modelWorkspace = m_model->getWorkspace(0);
    auto const alg = getExecutedFitAlgorithm(m_model, modelWorkspace, modelWorkspace->getName());
    m_model->addOutput(alg);
  }

  std::unique_ptr<FittingModel> m_model;
};
