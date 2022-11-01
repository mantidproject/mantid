// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <memory>
#include <string>
#include <utility>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

class PlotFitAnalysisPaneModelTest : public CxxTest::TestSuite {
public:
  PlotFitAnalysisPaneModelTest() { FrameworkManager::Instance(); }

  static PlotFitAnalysisPaneModelTest *createSuite() { return new PlotFitAnalysisPaneModelTest(); }

  static void destroySuite(PlotFitAnalysisPaneModelTest *suite) { delete suite; }

  void setUp() override {
    m_model = std::make_unique<PlotFitAnalysisPaneModel>();

    m_workspace = WorkspaceCreationHelper::create2DWorkspace(1, 100);
    m_workspaceName = "test";
    m_range = std::make_pair<double, double>(0.0, 100.0);

    AnalysisDataService::Instance().addOrReplace(m_workspaceName, m_workspace);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    m_model.reset();
  }

  void test_that_calculateEstimate_returns_zero_peak_centre_if_the_workspace_does_not_exist_in_the_ADS() {
    AnalysisDataService::Instance().clear();

    m_model->calculateEstimate(m_workspaceName, m_range);

    TS_ASSERT_EQUALS(0.0, m_model->peakCentre());
    TS_ASSERT_EQUALS("", m_model->fitStatus());
  }

  void test_that_calculateEstimate_returns_an_estimate_if_the_workspace_does_exist_in_the_ADS() {
    m_model->calculateEstimate(m_workspaceName, m_range);

    TS_ASSERT_EQUALS(0.5, m_model->peakCentre());
    TS_ASSERT_EQUALS("", m_model->fitStatus());
  }

  void test_that_calculateEstimate_returns_zero_peak_centre_if_the_crop_range_is_invalid() {
    m_workspace = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100, 300.0);
    AnalysisDataService::Instance().addOrReplace(m_workspaceName, m_workspace);

    m_model->calculateEstimate(m_workspaceName, m_range);

    TS_ASSERT_EQUALS(0.0, m_model->peakCentre());
    TS_ASSERT_EQUALS("", m_model->fitStatus());
  }

private:
  std::unique_ptr<PlotFitAnalysisPaneModel> m_model;

  MatrixWorkspace_sptr m_workspace;
  std::string m_workspaceName;
  std::pair<double, double> m_range;
};
