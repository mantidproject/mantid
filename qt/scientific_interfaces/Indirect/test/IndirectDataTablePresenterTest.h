// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTDATATABLEPRESENTERTEST_H_
#define MANTIDQT_INDIRECTDATATABLEPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "IndirectFittingModel.h"
#include "IndirectDataTablePresenter.h"

#include "MantidKernel/WarningSuppressions.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockIndirectDataTableModel : public IndirectFittingModel {
public:
	MOCK_CONST_METHOD0(isMultiFit, bool());

private:
	std::string sequentialFitOutputName() const override { return ""; };
	std::string simultaneousFitOutputName() const override { return ""; };
	std::string singleFitOutputName(std::size_t index,
		std::size_t spectrum) const override {
		UNUSED_ARG(index);
		UNUSED_ARG(spectrum);
		return "";
	};

	std::vector<std::string> getSpectrumDependentAttributes() const override {
		return{};
	};
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectDataTablePresenterTest : public CxxTest::TestSuite {
public:
	/// Needed to make sure everything is initialized
	IndirectDataTablePresenterTest() { FrameworkManager::Instance(); }

	static IndirectDataTablePresenterTest *createSuite() {
		return new IndirectDataTablePresenterTest();
	}

	static void destroySuite(IndirectDataTablePresenterTest *suite) {
		delete suite;
	}

	void setUp() override {
		m_model = std::make_unique<NiceMock<MockIndirectDataTableModel>>();
		createEmptyTableWidget(5, 5);
		m_presenter = std::make_unique<IndirectDataTablePresenter>(std::move(m_model.get()), std::move(m_table.get()));

		SetUpADSWithWorkspace m_ads("WorkspaceName", createWorkspace(5));
		m_model->addWorkspace("WorkspaceName");
	}

	void createEmptyTableWidget(int const &columns, int const &rows) {
		m_table = std::make_unique<QTableWidget>(columns, rows);
		for (auto i = 0; i < columns; ++i)
			for (auto j = 0; j < rows; ++j)
				m_table->setItem(j, i, new QTableWidgetItem("test"));
	}

	void tearDown() override {
		AnalysisDataService::Instance().clear();

		TS_ASSERT(Mock::VerifyAndClearExpectations(m_table.get()));
		TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

		m_presenter.reset();
		m_model.reset();
		m_table.reset();
	}

	///----------------------------------------------------------------------
	/// Unit tests to check for successful presenter instantiation
	///----------------------------------------------------------------------

	void test_that_the_model_has_been_instantiated_correctly() {
		std::size_t const selectedSpectrum(3);
		ON_CALL(*m_model, isMultiFit()).WillByDefault(Return(false));

		EXPECT_CALL(*m_model, isMultiFit()).Times(1).WillOnce(Return(false));

		m_model->isMultiFit();
	}

	void
	test_that_invoking_setStartX_will_alter_the_relevant_column_in_the_table() {
		int const startXColumn(2);

		m_presenter->setStartX(2.2, 0, 0);

		for (auto i = 0; i < m_table->rowCount(); ++i)
			TS_ASSERT_EQUALS(m_table->item(i, startXColumn)->text().toStdString(), "2.2")
	}

	///----------------------------------------------------------------------
	/// Unit Tests that test the signals (only the view emits signals here)
	///----------------------------------------------------------------------

	void test_test() {}

	///----------------------------------------------------------------------
	/// Unit Tests that test the methods and slots of the view
	///----------------------------------------------------------------------

	void test_test2() {}

private:
	std::unique_ptr<QTableWidget> m_table;
	std::unique_ptr<MockIndirectDataTableModel> m_model;
	std::unique_ptr<IndirectDataTablePresenter> m_presenter;

	SetUpADSWithWorkspace *m_ads;
};

#endif