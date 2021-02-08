// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "ConvFitModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;
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
  auto fun = FunctionFactory::Instance().createInitialized(
      "composite=MultiDomainFunction;" + functionString + ";" + functionString);
  return std::dynamic_pointer_cast<MultiDomainFunction>(fun);
}

} // namespace

class ConvFitModelTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  ConvFitModelTest() { FrameworkManager::Instance(); }

  static ConvFitModelTest *createSuite() { return new ConvFitModelTest(); }

  static void destroySuite(ConvFitModelTest *suite) { delete suite; }

  void setUp() override {
    m_workspace = createWorkspaceWithInstrument(6, 5);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("Name", m_workspace);
    m_model = std::make_unique<ConvFitModel>();
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    m_ads.reset();
    m_workspace.reset();
    m_model.reset();
  }

  void test_that_addWorkspace_will_add_multiple_workspaces() {
    FunctionModelSpectra const spectra = FunctionModelSpectra("0-1");
    auto const workspace2 = createWorkspace(3, 3);
    auto const workspace3 = createWorkspace(3, 2);
    auto const workspace4 = createWorkspace(3, 6);
    auto const workspace5 = createWorkspace(3, 7);

    addWorkspacesToModel(spectra, m_workspace, workspace2, workspace3,
                         workspace4, workspace5);

    TS_ASSERT_EQUALS(m_model->numberOfWorkspaces(), TableDatasetIndex{5});
  }

  void
  test_that_getFittingFunction_will_return_the_fitting_function_which_has_been_set() {
    FunctionModelSpectra const spectra = FunctionModelSpectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);
    m_model->setFitFunction(getFunction(getFunctionString("Name")));

    auto const fittingFunction = m_model->getFittingFunction();
    TS_ASSERT(fittingFunction);
    TS_ASSERT_EQUALS(fittingFunction->getAttributeNames()[0], "NumDeriv");
  }

  void
  test_that_getInstrumentResolution_will_return_none_if_the_index_provided_is_larger_than_the_number_of_workspaces() {
    FunctionModelSpectra const spectra = FunctionModelSpectra("0-1");
    auto const workspace2 = createWorkspace(3, 3);
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);

    TS_ASSERT(!m_model->getInstrumentResolution(TableDatasetIndex{3}));
  }

  void
  test_that_getInstrumentResolution_will_return_the_none_if_the_workspace_has_no_analyser() {
    /// A unit test for a positive response from getInstrumentResolution needs
    /// to be added. The workspace used in the test will need to have an
    /// analyser attached to its instrument
    FunctionModelSpectra const spectra = FunctionModelSpectra("0-1");
    auto const workspace2 = createWorkspace(3, 3);
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);

    TS_ASSERT(!m_model->getInstrumentResolution(TableDatasetIndex{0}));
  }

  void
  test_that_getNumberHistograms_will_get_the_number_of_spectra_for_the_workspace_specified() {
    FunctionModelSpectra const spectra = FunctionModelSpectra("0-1");
    auto const workspace2 = createWorkspace(5, 3);
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);

    TS_ASSERT_EQUALS(m_model->getNumberHistograms(TableDatasetIndex{1}), 5);
  }

  void
  test_that_removeWorkspace_will_remove_the_workspace_specified_from_the_model() {
    FunctionModelSpectra const spectra = FunctionModelSpectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);
    m_model->removeWorkspace(TableDatasetIndex{0});

    TS_ASSERT_EQUALS(m_model->numberOfWorkspaces(), TableDatasetIndex{0});
  }

  void
  test_that_setResolution_will_throw_when_provided_the_name_of_a_workspace_which_does_not_exist() {
    TS_ASSERT_THROWS(
        m_model->setResolution("InvalidName", TableDatasetIndex{0}),
        const std::runtime_error &);
  }

  void
  test_that_setResolution_will_throw_when_provided_an_index_that_is_out_of_range() {
    TS_ASSERT_THROWS(
        m_model->setResolution(m_workspace->getName(), TableDatasetIndex{5}),
        const std::out_of_range &);
  }

  void
  test_that_get_resolution_for_fit_returns_correctly_for_multiple_workspaces() {
    FunctionModelSpectra const spectra = FunctionModelSpectra("0,5");
    addWorkspacesToModel(spectra, m_workspace);
    auto const workspace2 = createWorkspace(3, 3);
    m_ads->addOrReplace("Workspace2", workspace2);
    FunctionModelSpectra const spectra2 = FunctionModelSpectra("1-2");
    addWorkspacesToModel(spectra2, workspace2);
    m_model->setResolution(m_workspace->getName(), TableDatasetIndex{0});
    m_model->setResolution(workspace2->getName(), TableDatasetIndex{1});

    auto fitResolutions = m_model->getResolutionsForFit();

    TS_ASSERT_EQUALS(fitResolutions.size(), 4);
    TS_ASSERT_EQUALS(fitResolutions[0].first, "Name");
    TS_ASSERT_EQUALS(fitResolutions[0].second, 0);
    TS_ASSERT_EQUALS(fitResolutions[1].first, "Name");
    TS_ASSERT_EQUALS(fitResolutions[1].second, 5);
    TS_ASSERT_EQUALS(fitResolutions[2].first, "Workspace2");
    TS_ASSERT_EQUALS(fitResolutions[2].second, 1);
    TS_ASSERT_EQUALS(fitResolutions[3].first, "Workspace2");
    TS_ASSERT_EQUALS(fitResolutions[3].second, 2);
  }

private:
  template <typename Workspace, typename... Workspaces>
  void addWorkspacesToModel(FunctionModelSpectra const &spectra,
                            Workspace const &workspace,
                            Workspaces const &... workspaces) {
    m_model->addWorkspace(workspace, spectra);
    addWorkspacesToModel(spectra, workspaces...);
  }

  void addWorkspacesToModel(FunctionModelSpectra const &spectra,
                            MatrixWorkspace_sptr const &workspace) {
    m_model->addWorkspace(workspace, spectra);
  }

  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  std::unique_ptr<ConvFitModel> m_model;
};
