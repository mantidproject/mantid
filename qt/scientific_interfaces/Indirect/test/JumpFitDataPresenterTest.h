// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_JUMPFITDATAPRESENTERTEST_H_
#define MANTIDQT_JUMPFITDATAPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidAPI/FrameworkManager.h"

using namespace Mantid::API;

class JumpFitDataPresenterTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  JumpFitDataPresenterTest() { FrameworkManager::Instance(); }

  static JumpFitDataPresenterTest *createSuite() {
    return new JumpFitDataPresenterTest();
  }

  static void destroySuite(JumpFitDataPresenterTest *suite) {
    delete suite;
  }

  // void setUp() override {
  //  m_view = std::make_unique<NiceMock<MockIIndirectFitDataView>>();
  //  m_model = std::make_unique<NiceMock<MockIndirectFitDataModel>>();
  //  m_table = createEmptyTableWidget(5, 5);

  //  m_dataTablePresenter = std::make_unique<IndirectDataTablePresenter>(
  //      std::move(m_model.get()), std::move(m_table.get()));
  //  m_presenter = std::make_unique<IndirectFitDataPresenter>(
  //      std::move(m_model.get()), std::move(m_view.get()),
  //      std::move(m_dataTablePresenter));

  //  SetUpADSWithWorkspace m_ads("WorkspaceName", createWorkspace(5));
  //  m_model->addWorkspace("WorkspaceName");
  //}

  // void tearDown() override {
  //  AnalysisDataService::Instance().clear();

  //  TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
  //  TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

  //  deleteSetup();
  //}

  void test_test() {}
};
#endif