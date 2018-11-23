// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTFITPLOTPRESENTERTEST_H_
#define MANTIDQT_INDIRECTFITPLOTPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

//#include "IndirectFitPlotModel.h"
#include "IndirectFitPlotView.h"
#include "IndirectFitPlotPresenter.h"
#include "IndirectFittingModel.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/WarningSuppressions.h"
//#include "MantidAPI/IAlgorithm.h"
//#include "MantidAPI/IFunction.h"
//#include "MantidAPI/MatrixWorkspace.h"
//#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
//using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockIndirectFitPlotView : public IndirectFitPlotView {
public:
	/// Signals
	//void emitSelectedFitDataChanged(std::size_t const &index) {
	//	emit selectedFitDataChanged(index);
	//}

	//void emitPlotCurrentPreview() {
	//	emit plotCurrentPreview();
	//}

	//void emitPlotSpectrumChanged(std::size_t const &spectrum) {
	//	emit plotSpectrumChanged(spectrum);
	//}

	//void emitPlotGuessChanged(bool doPlotGuess) {
	//	emit plotGuessChanged(doPlotGuess);
	//}

	//void emitFitSelectedSpectrum() {
	//	emit fitSelectedSpectrum();
	//}

	//void emitStartXChanged(double const &value) {
	//	emit startXChanged(value);
	//}

	//void emitEndXChanged(double const &value) {
	//	emit endXChanged(value);
	//}

	//void emitHWHMMinimumChanged(double const &value) {
	//	emit hwhmMinimumChanged(value);
	//}

	//void emitHWHMMaximumChanged(double const &value) {
	//	emit hwhmMaximumChanged(value);
	//}

	//void emitHWHMChanged(double const &minimum, double const &maximum) {
	//	emit hwhmChanged(minimum, maximum);
	//}

	//void emitBackgroundChanged(double const &value) {
	//	emit backgroundChanged(value);
	//}

	// Public methods
	//MOCK_CONST_METHOD0(getSelectedSpectrum, std::size_t());
};

class MockIndirectFittingModel : public IndirectFittingModel {
public:
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

class IndirectFitPlotPresenterTest : public CxxTest::TestSuite {
public:
	/// Needed to make sure everything is initialized
	IndirectFitPlotPresenterTest() { FrameworkManager::Instance(); }

	static IndirectFitPlotPresenterTest *createSuite() {
		return new IndirectFitPlotPresenterTest();
	}

	static void destroySuite(IndirectFitPlotPresenterTest *suite) {
		delete suite;
	}

	void setUp() override {
		m_view = new NiceMock<MockIndirectFitPlotView>();
		m_model = new NiceMock<MockIndirectFittingModel>();
		m_presenter = new IndirectFitPlotPresenter(m_model, m_view);

		//SetUpADSWithWorkspace ads("WorkspaceName", createWorkspace(10));
		//m_model->addWorkspace("WorkspaceName");
	}

	void tearDown() override {
		AnalysisDataService::Instance().clear();

		TS_ASSERT(Mock::VerifyAndClearExpectations(m_view));
		TS_ASSERT(Mock::VerifyAndClearExpectations(m_model));

		delete m_presenter;
		delete m_model;
		// delete m_view; - causes an error
	}

	void test_test() {}

private:
	MockIndirectFitPlotView *m_view;
	MockIndirectFittingModel *m_model;
	IndirectFitPlotPresenter *m_presenter;
};

#endif 