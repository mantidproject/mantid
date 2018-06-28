#ifndef MANTID_MANTIDWIDGETS_PROGRESSABLEVIEWTEST_H
#define MANTID_MANTIDWIDGETS_PROGRESSABLEVIEWTEST_H

#include "MantidQtWidgets/Common/ProgressableView.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::MantidWidgets;

//=====================================================================================
// Functional tests
//=====================================================================================
class ProgressableViewTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProgressableViewTest *createSuite() {
    return new ProgressableViewTest();
  }
  static void destroySuite(ProgressableViewTest *suite) { delete suite; }

  ProgressableViewTest() {}

  void testSetProgressRange() {
    int min = 5;
    int max = 18;
    m_progress.setProgressRange(min, max);
    m_progress.assertRange(min, max);
  }

  void testSetProgressRangeBothZero() {
    // Set a non-zero range first
    int min = 5;
    int max = 18;
    m_progress.setProgressRange(min, max);
    // Now set start=end=0
    m_progress.setProgressRange(0, 0);
    // A 0-0 range is a special case and should not be cached, so we should
    // still have the original range
    m_progress.assertRange(min, max);
  }

  void testSetProgressRangeZeroLength() {
    int min = 7;
    int max = 7;
    m_progress.setProgressRange(min, max);
    m_progress.assertRange(min, max);
  }

  void testSetPercentageIndicator() {
    m_progress.setAsPercentageIndicator();
    m_progress.assertStyle(ProgressableView::Style::PERCENTAGE);
  }

  void testSetEndlessIndicator() {
    m_progress.setAsEndlessIndicator();
    m_progress.assertStyle(ProgressableView::Style::ENDLESS);
  }

  void testRangeNotLostChangeStyle() {
    int min = 5;
    int max = 18;
    m_progress.setProgressRange(min, max);
    m_progress.setAsEndlessIndicator();
    m_progress.assertRange(min, max);
  }

private:
  // Inner class :: fake progressable view
  class ProgressBar : public ProgressableView {
  public:
    void setProgress(int) override {}
    void clearProgress() override {}
    void assertRange(int min, int max) const {
      TS_ASSERT_EQUALS(m_min, min);
      TS_ASSERT_EQUALS(m_max, max);
    }
    void assertStyle(Style style) const { TS_ASSERT_EQUALS(m_style, style); }
  };

  ProgressBar m_progress;
};

#endif /*MANTID_MANTIDWIDGETS_PROGRESSABLEVIEWTEST_H */
