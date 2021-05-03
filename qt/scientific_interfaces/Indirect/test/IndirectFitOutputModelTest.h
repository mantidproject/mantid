// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <utility>

#include "IndirectFitOutputModel.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidCurveFitting/Algorithms/ConvolutionFit.h"
#include "MantidCurveFitting/Algorithms/QENSFitSequential.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;

using ConvolutionFitSequential = Algorithms::ConvolutionFit<Algorithms::QENSFitSequential>;

namespace {
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
  alg->setProperty("OutputWorkspace", "output");
  alg->setLogging(false);
  return alg;
}

IAlgorithm_sptr getSetupFitAlgorithm(const MatrixWorkspace_sptr &workspace, std::string const &workspaceName) {
  std::string const function = "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
                               "(composite=Convolution,FixResolution=true,NumDeriv=true;"
                               "name=Resolution,Workspace=" +
                               workspaceName +
                               ",WorkspaceIndex=0;((composite=ProductFunction,NumDeriv="
                               "false;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0."
                               "0175)))";
  auto alg = setupFitAlgorithm(std::move(workspace), function);
  return alg;
}

IAlgorithm_sptr getExecutedFitAlgorithm(MatrixWorkspace_sptr workspace, std::string const &workspaceName) {
  auto const alg = getSetupFitAlgorithm(std::move(workspace), workspaceName);
  alg->execute();
  return alg;
}

template <typename WorkspaceType>
std::shared_ptr<WorkspaceType> getWorkspaceOutput(const IAlgorithm_sptr &algorithm, const std::string &propertyName) {
  return AnalysisDataService::Instance().retrieveWS<WorkspaceType>(algorithm->getProperty(propertyName));
}

} // namespace

class IndirectFitOutputModelTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  IndirectFitOutputModelTest() { FrameworkManager::Instance(); }

  static IndirectFitOutputModelTest *createSuite() { return new IndirectFitOutputModelTest(); }

  static void destroySuite(IndirectFitOutputModelTest *suite) { delete suite; }

  void setUp() override { m_model = std::make_unique<IndirectFitOutputModel>(); }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    m_model.reset();
  }

  void test_isEmpty_returns_true_if_no_output_is_set() { TS_ASSERT(m_model->isEmpty()); }

  void test_isEmpty_returns_False_if_no_output_has_been_set() {
    auto workspace = createWorkspaceWithInstrument(6, 5);
    SetUpADSWithWorkspace ads("wsName", workspace);
    auto const fitAlgorithm = getExecutedFitAlgorithm(workspace, "wsName");
    auto group = getWorkspaceOutput<WorkspaceGroup>(fitAlgorithm, "OutputWorkspaceGroup");
    auto parameters = getWorkspaceOutput<ITableWorkspace>(fitAlgorithm, "OutputParameterWorkspace");
    auto result = getWorkspaceOutput<WorkspaceGroup>(fitAlgorithm, "OutputWorkspace");
    m_model->addOutput(group, parameters, result);

    TS_ASSERT(!m_model->isEmpty());
  }

  void test_isSpectrumFit_returns_true_if_output_has_been_set() {
    auto workspace = createWorkspaceWithInstrument(6, 5);
    SetUpADSWithWorkspace ads("wsName", workspace);

    auto const fitAlgorithm = getExecutedFitAlgorithm(workspace, "wsName");

    auto group = getWorkspaceOutput<WorkspaceGroup>(fitAlgorithm, "OutputWorkspaceGroup");
    auto parameters = getWorkspaceOutput<ITableWorkspace>(fitAlgorithm, "OutputParameterWorkspace");
    auto result = getWorkspaceOutput<WorkspaceGroup>(fitAlgorithm, "OutputWorkspace");
    m_model->addOutput(group, parameters, result);
    TS_ASSERT(m_model->isSpectrumFit(FitDomainIndex{0}));
  }

  void test_isSpectrumFit_returns_False_if_no_output_has_been_set() {
    TS_ASSERT(!m_model->isSpectrumFit(FitDomainIndex{0}));
  }

  void test_isSpectrumFit_returns_false_if_index_is_out_of_range() {
    auto workspace = createWorkspaceWithInstrument(6, 5);
    SetUpADSWithWorkspace ads("wsName", workspace);

    auto const fitAlgorithm = getExecutedFitAlgorithm(workspace, "wsName");

    auto group = getWorkspaceOutput<WorkspaceGroup>(fitAlgorithm, "OutputWorkspaceGroup");
    auto parameters = getWorkspaceOutput<ITableWorkspace>(fitAlgorithm, "OutputParameterWorkspace");
    auto result = getWorkspaceOutput<WorkspaceGroup>(fitAlgorithm, "OutputWorkspace");
    m_model->addOutput(group, parameters, result);
    TS_ASSERT(!m_model->isSpectrumFit(FitDomainIndex{6}));
  }

  void test_getParameters_returns_correct_value() {
    auto workspace = createWorkspaceWithInstrument(6, 5);
    SetUpADSWithWorkspace ads("wsName", workspace);

    auto const fitAlgorithm = getExecutedFitAlgorithm(workspace, "wsName");

    auto group = getWorkspaceOutput<WorkspaceGroup>(fitAlgorithm, "OutputWorkspaceGroup");
    auto parameters = getWorkspaceOutput<ITableWorkspace>(fitAlgorithm, "OutputParameterWorkspace");
    auto result = getWorkspaceOutput<WorkspaceGroup>(fitAlgorithm, "OutputWorkspace");
    m_model->addOutput(group, parameters, result);
    auto params = m_model->getParameters(FitDomainIndex{0});
    TS_ASSERT_EQUALS(params["f0.A0"].value, 0.00);
  }

  void test_getParameters_throws_if_no_fitted_data_correct_value() {
    TS_ASSERT_THROWS(m_model->getParameters(FitDomainIndex{0}), std::invalid_argument);
    TS_ASSERT_THROWS(m_model->getParameters(FitDomainIndex{6}), std::invalid_argument);
  }

  void test_getResultLocaton() {
    auto workspace = createWorkspaceWithInstrument(6, 5);
    SetUpADSWithWorkspace ads("wsName", workspace);

    auto const fitAlgorithm = getExecutedFitAlgorithm(workspace, "wsName");

    auto group = getWorkspaceOutput<WorkspaceGroup>(fitAlgorithm, "OutputWorkspaceGroup");
    auto parameters = getWorkspaceOutput<ITableWorkspace>(fitAlgorithm, "OutputParameterWorkspace");
    auto result = getWorkspaceOutput<WorkspaceGroup>(fitAlgorithm, "OutputWorkspace");
    m_model->addOutput(group, parameters, result);
    auto const index = FitDomainIndex{0};
    auto resultLocation = ResultLocationNew(result, WorkspaceGroupIndex{static_cast<size_t>(index.value)});
    TS_ASSERT_EQUALS(m_model->getResultLocation(index)->index, resultLocation.index);
  }

private:
  std::unique_ptr<IndirectFitOutputModel> m_model;
};