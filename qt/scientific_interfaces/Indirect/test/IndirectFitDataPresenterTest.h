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
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockIndirectFitDataView : public IndirectFitDataView {
public:
	/// Signals
	void emitSampleLoaded(QString const &name) {
		emit sampleLoaded(name);
	}

	//void emitResolutionLoaded(QString const &name) {
	//	emit resolutionLoaded(name);
	//}

	void emitAddClicked() {
		emit addClicked();
	}

	void emitRemoveClicked() {
		emit removeClicked();
	}

	void emitMultipleDataViewSelected() {
		emit multipleDataViewSelected();
	}

	void emitSingleDataViewSelected() {
		emit singleDataViewSelected();
	}

	/// Public Methods
	MOCK_CONST_METHOD0(getSelectedSample, std::string());
	MOCK_CONST_METHOD0(isMultipleDataTabSelected, bool());

	/// Public Slots

};

class MockIndirectFitDataModel : public IndirectFittingModel {
public:
	MOCK_CONST_METHOD0(isMultiFit, bool());
	MOCK_CONST_METHOD0(numberOfWorkspaces, std::size_t());

	MOCK_METHOD1(addWorkspace, void(std::string const &workspaceName));

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
		m_view = std::make_unique<NiceMock<MockIndirectFitDataView>>();
		m_model = std::make_unique<NiceMock<MockIndirectFitDataModel>>();
		m_presenter = std::make_unique<IndirectFitDataPresenter>(
			std::move(m_model.get()), std::move(m_view.get()));

		SetUpADSWithWorkspace m_ads("WorkspaceName", createWorkspace(5));
		m_model->addWorkspace("WorkspaceName");
	}

	void tearDown() override {
		AnalysisDataService::Instance().clear();

		TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
		TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

		m_presenter.reset();
		m_model.reset();
		m_view.reset();
	}

	///----------------------------------------------------------------------
	/// Unit tests to check for successful mock object instantiation
	///----------------------------------------------------------------------

	void test_that_the_model_has_been_instantiated_correctly() {
		ON_CALL(*m_model, isMultiFit()).WillByDefault(Return(false));

		EXPECT_CALL(*m_model, isMultiFit()).Times(1).WillOnce(Return(false));

		m_model->isMultiFit();
	}

	void test_that_the_view_has_been_instantiated_correctly() {
	  std::string const sampleName("SampleName_red");
		ON_CALL(*m_view, getSelectedSample()).WillByDefault(Return(sampleName));

		EXPECT_CALL(*m_view, getSelectedSample()).Times(1).WillOnce(Return(sampleName));

		m_view->getSelectedSample();
	}

	void
	test_that_invoking_a_presenter_method_will_call_the_relevant_methods_in_the_view_and_model() {
		ON_CALL(*m_view, isMultipleDataTabSelected()).WillByDefault(Return(true));
		ON_CALL(*m_model, numberOfWorkspaces()).WillByDefault(Return(2));

		Expectation isMultipleData = EXPECT_CALL(*m_view, isMultipleDataTabSelected()).Times(1).WillOnce(Return(true));
		EXPECT_CALL(*m_model, numberOfWorkspaces()).Times(1).After(isMultipleData);

		m_presenter->updateSpectraInTable(0);
	}

	///----------------------------------------------------------------------
	/// Unit Tests that test the signals call the correct methods
	///----------------------------------------------------------------------

	void test_that_the_sampleLoaded_signal_will_add_the_loaded_workspace_to_the_model() {
	  std::string const workspaceName("WorkspaceName2");
		m_ads->addOrReplace(workspaceName, createWorkspace(5));

		EXPECT_CALL(*m_model, addWorkspace(workspaceName)).Times(1);
		
		m_view->emitSampleLoaded(QString::fromStdString(workspaceName));
	}

	void test_test() {}

private:
	std::unique_ptr<MockIndirectFitDataView> m_view;
	std::unique_ptr<MockIndirectFitDataModel> m_model;
	std::unique_ptr<IndirectFitDataPresenter> m_presenter;

	SetUpADSWithWorkspace *m_ads;
};

#endif