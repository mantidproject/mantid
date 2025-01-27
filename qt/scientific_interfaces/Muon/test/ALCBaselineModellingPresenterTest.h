// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include <boost/scoped_ptr.hpp>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/WarningSuppressions.h"

#include "../Muon/ALCBaselineModellingPresenter.h"
#include "../Muon/IALCBaselineModellingPresenterSubscriber.h"
#include "../Muon/IALCBaselineModellingView.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace testing;
using boost::scoped_ptr;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::LinearGenerator;
using Mantid::HistogramData::Points;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockALCBaselineModellingView : public IALCBaselineModellingView {
public:
  virtual ~MockALCBaselineModellingView() = default;

  MOCK_METHOD1(subscribePresenter, void(IALCBaselineModellingPresenter *));

  MOCK_METHOD0(initialize, void());
  MOCK_CONST_METHOD0(initConnections, void());

  MOCK_CONST_METHOD0(function, std::string());
  MOCK_CONST_METHOD1(sectionRow, SectionRow(int));

  MOCK_METHOD2(setDataCurve, void(MatrixWorkspace_sptr workspace, const std::size_t &workspaceIndex));
  MOCK_METHOD2(setCorrectedCurve, void(MatrixWorkspace_sptr workspace, const std::size_t &workspaceIndex));
  MOCK_METHOD2(setBaselineCurve, void(MatrixWorkspace_sptr workspace, const std::size_t &workspaceIndex));
  MOCK_METHOD1(setFunction, void(IFunction_const_sptr));

  MOCK_CONST_METHOD0(noOfSectionRows, int());
  MOCK_METHOD1(setNoOfSectionRows, void(int));
  MOCK_METHOD2(setSectionRow, void(int, SectionRow));

  MOCK_CONST_METHOD1(sectionSelector, SectionSelector(int));
  MOCK_METHOD2(addSectionSelector, void(int, SectionSelector));
  MOCK_METHOD2(updateSectionSelector, void(int, SectionSelector));
  MOCK_METHOD1(deleteSectionSelector, void(int));

  MOCK_METHOD1(setSectionSelectors, void(const std::vector<SectionSelector> &));
  MOCK_METHOD3(updateSectionSelector, void(size_t, double, double));

  MOCK_METHOD1(removePlot, void(const QString &plotName));

  MOCK_METHOD1(displayError, void(const QString &));

  MOCK_METHOD0(help, void());

  MOCK_CONST_METHOD0(handleFitRequested, void());
  MOCK_CONST_METHOD0(handleAddSectionRequested, void());
  MOCK_CONST_METHOD1(handleRemoveSectionRequested, void(int));
  MOCK_CONST_METHOD1(handleSectionRowModified, void(int));
  MOCK_CONST_METHOD1(handleSectionSelectorModified, void(int));
};

class MockALCBaselineModellingModel : public IALCBaselineModellingModel {
public:
  virtual ~MockALCBaselineModellingModel() = default;

  MOCK_CONST_METHOD0(fittedFunction, IFunction_const_sptr());
  MOCK_CONST_METHOD0(correctedData, MatrixWorkspace_sptr());
  MOCK_CONST_METHOD2(baselineData,
                     MatrixWorkspace_sptr(IFunction_const_sptr function, const std::vector<double> &xValues));
  MOCK_CONST_METHOD0(data, MatrixWorkspace_sptr());

  MOCK_METHOD2(fit, void(IFunction_const_sptr, const std::vector<Section> &));

  MOCK_CONST_METHOD0(exportWorkspace, Mantid::API::MatrixWorkspace_sptr());
  MOCK_METHOD1(setData, void(Mantid::API::MatrixWorkspace_sptr));
  MOCK_METHOD1(setCorrectedData, void(Mantid::API::MatrixWorkspace_sptr));
  MOCK_CONST_METHOD0(exportSections, Mantid::API::ITableWorkspace_sptr());
  MOCK_CONST_METHOD0(exportModel, Mantid::API::ITableWorkspace_sptr());
};

class MockALCBaselineModellingPresenterSubscriber : public IALCBaselineModellingPresenterSubscriber {
public:
  MOCK_METHOD(void, correctedDataChanged, (), (override));
};

MATCHER_P(FunctionName, name, "") { return arg->name() == name; }

MATCHER_P3(FunctionParameter, param, value, delta, "") { return fabs(arg->getParameter(param) - value) < delta; }

GNU_DIAG_ON_SUGGEST_OVERRIDE

class ALCBaselineModellingPresenterTest : public CxxTest::TestSuite {
  std::unique_ptr<MockALCBaselineModellingView> m_view;
  MockALCBaselineModellingModel *m_model;
  std::unique_ptr<MockALCBaselineModellingPresenterSubscriber> m_parentPresenter;
  std::unique_ptr<ALCBaselineModellingPresenter> m_presenter;

