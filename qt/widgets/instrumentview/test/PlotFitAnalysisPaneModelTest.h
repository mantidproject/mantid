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

  void test_that_calculateEstimate_returns_zero_parameters_if_the_workspace_does_not_exist_in_the_ADS() {
    AnalysisDataService::Instance().clear();

    const auto function = m_model->calculateEstimate(m_workspaceName, m_range);

    TS_ASSERT_EQUALS(function->asString(), "name=FlatBackground,A0=0;name=Gaussian,Height=0,PeakCentre=0,Sigma=0");
    TS_ASSERT(!m_model->hasEstimate());
  }

  void test_that_calculateEstimate_returns_an_estimate_if_the_workspace_does_exist_in_the_ADS() {
    const auto function = m_model->calculateEstimate(m_workspaceName, m_range);

    TS_ASSERT_EQUALS(function->asString(), "name=FlatBackground,A0=2;name=Gaussian,Height=0,"
                                           "PeakCentre=0.5,Sigma=0");
    TS_ASSERT(m_model->hasEstimate());
  }

private:
  std::unique_ptr<PlotFitAnalysisPaneModel> m_model;

  MatrixWorkspace_sptr m_workspace;
  std::string m_workspaceName;
  std::pair<double, double> m_range;
};
