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
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneMocks.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/FunctionFactory.h"

#include <string>
#include <utility>
#include<iostream>
using namespace Mantid::API;
using namespace testing;
using Mantid::Geometry::Instrument;
using namespace MantidQt::MantidWidgets;

class PlotFitAnalysisPanePresenterTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  PlotFitAnalysisPanePresenterTest() { FrameworkManager::Instance(); }

  static PlotFitAnalysisPanePresenterTest *createSuite() { return new PlotFitAnalysisPanePresenterTest(); }

  static void destroySuite(PlotFitAnalysisPanePresenterTest *suite) { delete suite; }

  void setUp() override {
  m_view = new NiceMock<paneViewTest>();
  m_model = new paneModelTest();
  m_presenter = new PlotFitAnalysisPanePresenter(m_view, m_model);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    delete m_view;
    delete m_presenter;
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

void test_doFit(){
  std::string name = "test";
  // set name via addSpectrum
  EXPECT_CALL(*m_view, addSpectrum(name)).Times(1);
  m_presenter->addSpectrum(name);
  // set up rest of test

  IFunction_sptr function = Mantid::API::FunctionFactory::Instance().createInitialized(
      "name = FlatBackground");

  EXPECT_CALL(*m_view, getFunction()).Times(1).WillOnce(Return(function));
  std::pair<double,double> range = std::make_pair(0.0,1.0);
  EXPECT_CALL(*m_view, getRange()).Times(1).WillOnce(Return(range));
  
  EXPECT_CALL(*m_view, updateFunction(function));

  m_presenter->doFit();
  TS_ASSERT_EQUALS(m_model->getCount(),1);
}

void test_addFunction(){
 
  IFunction_sptr function = Mantid::API::FunctionFactory::Instance().createInitialized(
      "name = FlatBackground");
 EXPECT_CALL(*m_view, addFunction(function)).Times(1);
 m_presenter->addFunction(function);
}

void test_addSpectrum(){
  std::string name = "test";
  EXPECT_CALL(*m_view, addSpectrum(name)).Times(1);
  m_presenter->addSpectrum(name);
}

private:
  NiceMock<paneViewTest> *m_view;
  paneModelTest *m_model;
  PlotFitAnalysisPanePresenter *m_presenter;
};

