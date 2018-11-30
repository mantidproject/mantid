// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTFITDATAPRESENTERTEST_H_
#define MANTIDQT_INDIRECTFITDATAPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "IndirectFitDataPresenter.h"
#include "IndirectFitDataView.h"
#include "IndirectFittingModel.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/WarningSuppressions.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::IDA;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockIndirectFitDataView : public IndirectFitDataView {
public:
};

class MockIndirectFitDataModel : public IndirectFittingModel {
public:
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectFitDataPresenterTest : public CxxTest::TestSuite {
public:
	/// Needed to make sure everything is initialized
	IndirectFitDataPresenterTest() { FrameworkManager::Instance(); }

	static IndirectFitDataPresenterTest *createSuite() {
		return new IndirectFitDataPresenterTest();
	}

	static void destroySuite(IndirectFitDataPresenterTest *suite) {
		delete suite;
	}

	void setUp() override {
		//m_model = std::make_unique<NiceMock<MockIndirectDataTableModel>>();
		//createEmptyTableWidget(5, 5);
		//m_presenter = std::make_unique<IndirectDataTablePresenter>(
		//	std::move(m_model.get()), std::move(m_table.get()));

		//SetUpADSWithWorkspace m_ads("WorkspaceName", createWorkspace(5));
		//m_model->addWorkspace("WorkspaceName");
	}

	void tearDown() override {
		//AnalysisDataService::Instance().clear();

		//TS_ASSERT(Mock::VerifyAndClearExpectations(m_table.get()));
		//TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

		//m_presenter.reset();
		//m_model.reset();
		//m_table.reset();
	}

	void test_test() {}

};

#endif