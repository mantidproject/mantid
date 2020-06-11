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
  BaseCustomInstrumentPresenterTest() { FrameworkManager::Instance(); }

  static BaseCustomInstrumentPresenterTest *createSuite() { return new BaseCustomInstrumentPresenterTest(); }

  static void destroySuite(BaseCustomInstrumentPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_model = new NiceMock<MockBaseCustomInstrumentModel>();
    m_view = new NiceMock<MockBaseCustomInstrumentView>("EMU");
    m_paneView = new NiceMock<MockPlotFitAnalysisPaneView>();
    m_paneModel = new NiceMock<MockPlotFitAnalysisPaneModel>();
    m_pane =
        new NiceMock<MockPlotFitAnalysisPanePresenter>(m_paneView, m_paneModel);
  m_presenter =
      new PartMockBaseCustomInstrumentPresenter(m_view, m_model, m_pane);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    delete m_view;
    delete m_model;
    delete m_paneView;
    m_paneModel = NULL;
    delete m_presenter;
    delete m_pane;
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


EXPECT_CALL(*m_view, observeLoadRun(m_presenter->loadObserver())).Times(1);
m_presenter->setMockInitInstrument();

QWidget *widget = new QWidget();
EXPECT_CALL(*m_pane, getView()).Times(1).WillOnce(Return(m_paneView));
EXPECT_CALL(*m_paneView, getQWidget()).Times(1).WillOnce(Return(widget));
EXPECT_CALL(*m_view, setupInstrumentAnalysisSplitters(widget)).Times(1);
EXPECT_CALL(*m_view, setupHelp()).Times(1);

m_presenter->initLayout();
TS_ASSERT_EQUALS(m_presenter->getInitInstrumentCount(),1);
}

void test_setUpInstrumentAnalysisSplitter(){
auto widget = new QWidget();
EXPECT_CALL(*m_pane, getView()).Times(1).WillOnce(Return(m_paneView));
EXPECT_CALL(*m_paneView, getQWidget()).Times(1).WillOnce(Return(widget));
EXPECT_CALL(*m_view, setupInstrumentAnalysisSplitters(widget)).Times(1);
m_presenter->setUpInstrumentAnalysisSplitter();
}

void test_loadAndAnalysisSuccess(){
std::string path = "path_to_run";
int run = 101;
std::string status = "success";
std::pair<int, std::string> result = std::make_pair(run,status);
EXPECT_CALL(*m_model, loadData(path)).Times(1).WillOnce(Return(result));
EXPECT_CALL(*m_view, setRunQuietly(std::to_string(run)));
EXPECT_CALL(*m_model, setCurrentRun(run));
m_presenter->setMockSideEffects();

m_presenter->loadAndAnalysis(path);
TS_ASSERT_EQUALS(m_presenter->getLoadSideEffectsCount(),1);

}
void test_loadAndAnalysisFail(){
std::string path = "path_to_run";
int run = 101;
std::string status = "fail";

int oldRun = 42;
std::string oldPath = "old_path";
m_presenter->setCurrent(oldRun, oldPath);

std::pair<int, std::string> result = std::make_pair(run,status);
EXPECT_CALL(*m_model, loadData(path)).Times(1).WillOnce(Return(result));
EXPECT_CALL(*m_view, setRunQuietly(std::to_string(oldRun)));
EXPECT_CALL(*m_view, warningBox(status));
EXPECT_CALL(*m_model, setCurrentRun(oldRun));
m_presenter->setMockSideEffects();

m_presenter->loadAndAnalysis(path);
TS_ASSERT_EQUALS(m_presenter->getLoadSideEffectsCount(),1);

}

void test_loadRunNumber(){
m_presenter->setMockLoad();
std::string path = "path_to_file";
EXPECT_CALL(*m_view, getFile()).Times(1).WillOnce(Return(path));

m_presenter->loadRunNumber();
TS_ASSERT_EQUALS(m_presenter->getLoadCount(),1);
}
void test_loadRunNumberNoChange(){
m_presenter->setMockLoad();
std::string path = "path_to_file";
m_presenter->setCurrent(5, path);
EXPECT_CALL(*m_view, getFile()).Times(1).WillOnce(Return(path));

m_presenter->loadRunNumber();
TS_ASSERT_EQUALS(m_presenter->getLoadCount(),0);
}

void test_loadRunNumberEmpty(){
m_presenter->setMockLoad();
std::string path = "";
EXPECT_CALL(*m_view, getFile()).Times(1).WillOnce(Return(path));

m_presenter->loadRunNumber();
TS_ASSERT_EQUALS(m_presenter->getLoadCount(),0);
}

void test_initInstrument(){
return;
}

private:
  NiceMock<MockBaseCustomInstrumentView> *m_view;
  NiceMock<MockBaseCustomInstrumentModel> *m_model;
  NiceMock<MockPlotFitAnalysisPaneModel> *m_paneModel;
  NiceMock<MockPlotFitAnalysisPaneView> *m_paneView;
  NiceMock<MockPlotFitAnalysisPanePresenter> *m_pane;
  PartMockBaseCustomInstrumentPresenter *m_presenter;
};

