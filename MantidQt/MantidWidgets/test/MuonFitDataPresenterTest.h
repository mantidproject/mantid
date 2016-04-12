#ifndef MANTID_MANTIDWIDGETS_MUONFITDATAPRESENTERTEST_H_
#define MANTID_MANTIDWIDGETS_MUONFITDATAPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtMantidWidgets/MuonFitDataPresenter.h"
#include "MantidQtMantidWidgets/IMuonFitDataView.h"

using MantidQt::MantidWidgets::MuonFitDataPresenter;

/**
 * Mock for the view
 */
class MockMuonFitDataView : public MantidQt::MantidWidgets::IMuonFitDataView {
public:
  MOCK_CONST_METHOD0(getRuns, QStringList());
  MOCK_CONST_METHOD0(getWorkspaceIndex, unsigned int());
  MOCK_METHOD1(setWorkspaceIndex, void(unsigned int));
  MOCK_CONST_METHOD0(getStartTime, double());
  MOCK_METHOD1(setStartTime, void(double));
  MOCK_CONST_METHOD0(getEndTime, double());
  MOCK_METHOD1(setEndTime, void(double));
  MOCK_METHOD1(setPeriodVisibility, void(bool));
  MOCK_METHOD1(addGroupCheckbox, void(const QString &));
  MOCK_METHOD0(clearGroupCheckboxes, void());
  MOCK_CONST_METHOD1(isGroupSelected, bool(const QString &));
  MOCK_METHOD2(setGroupSelected, void(const QString &, bool));
  MOCK_METHOD1(setNumPeriods, void(size_t));
  MOCK_CONST_METHOD0(getPeriodSelections, QStringList());
};

class MuonFitDataPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonFitDataPresenterTest *createSuite() {
    return new MuonFitDataPresenterTest();
  }
  static void destroySuite(MuonFitDataPresenterTest *suite) { delete suite; }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};

#endif /* MANTID_MANTIDWIDGETS_MUONFITDATAPRESENTERTEST_H_ */