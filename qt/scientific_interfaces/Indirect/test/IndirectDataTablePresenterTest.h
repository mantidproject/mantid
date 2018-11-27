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

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockIndirectFittingModel : public IndirectFittingModel {
public:
	MOCK_CONST_METHOD0(isMultiFit, bool());
	MOCK_CONST_METHOD0(getFittingMode, FittingMode());

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
		m_model = std::make_unique<NiceMock<MockIndirectFittingModel>>();
		m_table = std::make_unique<QTableWidget>(10, 10);
		//m_table->item(0, 0)->setText("test");
		m_presenter = std::make_unique<IndirectDataTablePresenter>(std::move(m_model.get()), std::move(m_table.get()));

		//SetUpADSWithWorkspace m_ads("WorkspaceName", createWorkspace(10));
		//m_fittingModel->addWorkspace("WorkspaceName");
	}

	void tearDown() override {
		//AnalysisDataService::Instance().clear();

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
		test_that_invoking_a_presenter_method_will_call_the_relevant_methods_in_the_model_and_view() {
		//ON_CALL(*m_model, getFittingMode())
		//	.WillByDefault(Return(FittingMode::SEQUENTIAL));

		//EXPECT_CALL(*m_model, getFittingMode())
		//	.Times(1)
		//	.WillOnce(Return(FittingMode::SEQUENTIAL));
		//EXPECT_CALL(*m_view, setMaskString(excludeRegion)).Times(1).After(getMask);
		m_presenter->setStartX(2.0, 0, 0);
		auto ggg = m_table->size();


	}

	void test_test() {}

private:
	std::unique_ptr<QTableWidget> m_table;
	std::unique_ptr<MockIndirectFittingModel> m_model;
	std::unique_ptr<IndirectDataTablePresenter> m_presenter;

	//SetUpADSWithWorkspace *m_ads;
};

#endif