#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMBUILDERTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMBUILDERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/HistogramBuilder.h"

using namespace Mantid::HistogramData;

class HistogramBuilderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramBuilderTest *createSuite() {
    return new HistogramBuilderTest();
  }
  static void destroySuite(HistogramBuilderTest *suite) { delete suite; }

  void test_missing_x_failure() {
    HistogramBuilder builder;
    builder.setY(5);
    TS_ASSERT_THROWS_EQUALS(builder.build(), const std::runtime_error &e,
                            std::string(e.what()),
                            "HistogramBuilder: No X data has been set");
  }

  void test_missing_y_failure() {
    HistogramBuilder builder;
    builder.setX(5);
    TS_ASSERT_THROWS_EQUALS(builder.build(), const std::runtime_error &e,
                            std::string(e.what()),
                            "HistogramBuilder: No Y data has been set");
  }

  void test_size_failures() {
    HistogramBuilder builder;
    builder.setX(5);
    builder.setY(3);
    TS_ASSERT_THROWS(builder.build(), std::logic_error);
    builder.setY(6);
    TS_ASSERT_THROWS(builder.build(), std::logic_error);
  }

  void test_build_from_size() {
    HistogramBuilder builder;
    builder.setX(5);
    builder.setY(5);
    const auto hist = builder.build();
    TS_ASSERT_EQUALS(hist.x().size(), 5);
    TS_ASSERT_EQUALS(hist.y().size(), 5);
    TS_ASSERT_EQUALS(hist.xMode(), Histogram::XMode::Points);
    TS_ASSERT_EQUALS(hist.yMode(), Histogram::YMode::Counts);
  }

  void test_build_from_size_distribution() {
    HistogramBuilder builder;
    builder.setX(5);
    builder.setY(5);
    builder.setDistribution(true);
    const auto hist = builder.build();
    TS_ASSERT_EQUALS(hist.x().size(), 5);
    TS_ASSERT_EQUALS(hist.y().size(), 5);
    TS_ASSERT_EQUALS(hist.xMode(), Histogram::XMode::Points);
    TS_ASSERT_EQUALS(hist.yMode(), Histogram::YMode::Frequencies);
  }
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMBUILDERTEST_H_ */
