// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_JUMPFITMODELTEST_H_
#define MANTIDQT_JUMPFITMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "JumpFitModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;

namespace {

std::vector<std::string> getParameterLabels() {
  return {"f0.EISF", "f1.Width", "f1.FWHM", "f1.EISF"};
}

std::vector<std::string> getNoWidthLabels() { return {"f0.EISF", "f1.EISF"}; }

std::vector<std::string> getNoEISFLabels() { return {"f1.Width", "f1.FWHM"}; }

} // namespace

class JumpFitModelTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  JumpFitModelTest() { FrameworkManager::Instance(); }

  static JumpFitModelTest *createSuite() { return new JumpFitModelTest(); }

  static void destroySuite(JumpFitModelTest *suite) { delete suite; }

  void setUp() override {
    m_workspace = createWorkspaceWithTextAxis(4, getParameterLabels());
    m_ads = std::make_unique<SetUpADSWithWorkspace>("Name", m_workspace);
    m_model = std::make_unique<JumpFitModel>();
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    m_ads.reset();
    m_workspace.reset();
    m_model.reset();
  }

  void test_that_the_model_is_instantiated_and_can_hold_a_workspace() {
    Spectra const spectra = Spectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);

    TS_ASSERT_EQUALS(m_model->numberOfWorkspaces(), 1);
  }

  void
  test_that_removeWorkspace_will_remove_the_specified_workspace_from_the_model() {
    Spectra const spectra = Spectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);
    m_model->removeWorkspace(0);

    TS_ASSERT_EQUALS(m_model->numberOfWorkspaces(), 0);
  }

  void
  test_that_setFitType_will_change_the_fit_type_in_the_sequentialFitOutputName() {
    Spectra const spectra = Spectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);
    m_model->setFitType("ChudleyElliot");

    TS_ASSERT_EQUALS(m_model->sequentialFitOutputName(),
                     "Name_HWHM_FofQFit_ChudleyElliots");
  }

  void
  test_that_setFitType_will_change_the_fit_type_in_the_simultaneousFitOutputName() {
    Spectra const spectra = Spectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);
    m_model->setFitType("ChudleyElliot");

    TS_ASSERT_EQUALS(m_model->simultaneousFitOutputName(),
                     "Name_HWHM_FofQFit_ChudleyElliots");
  }

  void test_that_zeroWidths_returns_false_if_the_workspace_contains_widths() {
    Spectra const spectra = Spectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);

    TS_ASSERT(!m_model->zeroWidths(0));
  }

  void test_that_zeroWidths_returns_true_if_the_workspace_contains_no_widths() {
    Spectra const spectra = Spectra("0-1");
    auto const workspace2 = createWorkspaceWithTextAxis(2, getNoWidthLabels());
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);

    TS_ASSERT(m_model->zeroWidths(1));
  }

  void test_that_zeroEISF_returns_false_if_the_workspace_contains_EISFs() {
    Spectra const spectra = Spectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);

    TS_ASSERT(!m_model->zeroEISF(0));
  }

  void test_that_zeroEISF_returns_true_if_the_workspace_contains_no_EISFs() {
    Spectra const spectra = Spectra("0-1");
    auto const workspace2 = createWorkspaceWithTextAxis(2, getNoEISFLabels());
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);

    TS_ASSERT(m_model->zeroEISF(1));
  }

  void
  test_that_isMultiFit_returns_false_if_the_model_contains_one_workspace() {
    Spectra const spectra = Spectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);

    TS_ASSERT(!m_model->isMultiFit());
  }

  void
  test_that_isMultiFit_returns_true_if_the_model_contains_multiple_workspace() {
    Spectra const spectra = Spectra("0-1");
    auto const workspace2 = createWorkspaceWithTextAxis(2, getNoEISFLabels());
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);

    TS_ASSERT(m_model->isMultiFit());
  }

  void
  test_that_isMultiFit_returns_false_if_the_model_contains_multiple_workspace_which_are_identical() {
    Spectra const spectra = Spectra("0-1");

    addWorkspacesToModel(spectra, m_workspace, m_workspace);

    TS_ASSERT(!m_model->isMultiFit());
  }

  void test_that_getSpectrumDependentAttributes_returns_an_empty_vector() {
    TS_ASSERT(m_model->getSpectrumDependentAttributes().empty());
  }

  void
  test_that_getFitParameterName_will_return_the_name_of_the_expected_parameter() {
    Spectra const spectra = Spectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);

    TS_ASSERT_EQUALS(m_model->getFitParameterName(0, 0), "f0.EISF");
    TS_ASSERT_EQUALS(m_model->getFitParameterName(0, 2), "f1.FWHM");
  }

  void
  test_that_getWidths_will_return_an_empty_vector_if_there_are_no_widths() {
    Spectra const spectra = Spectra("0-1");
    auto const workspace2 = createWorkspaceWithTextAxis(2, getNoWidthLabels());
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);

    TS_ASSERT(m_model->getWidths(1).empty());
  }

  void test_that_getWidths_will_return_the_width_parameter_names() {
    Spectra const spectra = Spectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);

    TS_ASSERT_EQUALS(m_model->getWidths(0)[0], "f1.Width");
    TS_ASSERT_EQUALS(m_model->getWidths(0)[1], "f1.FWHM");
  }

  void test_that_getEISF_will_return_an_empty_vector_if_there_are_no_EISFs() {
    Spectra const spectra = Spectra("0-1");
    auto const workspace2 = createWorkspaceWithTextAxis(2, getNoEISFLabels());
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);

    TS_ASSERT(m_model->getEISF(1).empty());
  }

  void test_that_getEISF_will_return_the_EISF_parameter_names() {
    Spectra const spectra = Spectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);

    TS_ASSERT_EQUALS(m_model->getEISF(0)[0], "f0.EISF");
    TS_ASSERT_EQUALS(m_model->getEISF(0)[1], "f1.EISF");
  }

  void test_that_getWidthSpectrum_will_return_none_when_there_are_no_widths() {
    Spectra const spectra = Spectra("0-1");
    auto const workspace2 = createWorkspaceWithTextAxis(2, getNoWidthLabels());
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);

    TS_ASSERT(!m_model->getWidthSpectrum(0, 1));
  }

  void test_that_getWidthSpectrum_will_return_the_width_spectrum_number() {
    Spectra const spectra = Spectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);

    TS_ASSERT_EQUALS(m_model->getWidthSpectrum(0, 0).get(), 1);
    TS_ASSERT_EQUALS(m_model->getWidthSpectrum(1, 0).get(), 2);
  }

  void test_that_getEISFSpectrum_will_return_none_when_there_are_no_EISFs() {
    Spectra const spectra = Spectra("0-1");
    auto const workspace2 = createWorkspaceWithTextAxis(2, getNoEISFLabels());
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);

    TS_ASSERT(!m_model->getEISFSpectrum(0, 1));
  }

  void test_that_getEISFSpectrum_will_return_the_EISF_spectrum_number() {
    Spectra const spectra = Spectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);

    TS_ASSERT_EQUALS(m_model->getEISFSpectrum(0, 0).get(), 0);
    TS_ASSERT_EQUALS(m_model->getEISFSpectrum(1, 0).get(), 3);
  }

  void
  test_that_sequentialFitOutputName_returns_the_correct_name_for_a_multi_fit() {
    Spectra const spectra = Spectra("0-1");
    auto const workspace2 = createWorkspaceWithTextAxis(2, getNoEISFLabels());
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);
    m_model->setFitType("ChudleyElliot");

    TS_ASSERT_EQUALS(m_model->sequentialFitOutputName(),
                     "MultiFofQFit_ChudleyElliot_Results");
  }

  void
  test_that_simultaneousFitOutputName_returns_the_correct_name_for_a_multi_fit() {
    Spectra const spectra = Spectra("0-1");
    auto const workspace2 = createWorkspaceWithTextAxis(2, getNoEISFLabels());
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(spectra, m_workspace, workspace2);
    m_model->setFitType("ChudleyElliot");

    TS_ASSERT_EQUALS(m_model->simultaneousFitOutputName(),
                     "MultiFofQFit_ChudleyElliot_Results");
  }

  void
  test_that_singleFitOutputName_returns_the_correct_name_for_a_single_data_set_fit() {
    Spectra const spectra = Spectra("0-1");

    addWorkspacesToModel(spectra, m_workspace);
    m_model->setFitType("ChudleyElliot");

    TS_ASSERT_EQUALS(m_model->singleFitOutputName(0, 0),
                     "Name_HWHM_FofQFit_ChudleyElliot_s0_Results");
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
  std::unique_ptr<JumpFitModel> m_model;
};

#endif /* MANTIDQT_JUMPFITMODELTEST_H_ */
