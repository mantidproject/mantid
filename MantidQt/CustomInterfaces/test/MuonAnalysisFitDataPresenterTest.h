#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisDataLoader.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisFitDataPresenter.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h"
#include "MantidQtMantidWidgets/IMuonFitDataSelector.h"
#include "MantidQtMantidWidgets/IWorkspaceFitControl.h"

using MantidQt::CustomInterfaces::MuonAnalysisDataLoader;
using MantidQt::CustomInterfaces::MuonAnalysisFitDataPresenter;
using MantidQt::MantidWidgets::IMuonFitDataSelector;
using MantidQt::MantidWidgets::IWorkspaceFitControl;
using MantidQt::CustomInterfaces::Muon::DeadTimesType;
using namespace testing;

/// Mock data selector widget
class MockDataSelector : public IMuonFitDataSelector {
public:
  MOCK_CONST_METHOD0(getFilenames, QStringList());
  MOCK_CONST_METHOD0(getWorkspaceIndex, unsigned int());
  MOCK_CONST_METHOD0(getStartTime, double());
  MOCK_CONST_METHOD0(getEndTime, double());
  MOCK_METHOD1(setNumPeriods, void(size_t));
  MOCK_METHOD1(setChosenPeriod, void(const QString &));
  MOCK_CONST_METHOD0(getPeriodSelections, QStringList());
  MOCK_METHOD2(setWorkspaceDetails, void(const QString &, const QString &));
  MOCK_METHOD1(setAvailableGroups, void(const QStringList &));
  MOCK_CONST_METHOD0(getChosenGroups, QStringList());
  MOCK_METHOD1(setChosenGroup, void(const QString &));
  MOCK_METHOD1(setWorkspaceIndex, void(unsigned int));
  MOCK_METHOD1(setStartTime, void(double));
  MOCK_METHOD1(setEndTime, void(double));
  MOCK_METHOD1(setStartTimeQuietly, void(double));
  MOCK_METHOD1(setEndTimeQuietly, void(double));
  MOCK_CONST_METHOD0(getFitType, IMuonFitDataSelector::FitType());
  MOCK_CONST_METHOD0(getInstrumentName, QString());
  MOCK_CONST_METHOD0(getRuns, QString());
};

/// Mock fit property browser
class MockFitBrowser : public IWorkspaceFitControl {
public:
  MOCK_METHOD1(setWorkspaceName, void(const QString &));
  MOCK_METHOD1(setStartX, void(double));
  MOCK_METHOD1(setEndX, void(double));
  MOCK_METHOD1(setWorkspaceIndex, void(int));
  MOCK_METHOD1(allowSequentialFits, void(bool));
  MOCK_METHOD1(setWorkspaceNames, void(const QStringList &));
};

class MuonAnalysisFitDataPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonAnalysisFitDataPresenterTest *createSuite() {
    return new MuonAnalysisFitDataPresenterTest();
  }
  static void destroySuite(MuonAnalysisFitDataPresenterTest *suite) {
    delete suite;
  }

  /// Constructor
  MuonAnalysisFitDataPresenterTest()
      : m_dataLoader(DeadTimesType::None,
                     QStringList{"MUSR", "EMU", "HIFI", "ARGUS", "CHRONUS"}) {
    Mantid::API::FrameworkManager::Instance();
  }

  /// Run before each test to create mock objects
  void setUp() override {
    m_dataSelector = new NiceMock<MockDataSelector>();
    m_fitBrowser = new NiceMock<MockFitBrowser>();
    m_presenter = new MuonAnalysisFitDataPresenter(m_fitBrowser, m_dataSelector,
                                                   m_dataLoader);
  }

  /// Run after each test to check expectations and remove mocks
  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_dataSelector));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_fitBrowser));
    delete m_dataSelector;
    delete m_fitBrowser;
    delete m_presenter;
  }

  void test_handleDataPropertiesChanged() {
    ON_CALL(*m_dataSelector, getWorkspaceIndex()).WillByDefault(Return(0));
    ON_CALL(*m_dataSelector, getStartTime()).WillByDefault(Return(0.3));
    ON_CALL(*m_dataSelector, getEndTime()).WillByDefault(Return(9.9));
    EXPECT_CALL(*m_fitBrowser, setWorkspaceIndex(0)).Times(1);
    EXPECT_CALL(*m_fitBrowser, setStartX(0.3)).Times(1);
    EXPECT_CALL(*m_fitBrowser, setEndX(9.9)).Times(1);
    m_presenter->handleDataPropertiesChanged();
  }

  void test_handleSelectedDataChanged() {
    Mantid::API::Grouping grouping;
    grouping.groupNames = {"fwd", "bwd"};
    grouping.pairNames = {"long"};
    EXPECT_CALL(*m_dataSelector, getInstrumentName())
        .Times(1)
        .WillOnce(Return("MUSR"));
    EXPECT_CALL(*m_dataSelector, getRuns())
        .Times(1)
        .WillOnce(Return("15189-91"));
    EXPECT_CALL(*m_dataSelector, getChosenGroups())
        .Times(1)
        .WillOnce(Return(QStringList({"fwd", "long"})));
    EXPECT_CALL(*m_dataSelector, getPeriodSelections())
        .Times(1)
        .WillOnce(Return(QStringList({"1", "1-2"})));
    EXPECT_CALL(*m_dataSelector, getFitType())
        .Times(1)
        .WillOnce(Return(IMuonFitDataSelector::FitType::Simultaneous));
    std::vector<QString> expectedNames = {
        "MUSR00015189; Group; fwd; Asym; 1; #1",
        "MUSR00015189; Pair; long; Asym; 1; #1",
        "MUSR00015189; Group; fwd; Asym; 1-2; #1",
        "MUSR00015189; Pair; long; Asym; 1-2; #1",
        "MUSR00015190; Group; fwd; Asym; 1; #1",
        "MUSR00015190; Pair; long; Asym; 1; #1",
        "MUSR00015190; Group; fwd; Asym; 1-2; #1",
        "MUSR00015190; Pair; long; Asym; 1-2; #1",
        "MUSR00015191; Group; fwd; Asym; 1; #1",
        "MUSR00015191; Pair; long; Asym; 1; #1",
        "MUSR00015191; Group; fwd; Asym; 1-2; #1",
        "MUSR00015191; Pair; long; Asym; 1-2; #1"};
    EXPECT_CALL(*m_fitBrowser,
                setWorkspaceNames(UnorderedElementsAreArray(expectedNames)))
        .Times(1);
    m_presenter->handleSelectedDataChanged(
        grouping, MantidQt::CustomInterfaces::Muon::PlotType::Asymmetry, true);
    // test ADS here
    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_handleXRangeChangedGraphically() {
    EXPECT_CALL(*m_dataSelector, setStartTimeQuietly(0.4)).Times(1);
    EXPECT_CALL(*m_dataSelector, setEndTimeQuietly(9.4)).Times(1);
    m_presenter->handleXRangeChangedGraphically(0.4, 9.4);
  }

  void test_setAssignedFirstRun_singleWorkspace() {
    const QString wsName("MUSR00015189; Pair; long; Asym; 1; #1");
    EXPECT_CALL(*m_dataSelector,
                setWorkspaceDetails(QString("00015189"), QString("MUSR")))
        .Times(1);
    EXPECT_CALL(*m_dataSelector, setWorkspaceIndex(0u)).Times(1);
    EXPECT_CALL(*m_fitBrowser, allowSequentialFits(true)).Times(1);
    m_presenter->setAssignedFirstRun(wsName);
  }

  void test_setAssignedFirstRun_contiguousRange() {
    const QString wsName("MUSR00015189-91; Pair; long; Asym; 1; #1");
    EXPECT_CALL(*m_dataSelector,
                setWorkspaceDetails(QString("00015189-91"), QString("MUSR")))
        .Times(1);
    EXPECT_CALL(*m_dataSelector, setWorkspaceIndex(0u)).Times(1);
    EXPECT_CALL(*m_fitBrowser, allowSequentialFits(false)).Times(1);
    EXPECT_CALL(*m_dataSelector, setChosenGroup(QString("long"))).Times(1);
    EXPECT_CALL(*m_dataSelector, setChosenPeriod(QString("1"))).Times(1);
    m_presenter->setAssignedFirstRun(wsName);
  }

  void test_setAssignedFirstRun_nonContiguousRange() {
    const QString wsName("MUSR00015189-91, 15193; Pair; long; Asym; 1; #1");
    EXPECT_CALL(
        *m_dataSelector,
        setWorkspaceDetails(QString("00015189-91, 15193"), QString("MUSR")))
        .Times(1);
    EXPECT_CALL(*m_dataSelector, setWorkspaceIndex(0u)).Times(1);
    EXPECT_CALL(*m_fitBrowser, allowSequentialFits(false)).Times(1);
    EXPECT_CALL(*m_dataSelector, setChosenGroup(QString("long"))).Times(1);
    EXPECT_CALL(*m_dataSelector, setChosenPeriod(QString("1"))).Times(1);
    m_presenter->setAssignedFirstRun(wsName);
  }

  void test_setAssignedFirstRun_alreadySet() {
    const QString wsName("MUSR00015189; Pair; long; Asym; 1; #1");
    m_presenter->setAssignedFirstRun(wsName);
    EXPECT_CALL(*m_dataSelector, setWorkspaceDetails(_, _)).Times(0);
    EXPECT_CALL(*m_dataSelector, setWorkspaceIndex(_)).Times(0);
    EXPECT_CALL(*m_fitBrowser, allowSequentialFits(_)).Times(0);
    EXPECT_CALL(*m_dataSelector, setChosenGroup(QString("long"))).Times(0);
    EXPECT_CALL(*m_dataSelector, setChosenPeriod(QString("1"))).Times(0);
    m_presenter->setAssignedFirstRun(wsName);
  }

  void test_getAssignedFirstRun() {
    const QString wsName("MUSR00015189; Pair; long; Asym; 1; #1");
    m_presenter->setAssignedFirstRun(wsName);
    TS_ASSERT_EQUALS(wsName, m_presenter->getAssignedFirstRun());
  }

private:
  MockDataSelector *m_dataSelector;
  MockFitBrowser *m_fitBrowser;
  MuonAnalysisFitDataPresenter *m_presenter;
  MuonAnalysisDataLoader m_dataLoader;
};

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAPRESENTERTEST_H_ */