// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "QENSFitting/ConvolutionModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace MantidQt::MantidWidgets;

namespace {

std::string getFunctionString(std::string const &workspaceName) {
  return "composite=CompositeFunction,$domains=i;name=LinearBackground,A0=0,A1="
         "0,ties=(A0=0.000000,A1=0.0);"
         "(composite=Convolution,FixResolution=true,NumDeriv=true;"
         "name=Resolution,Workspace=" +
         workspaceName +
         ",WorkspaceIndex=0;((composite=ProductFunction,NumDeriv="
         "false;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0."
         "0175)))";
}

MultiDomainFunction_sptr getFunction(std::string const &functionString) {
  auto fun = FunctionFactory::Instance().createInitialized("composite=MultiDomainFunction;" + functionString + ";" +
                                                           functionString);
  return std::dynamic_pointer_cast<MultiDomainFunction>(fun);
}

void setFittingFunction(std::unique_ptr<ConvolutionModel> &model, std::string const &functionString) {
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

IAlgorithm_sptr getSetupFitAlgorithm(std::unique_ptr<ConvolutionModel> &model, const MatrixWorkspace_sptr &workspace,
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

IAlgorithm_sptr getExecutedFitAlgorithm(std::unique_ptr<ConvolutionModel> &model, MatrixWorkspace_sptr workspace,
                                        std::string const &workspaceName) {
  auto const alg = getSetupFitAlgorithm(model, std::move(workspace), workspaceName);
  alg->execute();
  return alg;
}

} // namespace

class ConvolutionModelTest : public CxxTest::TestSuite {
public:
  static ConvolutionModelTest *createSuite() { return new ConvolutionModelTest(); }

  static void destroySuite(ConvolutionModelTest *suite) { delete suite; }

  void setUp() override {
    m_workspace = createWorkspaceWithInstrument(6, 5);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("Name", m_workspace);
    m_model = std::make_unique<ConvolutionModel>();
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    m_ads.reset();
    m_workspace.reset();
    m_model.reset();
  }

  void test_that_getFitFunction_will_return_the_fitting_function_which_has_been_set() {
    FunctionModelSpectra const spectra = FunctionModelSpectra("0-1");

    m_model->getFitDataModel()->addWorkspace(m_workspace, spectra);
    m_model->setFitFunction(getFunction(getFunctionString("Name")));

    auto const fittingFunction = m_model->getFitFunction();
    TS_ASSERT(fittingFunction);
    TS_ASSERT_EQUALS(fittingFunction->getAttributeNames()[0], "NumDeriv");
  }

  void
  test_that_getInstrumentResolution_will_return_none_if_the_index_provided_is_larger_than_the_number_of_workspaces() {
    FunctionModelSpectra const spectra = FunctionModelSpectra("0-1");
    auto const workspace2 = createWorkspace(3, 3);
    m_ads->addOrReplace("Name2", workspace2);

    m_model->getFitDataModel()->addWorkspace(m_workspace, spectra);
    m_model->getFitDataModel()->addWorkspace(workspace2, spectra);

    TS_ASSERT(!m_model->getInstrumentResolution(WorkspaceID{3}));
  }

  void test_that_getInstrumentResolution_will_return_the_none_if_the_workspace_has_no_analyser() {
    /// A unit test for a positive response from getInstrumentResolution needs
    /// to be added. The workspace used in the test will need to have an
    /// analyser attached to its instrument
    FunctionModelSpectra const spectra = FunctionModelSpectra("0-1");
    auto const workspace2 = createWorkspace(3, 3);
    m_ads->addOrReplace("Name2", workspace2);

    m_model->getFitDataModel()->addWorkspace(m_workspace, spectra);
    m_model->getFitDataModel()->addWorkspace(workspace2, spectra);

    TS_ASSERT(!m_model->getInstrumentResolution(WorkspaceID{0}));
  }

  void test_addOutput_does_not_throw_with_executed_fit() {
    FunctionModelSpectra const spectra = FunctionModelSpectra("0-1");
    m_model->getFitDataModel()->addWorkspace(m_workspace, spectra);
    auto const modelWorkspace = m_model->getWorkspace(0);

    auto const alg = getExecutedFitAlgorithm(m_model, modelWorkspace, "Name");
    TS_ASSERT_THROWS_NOTHING(m_model->addOutput(alg));
  }

  void test_addOutput_does_not_throw_with_unexecuted_fit() {
    FunctionModelSpectra const spectra = FunctionModelSpectra("0-1");
    m_model->getFitDataModel()->addWorkspace(m_workspace, spectra);
    auto const modelWorkspace = m_model->getWorkspace(0);

    auto const alg = getSetupFitAlgorithm(m_model, std::move(modelWorkspace), "Name");
    TS_ASSERT_THROWS_NOTHING(m_model->addOutput(alg));
  }

private:
  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  std::unique_ptr<ConvolutionModel> m_model;
};
