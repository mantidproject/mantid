// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneMocks.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"

#include <string>
#include <utility>

using namespace Mantid::API;
using namespace testing;
using Mantid::Geometry::Instrument;
using namespace MantidQt::MantidWidgets;

class PlotFitAnalysisPanePresenterTest : public CxxTest::TestSuite {
public:
  PlotFitAnalysisPanePresenterTest() { FrameworkManager::Instance(); }

  static PlotFitAnalysisPanePresenterTest *createSuite() { return new PlotFitAnalysisPanePresenterTest(); }

  static void destroySuite(PlotFitAnalysisPanePresenterTest *suite) { delete suite; }

  void setUp() override {
    m_view = new NiceMock<MockPlotFitAnalysisPaneView>();
    m_model = new MockPlotFitAnalysisPaneModel();
    m_presenter = new PlotFitAnalysisPanePresenter(m_view, m_model);
    m_workspaceName = "test";
    m_range = std::make_pair(0.0, 1.0);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    delete m_view;
    delete m_presenter;
    m_model = nullptr;
  }

  void test_fitClicked() {
    // set name via addSpectrum
    EXPECT_CALL(*m_view, addSpectrum(m_workspaceName)).Times(1);
    m_presenter->addSpectrum(m_workspaceName);

    EXPECT_CALL(*m_view, getRange()).Times(1).WillOnce(Return(m_range));
    EXPECT_CALL(*m_model, doFit(m_workspaceName, m_range)).Times(1);

    m_presenter->fitClicked();
  }

  void test_addSpectrum() {
    EXPECT_CALL(*m_view, addSpectrum(m_workspaceName)).Times(1);
    m_presenter->addSpectrum(m_workspaceName);
  }

  void test_that_calculateEstimate_is_not_called_when_the_current_workspace_name_is_blank() {
    EXPECT_CALL(*m_view, displayWarning("Could not update estimate: data has not been extracted.")).Times(1);

    m_presenter->updateEstimateClicked();
  }

  void test_that_calculateEstimate_is_called_as_expected() {
    EXPECT_CALL(*m_view, addSpectrum(m_workspaceName)).Times(1);
    m_presenter->addSpectrum(m_workspaceName);

    EXPECT_CALL(*m_view, getRange()).Times(1).WillOnce(Return(m_range));
    EXPECT_CALL(*m_model, calculateEstimate(m_workspaceName, m_range)).Times(1);

    m_presenter->updateEstimateClicked();
  }

private:
  NiceMock<MockPlotFitAnalysisPaneView> *m_view;
  MockPlotFitAnalysisPaneModel *m_model;
  PlotFitAnalysisPanePresenter *m_presenter;

  std::string m_workspaceName;
  std::pair<double, double> m_range;
};
