#ifndef MANTID_DATAHANDLING_DATABLOCKCOMPOSITETEST_H_
#define MANTID_DATAHANDLING_DATABLOCKCOMPOSITETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/DataBlock.h"
#include "MantidDataHandling/DataBlockComposite.h"
#include "MantidDataHandling/DataBlockGenerator.h"

using Mantid::DataHandling::DataBlock;
using Mantid::DataHandling::DataBlockComposite;

class DataBlockCompositeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataBlockCompositeTest *createSuite() {
    return new DataBlockCompositeTest();
  }
  static void destroySuite(DataBlockCompositeTest *suite) { delete suite; }

  void
  test_that_data_block_composite_produces_generator_which_generates_range() {
    // Arrange
    int64_t min1 = 2;
    int64_t max1 = 8;
    DataBlock dataBlock1;
    dataBlock1.setMinSpectrumID(min1);
    dataBlock1.setMaxSpectrumID(max1);

    int64_t min2 = 45;
    int64_t max2 = 49;
    DataBlock dataBlock2;
    dataBlock2.setMinSpectrumID(min2);
    dataBlock2.setMaxSpectrumID(max2);

    int64_t min3 = 23;
    int64_t max3 = 27;
    DataBlock dataBlock3;
    dataBlock3.setMinSpectrumID(min3);
    dataBlock3.setMaxSpectrumID(max3);

    DataBlockComposite dataBlockCompsite;
    dataBlockCompsite.addDataBlock(dataBlock1);
    dataBlockCompsite.addDataBlock(dataBlock2);
    dataBlockCompsite.addDataBlock(dataBlock3);

    // Act
    auto generator = dataBlockCompsite.getGenerator();

    // Assert
    std::vector<int64_t> expected = {2,  3,  4,  5,  6,  7,  8,  23, 24,
                                     25, 26, 27, 45, 46, 47, 48, 49};
    auto index = 0;
    for (; !generator->isDone(); generator->next(), ++index) {
      TSM_ASSERT_EQUALS("Should take elements out of the DataBlock interval",
                        expected[index], generator->getValue());
    }

    TSM_ASSERT_EQUALS("Should be equal", index, expected.size());
  }
};

#endif /* MANTID_DATAHANDLING_DATABLOCKCOMPOSITETEST_H_ */