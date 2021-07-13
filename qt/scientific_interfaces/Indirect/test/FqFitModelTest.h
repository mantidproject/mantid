// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "FqFitModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;

namespace {

std::vector<std::string> getParameterLabels() { return {"f0.EISF", "f1.Width", "f1.FWHM", "f1.EISF"}; }

std::vector<std::string> getNoWidthLabels() { return {"f0.EISF", "f1.EISF"}; }

std::vector<std::string> getNoEISFLabels() { return {"f1.Width", "f1.FWHM"}; }

} // namespace

class FqFitModelTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  FqFitModelTest() { FrameworkManager::Instance(); }

  static FqFitModelTest *createSuite() { return new FqFitModelTest(); }

  static void destroySuite(FqFitModelTest *suite) { delete suite; }

  void setUp() override {
    m_workspace = createWorkspaceWithTextAxis(4, getParameterLabels());
    m_ads = std::make_unique<SetUpADSWithWorkspace>("Name", m_workspace);
    m_model = std::make_unique<FqFitModel>();
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    m_ads.reset();
    m_workspace.reset();
    m_model.reset();
  }

  void test_that_the_model_is_instantiated_and_can_hold_a_workspace() {
    addWorkspacesToModel(m_workspace);
    TS_ASSERT_EQUALS(m_model->getNumberOfWorkspaces(), TableDatasetIndex{1});
  }

  void test_that_removeWorkspace_will_remove_the_specified_workspace_from_the_model() {
    addWorkspacesToModel(m_workspace);
    m_model->removeWorkspace(TableDatasetIndex{0});

    TS_ASSERT_EQUALS(m_model->getNumberOfWorkspaces(), TableDatasetIndex{0});
  }

  void test_that_isMultiFit_returns_false_if_the_model_contains_one_workspace() {
    addWorkspacesToModel(m_workspace);
    TS_ASSERT(!m_model->isMultiFit());
  }

  void test_that_isMultiFit_returns_true_if_the_model_contains_multiple_workspace() {
    auto const workspace2 = createWorkspaceWithTextAxis(2, getNoEISFLabels());
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(m_workspace, workspace2);
    TS_ASSERT(m_model->isMultiFit());
  }

  void test_that_isMultiFit_returns_false_if_the_model_contains_multiple_workspace_which_are_identical() {
    addWorkspacesToModel(m_workspace, m_workspace);
    TS_ASSERT(!m_model->isMultiFit());
  }

  void test_that_getFitParameterName_will_return_the_name_of_the_expected_parameter() {
    addWorkspacesToModel(m_workspace);

    TS_ASSERT_EQUALS(
        m_model->getFitParameterName(TableDatasetIndex{0}, MantidQt::CustomInterfaces::IDA::WorkspaceIndex{0}),
        "f0.EISF");
    TS_ASSERT_EQUALS(
        m_model->getFitParameterName(TableDatasetIndex{0}, MantidQt::CustomInterfaces::IDA::WorkspaceIndex{2}),
        "f1.FWHM");
  }

  void test_that_getWidths_returns_true_if_the_workspace_contains_no_widths() {
    auto const workspace2 = createWorkspaceWithTextAxis(2, getNoWidthLabels());
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(workspace2);

    TS_ASSERT(m_model->getWidths(TableDatasetIndex{1}).empty());
  }

  void test_that_getWidths_will_return_the_width_parameter_names() {
    addWorkspacesToModel(m_workspace);

    TS_ASSERT_EQUALS(m_model->getWidths(TableDatasetIndex{0})[0], "f1.Width");
    TS_ASSERT_EQUALS(m_model->getWidths(TableDatasetIndex{0})[1], "f1.FWHM");
  }

  void test_that_getEISF_will_return_an_empty_vector_if_there_are_no_EISFs() {
    auto const workspace2 = createWorkspaceWithTextAxis(2, getNoEISFLabels());
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(m_workspace, workspace2);

    TS_ASSERT(m_model->getEISF(TableDatasetIndex{1}).empty());
  }

  void test_that_getEISF_will_return_the_EISF_parameter_names() {
    addWorkspacesToModel(m_workspace);

    TS_ASSERT_EQUALS(m_model->getEISF(TableDatasetIndex{0})[0], "f0.EISF");
    TS_ASSERT_EQUALS(m_model->getEISF(TableDatasetIndex{0})[1], "f1.EISF");
  }

  void test_that_getWidthSpectrum_will_return_the_width_spectrum_number() {
    addWorkspacesToModel(m_workspace);

    TS_ASSERT_EQUALS(m_model->getWidthSpectrum(0, TableDatasetIndex{0}).get(), 1);
    TS_ASSERT_EQUALS(m_model->getWidthSpectrum(1, TableDatasetIndex{0}).get(), 2);
  }

  void test_that_getEISFSpectrum_will_return_none_when_there_are_no_EISFs() {
    auto const workspace2 = createWorkspaceWithTextAxis(2, getNoEISFLabels());
    m_ads->addOrReplace("Name2", workspace2);

    addWorkspacesToModel(m_workspace, workspace2);

    TS_ASSERT(!m_model->getEISFSpectrum(0, TableDatasetIndex{1}));
  }

  void test_that_getEISFSpectrum_will_return_the_EISF_spectrum_number() {
    addWorkspacesToModel(m_workspace);

    TS_ASSERT_EQUALS(m_model->getEISFSpectrum(0, TableDatasetIndex{0}).get(), 0);
    TS_ASSERT_EQUALS(m_model->getEISFSpectrum(1, TableDatasetIndex{0}).get(), 3);
  }

  void test_that_setActiveWidth_will_replace_spectrum_in_single_mode() {
    addWorkspacesToModel(m_workspace);

    m_model->setActiveWidth(0, TableDatasetIndex{0}, true);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0}).size(), 1);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0})[0].value, 1);

    m_model->setActiveWidth(1, TableDatasetIndex{0}, true);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0}).size(), 1);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0})[0].value, 2);
  }

  void test_that_setActiveWidth_will_append_spectrum_in_multiple_mode() {
    addWorkspacesToModel(m_workspace);

    m_model->setActiveWidth(0, TableDatasetIndex{0}, false);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0}).size(), 1);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0})[0].value, 1);

    m_model->setActiveWidth(1, TableDatasetIndex{0}, false);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0}).size(), 2);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0})[0].value, 2);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0})[1].value, 1);
  }

  void test_that_setActiveWidth_will_add_separate_spectrum() {
    auto const workspace2 = createWorkspaceWithTextAxis(4, getParameterLabels());
    m_ads->addOrReplace("Name2", workspace2);
    addWorkspacesToModel(m_workspace, workspace2);

    m_model->setActiveWidth(0, TableDatasetIndex{0}, true);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0}).size(), 1);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0})[0].value, 1);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{1}).size(), 0);
    m_model->setActiveWidth(0, TableDatasetIndex{1}, true);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0}).size(), 1);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0})[0].value, 1);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{1}).size(), 1);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{1})[0].value, 1);
  }

  void test_that_setActiveEISF_will_replace_spectrum_in_single_mode() {
    addWorkspacesToModel(m_workspace);

    m_model->setActiveEISF(0, TableDatasetIndex{0}, true);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0}).size(), 1);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0})[0].value, 0);

    m_model->setActiveEISF(1, TableDatasetIndex{0}, true);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0}).size(), 1);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0})[0].value, 3);
  }

  void test_that_setActiveEISF_will_append_spectrum_in_multiple_mode() {
    addWorkspacesToModel(m_workspace);

    m_model->setActiveEISF(0, TableDatasetIndex{0}, false);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0}).size(), 1);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0})[0].value, 0);

    m_model->setActiveEISF(1, TableDatasetIndex{0}, false);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0}).size(), 2);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0})[0].value, 3);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0})[1].value, 0);
  }

  void test_that_setActiveEISF_will_add_separate_spectrum() {
    auto const workspace2 = createWorkspaceWithTextAxis(4, getParameterLabels());
    m_ads->addOrReplace("Name2", workspace2);
    addWorkspacesToModel(m_workspace, workspace2);

    m_model->setActiveEISF(0, TableDatasetIndex{0}, true);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0}).size(), 1);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0})[0].value, 0);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{1}).size(), 0);

    m_model->setActiveEISF(0, TableDatasetIndex{1}, true);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0}).size(), 1);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{0})[0].value, 0);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{1}).size(), 1);
    TS_ASSERT_EQUALS(m_model->getSpectra(TableDatasetIndex{1})[0].value, 0);
  }

private:
  template <typename Workspace, typename... Workspaces>
  void addWorkspacesToModel(Workspace const &workspace, Workspaces const &...workspaces) {
    m_model->addWorkspace(workspace->getName());
    addWorkspacesToModel(workspaces...);
  }

  void addWorkspacesToModel(MatrixWorkspace_sptr const &workspace) { m_model->addWorkspace(workspace->getName()); }

  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  std::unique_ptr<FqFitModel> m_model;
};
