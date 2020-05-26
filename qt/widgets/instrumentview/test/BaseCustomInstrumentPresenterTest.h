// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"
//#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneView.h"
//#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneModel.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"


#include <string>
#include <utility>
#include<iostream>
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
 // m_model = new NiceMock<FullALFModelTest>();
 // m_view = new NiceMock<ALFViewTest>("ALF");
 // m_paneView = new NiceMock<paneViewTest>();
 // m_paneModel = new NiceMock<paneModelTest>();
 // m_pane = new NiceMock<paneTest>(m_paneView, m_paneModel);
 // m_presenter = new ALFCustomInstrumentPresenter(m_view, m_model,m_pane);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
  //  m_model->~FullALFModelTest();
 //   delete m_view;
 //   delete m_paneView;
 //   m_paneModel = NULL;
 //   delete m_presenter;
 //   m_pane->~paneTest();
  }


void test_addInstrument(){
return;  
// this is only called as part of initLayout
//  EXPECT_CALL(*m_pane, getView()).Times(1);
//  m_presenter->addInstrument();
// want to check initLayout is called from base class or mock base class funcs?
}

void test_startup(){
return;
}

void test_initLayout(){
return;
}

void test_setUpInstrumentAnalysisSplitter(){
return;
}

void test_loadAndAnalysisSuccess(){
return;
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
//  NiceMock<FullALFModelTest> *m_model;
//  NiceMock<ALFViewTest> *m_view;
//  NiceMock<paneViewTest> *m_paneView;
//  NiceMock<paneModelTest> *m_paneModel;
//  NiceMock<paneTest> *m_pane;
//  ALFCustomInstrumentPresenter *m_presenter;
};

