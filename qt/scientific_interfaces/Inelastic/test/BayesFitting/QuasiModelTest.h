// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include <memory>

#include "BayesFitting/QuasiModel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace MantidQt::CustomInterfaces;

namespace {
auto &ads = Mantid::API::AnalysisDataService::Instance();
}

class QuasiModelTest : public CxxTest::TestSuite {
public:
  static QuasiModelTest *createSuite() { return new QuasiModelTest(); }

  static void destroySuite(QuasiModelTest *suite) { delete suite; }

  void setUp() override {
    m_model = std::make_unique<QuasiModel>();
    m_workspace = WorkspaceCreationHelper::create2DWorkspace(5, 4);
  }

  void tearDown() override { ads.clear(); }

  void test_setSample_will_not_set_the_sample_workspace_if_its_not_in_the_ads() {
    m_model->setSample("WorkspaceName");
    TS_ASSERT(!m_model->sample());
  }

  void test_setSample_will_set_the_sample_workspace_if_its_in_the_ads() {
    ads.addOrReplace("WorkspaceName", m_workspace);

    m_model->setSample("WorkspaceName");

    TS_ASSERT(m_model->sample());
  }

  void test_setResolution_will_not_set_the_resolution_workspace_if_its_not_in_the_ads() {
    m_model->setResolution("WorkspaceName");
    TS_ASSERT(!m_model->resolution());
  }

  void test_setResolution_will_set_the_resolution_workspace_if_its_in_the_ads() {
    ads.addOrReplace("WorkspaceName", m_workspace);

    m_model->setResolution("WorkspaceName");

    TS_ASSERT(m_model->resolution());
  }

  void test_setOutputResult_will_not_set_the_result_workspace_if_its_not_in_the_ads() {
    m_model->setOutputResult("WorkspaceName");
    TS_ASSERT(!m_model->outputResult());
  }

  void test_setOutputResult_will_set_the_result_workspace_if_its_in_the_ads() {
    ads.addOrReplace("WorkspaceName", m_workspace);

    m_model->setOutputResult("WorkspaceName");

    TS_ASSERT(m_model->outputResult());
  }

  void test_setOutputProbability_will_not_set_the_probability_workspace_if_its_not_in_the_ads() {
    m_model->setOutputProbability("WorkspaceName");
    TS_ASSERT(!m_model->outputProbability());
  }

  void test_setOutputProbability_will_set_the_probability_workspace_if_its_in_the_ads() {
    ads.addOrReplace("WorkspaceName", m_workspace);

    m_model->setOutputProbability("WorkspaceName");

    TS_ASSERT(m_model->outputProbability());
  }

  void test_setOutputFitGroup_will_not_set_the_fit_group_workspace_if_its_not_in_the_ads() {
    m_model->setOutputFitGroup("WorkspaceName");
    TS_ASSERT(!m_model->outputFitGroup());
  }

  void test_setOutputFitGroup_will_not_set_the_fit_group_as_a_matrix_workspace() {
    ads.addOrReplace("WorkspaceName", m_workspace);

    m_model->setOutputFitGroup("WorkspaceName");

    TS_ASSERT(!m_model->outputFitGroup());
  }

  void test_setOutputFitGroup_will_set_the_fit_group_workspace_if_its_in_the_ads() {
    auto group = std::make_shared<Mantid::API::WorkspaceGroup>();
    group->addWorkspace(m_workspace);
    ads.addOrReplace("WorkspaceName", group);

    m_model->setOutputFitGroup("WorkspaceName");

    TS_ASSERT(m_model->outputFitGroup());
  }

  void test_isResolution_returns_true_if_the_name_ends_in_res() {
    TS_ASSERT(m_model->isResolution("WorkspaceName_res"));
  }

  void test_isResolution_returns_false_if_the_name_does_not_end_in_res() {
    TS_ASSERT(!m_model->isResolution("WorkspaceName_red"));
  }

  void test_curveColour_returns_the_expected_colour_for_each_label() {
    TS_ASSERT_EQUALS("red", *m_model->curveColour("WorkspaceName fit 1"));
    TS_ASSERT_EQUALS("magenta", *m_model->curveColour("WorkspaceName fit 2"));
    TS_ASSERT_EQUALS("blue", *m_model->curveColour("WorkspaceName diff 1"));
    TS_ASSERT_EQUALS("cyan", *m_model->curveColour("WorkspaceName diff 2"));
  }

  void test_curveColour_returns_a_nullopt_if_the_label_is_not_recognised() {
    TS_ASSERT(!m_model->curveColour("WorkspaceName unknown"));
  }

private:
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<QuasiModel> m_model;
};