  // To save myself some typing
  IALCBaselineModellingView::SectionRow sectionRow(double min, double max) {
    return std::make_pair(QString::number(min), QString::number(max));
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running otherl tests
  static ALCBaselineModellingPresenterTest *createSuite() { return new ALCBaselineModellingPresenterTest(); }
  static void destroySuite(ALCBaselineModellingPresenterTest *suite) { delete suite; }

  ALCBaselineModellingPresenterTest() {
    FrameworkManager::Instance(); // To make sure everything is initialized
  }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockALCBaselineModellingView>>();
    auto model = std::make_unique<NiceMock<MockALCBaselineModellingModel>>();
    m_model = model.get();
    m_parentPresenter = std::make_unique<NiceMock<MockALCBaselineModellingPresenterSubscriber>>();

    m_presenter = std::make_unique<ALCBaselineModellingPresenter>(m_view.get(), std::move(model));
    m_presenter->initialize();
    m_presenter->setSubscriber(m_parentPresenter.get());
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_parentPresenter));
  }

  // Creates a workspace with x = [1,2,3...size], y = x + deltaY and e = 1
  MatrixWorkspace_sptr createTestWs(size_t size, double deltaY = 0) {
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 1, size, size);

    ws->setHistogram(0, Points(size, LinearGenerator(1, 1)), Counts(size, LinearGenerator(1 + deltaY, 1)),
                     CountStandardDeviations(size, 1));

    return ws;
  }

  void test_updateDataCurve() {
    const auto dataWorkspace = createTestWs(3, 1);

    ON_CALL(*m_view, noOfSectionRows()).WillByDefault(Return(3));
    ON_CALL(*m_model, data()).WillByDefault(Return(dataWorkspace));

    EXPECT_CALL(*m_view, setDataCurve(dataWorkspace, 0)).Times(1);

    m_presenter->updateDataCurve();
  }

  void test_updateCorrectedCurve() {
    const auto correctedWorkspace = createTestWs(3, 2);
    ON_CALL(*m_model, correctedData()).WillByDefault(Return(correctedWorkspace));

    EXPECT_CALL(*m_view, setCorrectedCurve(correctedWorkspace, 0)).Times(1);
    EXPECT_CALL(*m_parentPresenter, correctedDataChanged()).Times(1);

    m_presenter->updateCorrectedCurve();
  }

  void test_updateCorrectedCurve_toEmpty() {
    ON_CALL(*m_model, correctedData()).WillByDefault(Return(MatrixWorkspace_sptr()));

    EXPECT_CALL(*m_view, removePlot(QString("Corrected"))).Times(1);
    EXPECT_CALL(*m_parentPresenter, correctedDataChanged()).Times(1);

    m_presenter->updateCorrectedCurve();
  }

  void test_fittedFunctionChanged() {
    IFunction_const_sptr function = FunctionFactory::Instance().createInitialized("name=FlatBackground,A0=5");
    const auto dataWorkspace = createTestWs(3);
    const auto xValues = dataWorkspace->x(0).rawData();
    const auto baselineWorkspace = createTestWs(3, 2);

    ON_CALL(*m_model, fittedFunction()).WillByDefault(Return(function));
    ON_CALL(*m_model, data()).WillByDefault(Return(dataWorkspace));
    ON_CALL(*m_model, baselineData(function, xValues)).WillByDefault(Return(baselineWorkspace));

    EXPECT_CALL(*m_view, setBaselineCurve(baselineWorkspace, 0)).Times(1);

    m_presenter->updateFunction();
    m_presenter->updateBaselineCurve();
  }

  void test_fittedFunctionChanged_toEmpty() {
    ON_CALL(*m_model, fittedFunction()).WillByDefault(Return(IFunction_const_sptr()));

    EXPECT_CALL(*m_view, setFunction(IFunction_const_sptr()));
    EXPECT_CALL(*m_view, removePlot(QString("Baseline")));

    m_presenter->updateFunction();
    m_presenter->updateBaselineCurve();
  }

  void test_addSection() {
    ON_CALL(*m_model, data()).WillByDefault(Return(createTestWs(10)));
    ON_CALL(*m_view, noOfSectionRows()).WillByDefault(Return(1));

    Expectation tableExtended = EXPECT_CALL(*m_view, setNoOfSectionRows(2));

    EXPECT_CALL(*m_view, setSectionRow(1, Pair(QString("1"), QString("10")))).After(tableExtended);

    EXPECT_CALL(*m_view, addSectionSelector(1, Pair(1, 10)));

    m_presenter->addSection();
  }

  void test_addSection_toEmptyWS() {
    ON_CALL(*m_model, data()).WillByDefault(Return(MatrixWorkspace_sptr()));

    EXPECT_CALL(*m_view, noOfSectionRows()).Times(0);
    EXPECT_CALL(*m_view, setSectionRow(_, _)).Times(0);
    EXPECT_CALL(*m_view, addSectionSelector(_, _)).Times(0);
    EXPECT_CALL(*m_view, displayError(_)).Times(1);

    m_presenter->addSection();
  }

  void test_removeSection() {
    ON_CALL(*m_view, noOfSectionRows()).WillByDefault(Return(3));
    ON_CALL(*m_view, sectionRow(0)).WillByDefault(Return(sectionRow(1, 2)));
    ON_CALL(*m_view, sectionRow(1)).WillByDefault(Return(sectionRow(3, 4)));
    ON_CALL(*m_view, sectionRow(2)).WillByDefault(Return(sectionRow(5, 6)));

    Expectation tableShrinked = EXPECT_CALL(*m_view, setNoOfSectionRows(2));

    EXPECT_CALL(*m_view, setSectionRow(0, Pair(QString("1"), QString("2")))).After(tableShrinked);
    EXPECT_CALL(*m_view, setSectionRow(1, Pair(QString("5"), QString("6")))).After(tableShrinked);

    ExpectationSet selectorsCleared;

    selectorsCleared += EXPECT_CALL(*m_view, deleteSectionSelector(0));
    selectorsCleared += EXPECT_CALL(*m_view, deleteSectionSelector(1));
    selectorsCleared += EXPECT_CALL(*m_view, deleteSectionSelector(2));

    EXPECT_CALL(*m_view, addSectionSelector(0, Pair(1, 2))).After(selectorsCleared);
    EXPECT_CALL(*m_view, addSectionSelector(1, Pair(5, 6))).After(selectorsCleared);

    m_presenter->removeSection(1);
  }

  void test_onSectionSelectorModified() {
    ON_CALL(*m_view, sectionSelector(5)).WillByDefault(Return(std::make_pair(1, 2)));

    EXPECT_CALL(*m_view, setSectionRow(5, Pair(QString("1"), QString("2"))));

    m_presenter->onSectionSelectorModified(5);
  }

  void test_onSectionRowModified() {
    IALCBaselineModellingView::SectionRow row(QString("3"), QString("4"));
    ON_CALL(*m_view, sectionRow(4)).WillByDefault(Return(row));

    EXPECT_CALL(*m_view, updateSectionSelector(4, Pair(3, 4)));
    m_presenter->onSectionRowModified(4);
  }

  void test_fit() {
    ON_CALL(*m_view, noOfSectionRows()).WillByDefault(Return(2));
    ON_CALL(*m_view, sectionRow(0)).WillByDefault(Return(sectionRow(10, 20)));
    ON_CALL(*m_view, sectionRow(1)).WillByDefault(Return(sectionRow(40, 55)));

    ON_CALL(*m_view, function()).WillByDefault(Return(std::string("name=FlatBackground,A0=3")));

    EXPECT_CALL(*m_model, fit(AllOf(FunctionName("FlatBackground"), FunctionParameter("A0", 3, 1E-8)),
                              ElementsAre(Pair(10, 20), Pair(40, 55))));
    EXPECT_CALL(*m_parentPresenter, correctedDataChanged()).Times(1);

    m_presenter->fit();
  }

  void test_fit_exception() {
    // Any valid set of sections
    ON_CALL(*m_view, noOfSectionRows()).WillByDefault(Return(1));
    ON_CALL(*m_view, sectionRow(0)).WillByDefault(Return(sectionRow(1, 2)));

    // Valid function
    ON_CALL(*m_view, function()).WillByDefault(Return(std::string("name=FlatBackground,A0=3")));

    std::string errorMsg = "Bad error";

    ON_CALL(*m_model, fit(_, _)).WillByDefault(Throw(std::runtime_error(errorMsg)));
    EXPECT_CALL(*m_view, displayError(QString::fromStdString(errorMsg)));

    m_presenter->fit();
  }

  void test_fit_badFunction() {
    // Any valid set of sections
    ON_CALL(*m_view, noOfSectionRows()).WillByDefault(Return(1));
    ON_CALL(*m_view, sectionRow(0)).WillByDefault(Return(sectionRow(1, 2)));

    // Invalid function
    ON_CALL(*m_view, function()).WillByDefault(Return(std::string("bla-bla")));

    EXPECT_CALL(*m_model, fit(_, _)).Times(0);
    EXPECT_CALL(*m_view, displayError(_));

    m_presenter->fit();
  }

  void test_fit_emptyFunction() {
    // Any valid set of sections
    ON_CALL(*m_view, noOfSectionRows()).WillByDefault(Return(1));
    ON_CALL(*m_view, sectionRow(0)).WillByDefault(Return(sectionRow(1, 2)));

    // Empty function
    ON_CALL(*m_view, function()).WillByDefault(Return(std::string("")));

    EXPECT_CALL(*m_model, fit(_, _)).Times(0);
    EXPECT_CALL(*m_view, displayError(_));

    m_presenter->fit();
  }

  void test_fit_noSections() {
    // No sections
    ON_CALL(*m_view, noOfSectionRows()).WillByDefault(Return(0));

    // Any valid function
    ON_CALL(*m_view, function()).WillByDefault(Return(std::string("name=FlatBackground,A0=0")));

    EXPECT_CALL(*m_model, fit(_, _)).Times(0);
    EXPECT_CALL(*m_view, displayError(_));

    m_presenter->fit();
  }

  void test_helpPage() {
    EXPECT_CALL(*m_view, help()).Times(1);
    m_view->help();
  }
};
