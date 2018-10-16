#ifndef MANTID_INDIRECTFITPLOTMODELTEST_H_
#define MANTID_INDIRECTFITPLOTMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectFitPlotModel.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Algorithms/ConvolutionFit.h"
#include "MantidCurveFitting/Algorithms/QENSFitSequential.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace Mantid::IndirectFitDataCreationHelper;

using ConvolutionFitSequential =
    Algorithms::ConvolutionFit<Algorithms::QENSFitSequential>;

namespace {

IFunction_sptr getFunction(std::string const &functionString) {
  return FunctionFactory::Instance().createInitialized(functionString);
}

/// A dummy class used to create a model to pass to IndirectFitPlotModel's
/// constructor
class DummyModel
    : public MantidQt::CustomInterfaces::IDA::IndirectFittingModel {
public:
  ~DummyModel(){};

private:
  std::string sequentialFitOutputName() const override { return ""; };
  std::string simultaneousFitOutputName() const override { return ""; };
  std::string singleFitOutputName(std::size_t index,
                                  std::size_t spectrum) const override {
    (void)index;
    (void)spectrum;
    return "";
  };
};

void setFittingFunction(IndirectFittingModel *model,
                        std::string const &functionString) {
  model->setFitFunction(getFunction(functionString));
}

IndirectFittingModel *getEmptyDummyModel() { return new DummyModel(); }

IndirectFittingModel *
createModelWithSingleWorkspace(std::string const &workspaceName,
                               int const &numberOfSpectra) {
  auto model = getEmptyDummyModel();
  SetUpADSWithNoDestructor ads(workspaceName, createWorkspace(numberOfSpectra));
  model->addWorkspace(workspaceName);
  std::string const function =
      "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
      "(composite=Convolution,FixResolution=true,NumDeriv=true;"
      "name=Resolution,Workspace=" +
      workspaceName +
      ",WorkspaceIndex=0;((composite=ProductFunction,NumDeriv="
      "false;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0."
      "0175)))";
  setFittingFunction(model, function);
  return model;
}

void addWorkspacesToModel(IndirectFittingModel *model,
                          int const &numberOfSpectra) {
  (void)model;
  (void)numberOfSpectra;
}

template <typename Name, typename... Names>
void addWorkspacesToModel(IndirectFittingModel *model,
                          int const &numberOfSpectra, Name const &workspaceName,
                          Names const &... workspaceNames) {
  SetUpADSWithNoDestructor ads(workspaceName, createWorkspace(numberOfSpectra));
  model->addWorkspace(workspaceName);
  addWorkspacesToModel(model, numberOfSpectra, workspaceNames...);
}

template <typename Name, typename... Names>
IndirectFittingModel *
createModelWithMultipleWorkspaces(int const &numberOfSpectra,
                                  Name const &workspaceName,
                                  Names const &... workspaceNames) {
  auto model = createModelWithSingleWorkspace(workspaceName, numberOfSpectra);
  addWorkspacesToModel(model, numberOfSpectra, workspaceNames...);
  return model;
}

IndirectFittingModel *createModelWithSingleInstrumentWorkspace(
    std::string const &workspaceName, int const &xLength, int const &yLength) {
  auto model = getEmptyDummyModel();
  SetUpADSWithNoDestructor ads(workspaceName,
                               createWorkspaceWithInstrument(xLength, yLength));
  model->addWorkspace(workspaceName);
  return model;
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

IAlgorithm_sptr getSetupFitAlgorithm(IndirectFittingModel *model,
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

IAlgorithm_sptr getExecutedFitAlgorithm(IndirectFittingModel *model,
                                        MatrixWorkspace_sptr workspace,
                                        std::string const &workspaceName) {
  auto const alg = getSetupFitAlgorithm(model, workspace, workspaceName);
  alg->execute();
  return alg;
}

IndirectFittingModel *getModelWithFitOutputData() {
  auto model = createModelWithSingleInstrumentWorkspace("__ConvFit", 6, 5);
  auto const modelWorkspace = model->getWorkspace(0);
  SetUpADSWithNoDestructor ads("__ConvFit", modelWorkspace);

  auto const alg = getExecutedFitAlgorithm(model, modelWorkspace, "__ConvFit");
  model->addOutput(alg);
  return model;
}

IndirectFitPlotModel getFitPlotModel() {
  return IndirectFitPlotModel(
      createModelWithMultipleWorkspaces(10, "Workspace1", "Workspace2"));
}

IndirectFitPlotModel getFitPlotModelWithFitData() {
  return IndirectFitPlotModel(getModelWithFitOutputData());
}

} // namespace

class IndirectFitPlotModelTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  IndirectFitPlotModelTest() { FrameworkManager::Instance(); }

  static IndirectFitPlotModelTest *createSuite() {
    return new IndirectFitPlotModelTest();
  }

  static void destroySuite(IndirectFitPlotModelTest *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void
  test_that_IndirectFittingModel_instantiates_a_model_with_the_correct_starting_member_variables() {
    auto const model = getFitPlotModel();

    TS_ASSERT_EQUALS(model.getActiveDataIndex(), 0);
    TS_ASSERT_EQUALS(model.getActiveSpectrum(), 0);
    TS_ASSERT_EQUALS(model.numberOfWorkspaces(), 2);
  }

  void
  test_that_getWorkspace_returns_a_workspace_with_the_correct_number_of_spectra() {
    auto const model = getFitPlotModel();
    TS_ASSERT_EQUALS(model.getWorkspace()->getNumberHistograms(), 10);
  }

  void
  test_that_getGuessWorkspace_will_create_and_then_return_a_guess_workspace_with_the_correct_number_of_spectra() {
    /// Only creates a guess for the active spectra of the selected workspace
    auto const model = getFitPlotModel();

    TS_ASSERT(model.getGuessWorkspace());
    TS_ASSERT_EQUALS(model.getGuessWorkspace()->getNumberHistograms(), 1);
  }

  void
  test_that_getResultWorkspace_returns_a_nullptr_if_a_fit_has_not_yet_been_run() {
    auto const model = getFitPlotModel();
    TS_ASSERT(!model.getResultWorkspace());
  }

  void
  test_that_getResultWorkspace_returns_a_workspace_when_data_has_been_fit() {
    auto const model = getFitPlotModelWithFitData();
    TS_ASSERT(model.getResultWorkspace());
  }
};

#endif
