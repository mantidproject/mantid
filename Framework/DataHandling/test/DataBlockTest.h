#ifndef MANTID_DATAHANDLING_DATABLOCKTEST_H_
#define MANTID_DATAHANDLING_DATABLOCKTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/DataBlock.h"
#include "MantidDataHandling/DataBlockGenerator.h"

using Mantid::DataHandling::DataBlock;

class DataBlockTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataBlockTest *createSuite() { return new DataBlockTest(); }
  static void destroySuite(DataBlockTest *suite) { delete suite; }

  void test_that_getting_next_when_next_is_available() {
    // Arrange
    DataBlock dataBlock;
    dataBlock.setMinSpectrumID(2);
    dataBlock.setMaxSpectrumID(17);

    // Act
    int64_t idToTest = 12;
    auto next = dataBlock.getNextSpectrumID(idToTest);

    // Assert
    int64_t expectedNext = 13;
    TSM_ASSERT_EQUALS("Should return 13.", expectedNext, idToTest);
  }

  void test_that_getting_next_when_past_range_returns_end_value() {
    // Arrange
    DataBlock dataBlock;
    dataBlock.setMinSpectrumID(2);
    dataBlock.setMaxSpectrumID(17);

    // Act
    int64_t idToTest = 19;
    auto next = dataBlock.getNextSpectrumID(idToTest);

    // Assert
    int64_t expectedNext = std::numeric_limits<int64_t>::min();
    TSM_ASSERT_EQUALS("Should return smalles int64_t.", expectedNext, idToTest);
  }

  void test_that_generation_of_indices_delivers_over_full_range() {
    // Arrange
    DataBlock dataBlock;
    int64_t min = 7;
    int64_t max = 17;
    dataBlock.setMinSpectrumID(min);
    dataBlock.setMaxSpectrumID(max);

    // Act
    auto generator = dataBlock.getGenerator();

    // Assert
    //void operator++();
    //bool isDone();
    //int64_t getCurrent();

    int64_t current = min;
    for (current; current <= max; ++current){
      auto dataItem = generator->getCurrent();
    }
  }
};

#endif /* MANTID_DATAHANDLING_DATABLOCKTEST_H_ */