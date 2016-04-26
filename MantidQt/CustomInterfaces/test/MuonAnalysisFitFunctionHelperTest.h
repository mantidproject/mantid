#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONHELPERTEST_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONHELPERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtCustomInterfaces/Muon/MuonAnalysisFitFunctionHelper.h"
#include "MantidQtMantidWidgets/IFunctionBrowser.h"
#include "MantidQtMantidWidgets/IMuonFitFunctionControl.h"

using MantidQt::CustomInterfaces::MuonAnalysisFitFunctionHelper;
using MantidQt::MantidWidgets::IFunctionBrowser;
using MantidQt::MantidWidgets::IMuonFitFunctionControl;
using namespace testing;

// Mock function browser widget
class MockFunctionBrowser : public IFunctionBrowser {
public:
  QString getFunctionString() override { return QString("Test function"); }
};

// Mock muon fit property browser
class MockFitFunctionControl : public IMuonFitFunctionControl {
public:
  MOCK_METHOD1(setFunction, void(const QString &));
  MOCK_METHOD0(runFit, void());
  MOCK_METHOD0(runSequentialFit, void());
  MOCK_METHOD1(functionUpdateRequested, void(bool));
};

class MuonAnalysisFitFunctionHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonAnalysisFitFunctionHelperTest *createSuite() {
    return new MuonAnalysisFitFunctionHelperTest();
  }
  static void destroySuite(MuonAnalysisFitFunctionHelperTest *suite) {
    delete suite;
  }

  /// Run before each test to create mock objects
  void setUp() override {
    m_funcBrowser = new NiceMock<MockFunctionBrowser>();
    m_fitBrowser = new NiceMock<MockFitFunctionControl>();
    m_helper =
        new MuonAnalysisFitFunctionHelper(nullptr, m_fitBrowser, m_funcBrowser);
    testString = QString("Test function");
  }

  /// Run after each test to check expectations and remove mocks
  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_funcBrowser));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_fitBrowser));
    delete m_funcBrowser;
    delete m_fitBrowser;
    delete m_helper;
  }

  void test_updateFunction_nonSequential() {
    EXPECT_CALL(*m_fitBrowser, setFunction(testString)).Times(1);
    EXPECT_CALL(*m_fitBrowser, runFit()).Times(1);
    m_helper->updateFunction(false);
  }

  void test_updateFunction_sequential() {
    EXPECT_CALL(*m_fitBrowser, setFunction(testString)).Times(1);
    EXPECT_CALL(*m_fitBrowser, runSequentialFit()).Times(1);
    m_helper->updateFunction(true);
  }

private:
  QString testString;
  MockFunctionBrowser *m_funcBrowser;
  MockFitFunctionControl *m_fitBrowser;
  MuonAnalysisFitFunctionHelper *m_helper;
};

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONHELPERTEST_H_ */