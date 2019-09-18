// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_ALFViewPresenterTest_H_
#define MANTID_CUSTOMINTERFACES_ALFViewPresenterTest_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "../ALFView_presenter.h"
#include "../ALFView_view.h"

#include<string>
#include<QString>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace testing;


GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockALFView_view : public ALFView_view {

public:
  MOCK_METHOD0(getRunNumber, int());
  MOCK_METHOD1(setRunQuietly, void(const QString));

  //MOCK_METHOD0(initialize, void());
  //MOCK_METHOD2(setTimeRange, void(double, double));
  //MOCK_METHOD1(checkBoxAutoChanged, void(int));
  //void requestLoading() { emit loadRequested(); }
};

class MockALFView_model : public ALFView_model {

public:
  MOCK_METHOD1(loadAndAnalysis, void(const std:string));

};



GNU_DIAG_ON_SUGGEST_OVERRIDE

class ALFView_presenterTest : public CxxTest::TestSuite {
  MockALFView_view *m_view;
  ALFViiew_presenter *m_presenter;


public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALFView_presenterTest *createSuite() {
    return new ALFView_presenterTest();
  }
  static void destroySuite(ALFView_presenterTest *suite) { delete suite; }

  ALFView_presenterTest() {
    FrameworkManager::Instance(); // To make sure everything is initialized
  }

  void setUp() override {
    m_view = new NiceMock<MockALFView_view>();
    m_presenter = new ALFView_presenter(nullptr, m_view);
    m_presenter->initialize();
    // Set some valid default return values for the view mock object getters
    ON_CALL(*m_view, getRunNumber()).WillByDefault(Return(1568));
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view));
    delete m_presenter;
    delete m_view;
  }

  void test_initialize() {
    MockALFView_view view;
    ALFView_presenter presenter(nullptr, &view);
    EXPECT_CALL(view, initialize());
    presenter.initialize();
  }

  void test_defaultLoad() {
    InSequence s;
    EXPECT_CALL(*m_view, disableAll());

    EXPECT_CALL(
        *m_view,
        setDataCurve(
            AllOf(WorkspaceX(0, 0, 1350, 1E-8), WorkspaceX(0, 1, 1360, 1E-8),
                  WorkspaceX(0, 2, 1370, 1E-8), WorkspaceY(0, 0, 0.150, 1E-3),
                  WorkspaceY(0, 1, 0.143, 1E-3), WorkspaceY(0, 2, 0.128, 1E-3)),
            0));

    EXPECT_CALL(*m_view, enableAll());

    TS_ASSERT_THROWS_NOTHING(m_view->requestLoading());
  }

};

#endif /* MANTID_CUSTOMINTERFACES_ALFVIEWPRESENTERTEST_H_ */
