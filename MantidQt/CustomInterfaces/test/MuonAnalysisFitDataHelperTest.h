#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAHELPERTEST_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAHELPERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/Muon/MuonAnalysisFitDataHelper.h"
#include "MantidQtMantidWidgets/IMuonFitDataSelector.h"
#include "MantidQtMantidWidgets/IWorkspaceFitControl.h"

using MantidQt::CustomInterfaces::MuonAnalysisFitDataHelper;
using MantidQt::MantidWidgets::IMuonFitDataSelector;
using MantidQt::MantidWidgets::IWorkspaceFitControl;
using namespace testing;

/// Mock data selector widget
class MockDataSelector : public IMuonFitDataSelector {
public:
  MOCK_CONST_METHOD0(getRuns, QStringList());
  MOCK_CONST_METHOD0(getWorkspaceIndex, unsigned int());
  MOCK_CONST_METHOD0(getStartTime, double());
  MOCK_CONST_METHOD0(getEndTime, double());
  MOCK_METHOD1(setNumPeriods, void(size_t));
  MOCK_CONST_METHOD0(getPeriodSelections, QStringList());
  MOCK_METHOD2(setWorkspaceDetails, void(int, const QString &));
  MOCK_METHOD1(setAvailableGroups, void(const QStringList &));
  MOCK_CONST_METHOD0(getChosenGroups, QStringList());
  MOCK_METHOD1(setWorkspaceIndex, void(unsigned int));
  MOCK_METHOD1(setStartTime, void(double));
  MOCK_METHOD1(setEndTime, void(double));
  MOCK_METHOD1(setStartTimeQuietly, void(double));
  MOCK_METHOD1(setEndTimeQuietly, void(double));
};

/// Mock fit property browser
class MockFitBrowser : public IWorkspaceFitControl {
public:
  MOCK_METHOD1(setWorkspaceName, void(const QString &));
  MOCK_METHOD1(setStartX, void(double));
  MOCK_METHOD1(setEndX, void(double));
  MOCK_METHOD1(setWorkspaceIndex, void(int));
};

class MuonAnalysisFitDataHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonAnalysisFitDataHelperTest *createSuite() {
    return new MuonAnalysisFitDataHelperTest();
  }
  static void destroySuite(MuonAnalysisFitDataHelperTest *suite) {
    delete suite;
  }

  /// Run before each test to create mock objects
  void setUp() override {
    m_dataSelector = new NiceMock<MockDataSelector>();
    m_fitBrowser = new NiceMock<MockFitBrowser>();
    m_helper = new MuonAnalysisFitDataHelper(m_fitBrowser, m_dataSelector);
  }

  /// Run after each test to check expectations and remove mocks
  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_dataSelector));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_fitBrowser));
    delete m_dataSelector;
    delete m_fitBrowser;
    delete m_helper;
  }

  void test_handleWorkspacePropertiesChanged() {
    QStringList runs;
    runs << "MUSR00015189.nxs"
         << "MUSR00015190.nxs";
    ON_CALL(*m_dataSelector, getRuns()).WillByDefault(Return(runs));
    ON_CALL(*m_dataSelector, getWorkspaceIndex()).WillByDefault(Return(0));
    ON_CALL(*m_dataSelector, getStartTime()).WillByDefault(Return(0.3));
    ON_CALL(*m_dataSelector, getEndTime()).WillByDefault(Return(9.9));
    EXPECT_CALL(*m_fitBrowser, setWorkspaceIndex(0)).Times(1);
    EXPECT_CALL(*m_fitBrowser, setStartX(0.3)).Times(1);
    EXPECT_CALL(*m_fitBrowser, setEndX(9.9)).Times(1);
    m_helper->handleWorkspacePropertiesChanged();
  }

  void test_handleSelectedGroupsChanged() { TS_FAIL("Test not implemented!"); }

  void test_handleSelectedPeriodsChanged() { TS_FAIL("Test not implemented!"); }

  void test_handleXRangeChangedGraphically() {
    EXPECT_CALL(*m_dataSelector, setStartTimeQuietly(0.4)).Times(1);
    EXPECT_CALL(*m_dataSelector, setEndTimeQuietly(9.4)).Times(1);
    m_helper->handleXRangeChangedGraphically(0.4, 9.4);
  }

private:
  MockDataSelector *m_dataSelector;
  MockFitBrowser *m_fitBrowser;
  MuonAnalysisFitDataHelper *m_helper;
};

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAHELPERTEST_H_ */