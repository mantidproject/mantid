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

  void test_that_data_block_produces_generator_which_generates_range() {
    // Arrange
    int64_t min = 2;
    int64_t max = 8;
    DataBlock dataBlock;
    // Act
    dataBlock.setMinSpectrumID(min);
    dataBlock.setMaxSpectrumID(max);
    auto generator = dataBlock.getGenerator();

    // Assert
    std::vector<int64_t> expected = {2, 3, 4, 5, 6, 7, 8};
    size_t index = 0;
    for (; !generator->isDone(); generator->next(), ++index) {
      TSM_ASSERT_EQUALS("Should take elements out of the DataBlock interval",
                        expected[index], generator->getValue());
    }

    TSM_ASSERT_EQUALS("Should have been incremented 7 times", index,
                      expected.size());
  }

  void test_that_two_data_blocks_are_equal() {
    // Arrange
    DataBlock block1(1, 10, 3);
    block1.setMinSpectrumID(5);
    block1.setMaxSpectrumID(15);

    // Act + Assert
    TSM_ASSERT("Should be equal", block1 == block1);
  }

  void test_that_two_data_blocks_are_not_equal() {
    // Arrange
    DataBlock block1(1, 10, 3);
    block1.setMinSpectrumID(5);
    block1.setMaxSpectrumID(15);

    DataBlock block2(2, 10, 3);
    block2.setMinSpectrumID(5);
    block2.setMaxSpectrumID(15);

    // Act + Assert
    TSM_ASSERT("Should not be equal", !(block1 == block2));
  }
};

#endif /* MANTID_DATAHANDLING_DATABLOCKTEST_H_ */
