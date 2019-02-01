// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CONVFITMODELTEST_H_
#define MANTIDQT_CONVFITMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "ConvFitModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;

namespace {

std::string getFunctionString(std::string const &workspaceName) {
  return "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
         "(composite=Convolution,FixResolution=true,NumDeriv=true;"
         "name=Resolution,Workspace=" +
         workspaceName +
         ",WorkspaceIndex=0;((composite=ProductFunction,NumDeriv="
         "false;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0."
         "0175)))";
}

IFunction_sptr getFunction(std::string const &functionString) {
  return FunctionFactory::Instance().createInitialized(functionString);
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

  void test_that_the_model_is_instantiated_and_can_hold_a_workspace() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");

    m_model->addWorkspace(m_workspace, spectra);

    TS_ASSERT_EQUALS(m_model->numberOfWorkspaces(), 1);
  }

  void test_that_addWorkspace_will_add_multiple_workspaces() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");
    auto const workspace2 = createWorkspace(3, 3);
    auto const workspace3 = createWorkspace(3, 2);
    auto const workspace4 = createWorkspace(3, 6);
    auto const workspace5 = createWorkspace(3, 7);

    addWorkspacesToModel(spectra, m_workspace, workspace2, workspace3,
                         workspace4, workspace5);

    TS_ASSERT_EQUALS(m_model->numberOfWorkspaces(), 5);
  }

  void
  test_that_getFittingFunction_will_return_the_fitting_function_which_has_been_set() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");

    addWorkspacesToModel(spectra, m_workspace);
    m_model->setFitFunction(getFunction(getFunctionString("Name")));

    auto const fittingFunction = m_model->getFittingFunction();
    TS_ASSERT(fittingFunction);
    TS_ASSERT_EQUALS(fittingFunction->getAttributeNames()[0], "NumDeriv");
  }

  void
  test_that_getInstrumentResolution_will_return_none_if_the_index_provided_is_larger_than_the_number_of_workspaces() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");
    auto const workspace2 = createWorkspace(3, 3);
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);

    TS_ASSERT(!m_model->getInstrumentResolution(3));
  }

  void
  test_that_getInstrumentResolution_will_return_the_none_if_the_workspace_has_no_analyser() {
    /// A unit test for a positive response from getInstrumentResolution needs
    /// to be added. The workspace used in the test will need to have an
    /// analyser attached to its instrument
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");
    auto const workspace2 = createWorkspace(3, 3);
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);

    TS_ASSERT(!m_model->getInstrumentResolution(0));
  }

  void
  test_that_getNumberHistograms_will_get_the_number_of_spectra_for_the_workspace_specified() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");
    auto const workspace2 = createWorkspace(5, 3);
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);

    TS_ASSERT_EQUALS(m_model->getNumberHistograms(1), 5);
  }

  void
  test_that_getResolution_will_return_the_a_nullptr_when_the_resolution_has_not_been_set() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");
    auto const resolution = createWorkspace(5, 3);

    addWorkspacesToModel(spectra, m_workspace);

    TS_ASSERT(!m_model->getResolution(0));
  }

  void test_that_getResolution_will_return_the_workspace_which_it_was_set_at() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");
    auto const resolution = createWorkspace(6, 3);

    addWorkspacesToModel(spectra, m_workspace);
    m_model->setResolution(resolution, 0);

    TS_ASSERT_EQUALS(m_model->getResolution(0), resolution);
  }

  void
  test_that_getResolution_will_return_the_a_nullptr_when_the_index_provided_is_out_of_range() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");
    auto const resolution = createWorkspace(6, 3);

    addWorkspacesToModel(spectra, m_workspace);
    m_model->setResolution(resolution, 0);

    TS_ASSERT(!m_model->getResolution(2));
  }

  void test_that_getSpectrumDependentAttributes_returns_workspace_index() {
    std::vector<std::string> attributes{"WorkspaceIndex"};

    auto const spectrumDependentAttributes =
        m_model->getSpectrumDependentAttributes();
    for (auto i = 0u; i < spectrumDependentAttributes.size(); ++i)
      TS_ASSERT_EQUALS(attributes[i], spectrumDependentAttributes[i]);
  }

  void
  test_that_removeWorkspace_will_remove_the_workspace_specified_from_the_model() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");

    addWorkspacesToModel(spectra, m_workspace);
    m_model->removeWorkspace(0);

    TS_ASSERT_EQUALS(m_model->numberOfWorkspaces(), 0);
  }

  void
  test_that_setResolution_will_throw_when_provided_the_name_of_a_workspace_which_does_not_exist() {
    TS_ASSERT_THROWS(m_model->setResolution("InvalidName", 0),
                     std::runtime_error);
  }

  void
  test_that_setResolution_will_set_the_resolution_when_provided_a_correct_workspace_name() {
    m_model->setResolution("Name", 0);
    TS_ASSERT_EQUALS(m_model->getResolution(0), m_workspace);
  }

  void
  test_that_setResolution_will_throw_when_provided_an_index_that_is_out_of_range() {
    TS_ASSERT_THROWS(m_model->setResolution(m_workspace, 5), std::out_of_range);
  }

private:
  template <typename Workspace, typename... Workspaces>
  void addWorkspacesToModel(Spectra const &spectra, Workspace const &workspace,
                            Workspaces const &... workspaces) {
    m_model->addWorkspace(workspace, spectra);
    addWorkspacesToModel(spectra, workspaces...);
  }

  void addWorkspacesToModel(Spectra const &spectra,
                            MatrixWorkspace_sptr const &workspace) {
    m_model->addWorkspace(workspace, spectra);
  }

  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  std::unique_ptr<ConvFitModel> m_model;
};

#endif /* MANTIDQT_CONVFITMODELTEST_H_ */
