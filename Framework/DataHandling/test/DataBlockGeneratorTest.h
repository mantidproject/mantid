#ifndef MANTID_DATAHANDLING_DATABLOCKGENERATORTEST_H_
#define MANTID_DATAHANDLING_DATABLOCKGENERATORTEST_H_

#include "MantidDataHandling/DataBlockGenerator.h"
#include <cxxtest/TestSuite.h>

class DataBlockGeneratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataBlockGeneratorTest *createSuite() {
    return new DataBlockGeneratorTest();
  }
  static void destroySuite(DataBlockGeneratorTest *suite) { delete suite; }

  void test_that_empty_interval_shows_up_as_done() {
    // Arrange
    std::vector<std::pair<int64_t, int64_t>> interval;
    Mantid::DataHandling::DataBlockGenerator generator(interval);

    // Act + Assert
    TSM_ASSERT("Should be done", generator.isDone())
    TSM_ASSERT_THROWS_NOTHING("Should be able to increment with postincrement",
                              generator++);
    TSM_ASSERT_THROWS_NOTHING("Should be able to increment with preincrement",
                              ++generator);
  }

  void test_that_single_interval_is_parsed_correctly() {
    // Arrange
    int64_t min = 2;
    int64_t max = 8;
    std::vector<std::pair<int64_t, int64_t>> interval{std::make_pair(min, max)};
    Mantid::DataHandling::DataBlockGenerator generator(interval);

    // Act + Assert
    TSM_ASSERT("Should be done", !generator.isDone());

    int64_t comparison = min;
    for (; !generator.isDone(); ++generator) {
      TSM_ASSERT_EQUALS("Should have a value from the interval", comparison,
                        generator.getValue());
      ++comparison;
    }

    TSM_ASSERT_EQUALS("Should have arrived at a count of 8+1", comparison,
                      max + 1);
    TSM_ASSERT("Should be done", generator.isDone());
  }

  void test_that_multiple_interval_is_parsed_correctly() {
    // Arrange
    int64_t min1 = 2;
    int64_t max1 = 5;

    int64_t min2 = 8;
    int64_t max2 = 12;

    int64_t min3 = 15;
    int64_t max3 = 19;

    std::vector<std::pair<int64_t, int64_t>> interval{
        std::make_pair(min1, max1), std::make_pair(min2, max2),
        std::make_pair(min3, max3)};
    std::vector<int64_t> expectedOutput = {2,  3,  4,  5,  8,  9,  10,
                                           11, 12, 15, 16, 17, 18, 19};

    // Act + Assert
    do_test_interval(interval, expectedOutput);
  }

  void test_that_multiple_interval_out_of_order_is_parsed_correctly() {
    // Arrange
    int64_t min1 = 8;
    int64_t max1 = 12;

    int64_t min2 = 2;
    int64_t max2 = 5;

    int64_t min3 = 15;
    int64_t max3 = 19;

    std::vector<std::pair<int64_t, int64_t>> interval{
        std::make_pair(min1, max1), std::make_pair(min2, max2),
        std::make_pair(min3, max3)};
    std::vector<int64_t> expectedOutput = {2,  3,  4,  5,  8,  9,  10,
                                           11, 12, 15, 16, 17, 18, 19};

    // Act + Assert
    do_test_interval(interval, expectedOutput);
  }

private:
  void do_test_interval(std::vector<std::pair<int64_t, int64_t>> interval,
                        std::vector<int64_t> expectedOutput) {
    Mantid::DataHandling::DataBlockGenerator generator(interval);

    TSM_ASSERT("Should be done", !generator.isDone());

    size_t index = 0;
    for (; !generator.isDone(); generator.next()) {
      TSM_ASSERT_EQUALS("Should have a value from the interval",
                        expectedOutput[index], generator.getValue());
      ++index;
    }

    TSM_ASSERT_EQUALS("There should have been 14 increments to index", index,
                      expectedOutput.size());
    TSM_ASSERT("Should be done", generator.isDone());
  }
};
#endif
