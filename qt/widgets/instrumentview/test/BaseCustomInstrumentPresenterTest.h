// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentMocks.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneMocks.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentPresenter.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"


#include <string>
#include <utility>
using namespace Mantid::API;
using namespace testing;
using Mantid::Geometry::Instrument;
using namespace MantidQt::MantidWidgets;

class BaseCustomInstrumentPresenterTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  BaseCustomInstrumentPresenterTest() { FrameworkManager::Instance(); }

  static BaseCustomInstrumentPresenterTest *createSuite() { return new BaseCustomInstrumentPresenterTest(); }

  static void destroySuite(BaseCustomInstrumentPresenterTest *suite) { delete suite; }

  void setUp() override {
    //m_workspace = createWorkspace(4, 3);
    //m_ads = std::make_unique<SetUpADSWithWorkspace>("Name", m_workspace);
  m_model = new NiceMock<baseModelTest>();
  m_view = new NiceMock<baseViewTest>("EMU");
  m_paneView = new NiceMock<paneViewTest>();
  m_paneModel = new NiceMock<paneModelTest>();
  m_pane = new NiceMock<paneTest>(m_paneView, m_paneModel);
  m_presenter = new baseTest(m_view, m_model, m_pane);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    delete m_view;
    delete m_model;
    delete m_paneView;
    m_paneModel = NULL;
    delete m_presenter;
    m_pane->~paneTest();
  }


void test_addInstrument(){
m_presenter->setMockLayout();
m_presenter->addInstrument();
TS_ASSERT_EQUALS(m_presenter->getLayoutCount(), 1);
}

void test_startup(){
return;
}

void test_initLayout(){

return;
}

void test_setUpInstrumentAnalysisSplitter(){
auto widget = new QWidget();
EXPECT_CALL(*m_view, getQWidget()).Times(1).WillOnce(Return(widget));
EXPECT_CALL(*m_view, setupInstrumentAnalysisSplitters(widget)).Times(1);
//m_presenter->setUpInstrumentAnalysisSplitter();
}

void test_loadAndAnalysisSuccess(){
std::string path = "path_to_run";
int run = 101;
std::string status = "success";
std::pair<int, std::string> result = std::make_pair(run,status);
EXPECT_CALL(*m_model, loadData(path)).Times(1).WillOnce(Return(result));
EXPECT_CALL(*m_view, setRunQuietly(std::to_string(run)));
EXPECT_CALL(*m_model, setCurrentRun(run));
//m_presenter->loadAndAnalysis();
}
void test_loadAndAnalysisFail(){
return;
}

void test_loadRunNumber(){
return;
}
void test_loadRunNumberNoChange(){
return;
}
void test_loadRunNumberEmpty(){
return;
}

void test_initInstrument(){
return;
}

private:
  NiceMock<baseViewTest> *m_view;
  NiceMock<baseModelTest> *m_model;
  NiceMock<paneModelTest> *m_paneModel;
  NiceMock<paneViewTest> *m_paneView;
  NiceMock<paneTest> *m_pane;
  baseTest *m_presenter;
};

