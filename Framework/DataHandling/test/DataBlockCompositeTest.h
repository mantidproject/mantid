#ifndef MANTID_DATAHANDLING_DATABLOCKCOMPOSITETEST_H_
#define MANTID_DATAHANDLING_DATABLOCKCOMPOSITETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/DataBlock.h"
#include "MantidDataHandling/DataBlockComposite.h"
#include "MantidDataHandling/DataBlockGenerator.h"
#include <boost/shared_array.hpp>

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

  void test_that_getting_dataBlocks_returns_them_sorted() {
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
    auto dataBlocks = dataBlockCompsite.getIntervals();

    // Assert
    TSM_ASSERT_EQUALS("There should be three data blocks", 3,
                      dataBlocks.size());
    TSM_ASSERT_EQUALS("The first min should be 2", min1,
                      dataBlocks[0].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The first max should be 8", max1,
                      dataBlocks[0].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The second min should be 23", min3,
                      dataBlocks[1].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The second max should be 27", max3,
                      dataBlocks[1].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The first min should be 45", min2,
                      dataBlocks[2].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The first min should be 49", max2,
                      dataBlocks[2].getMaxSpectrumID());
  }

  void
  test_that_add_number_of_spectra_is_returned_as_well_as_correct_min_and_max() {
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
    auto numberOfSpectra = dataBlockCompsite.getNumberOfSpectra();
    auto min = dataBlockCompsite.getMinSpectrumID();
    auto max = dataBlockCompsite.getMaxSpectrumID();

    // Assert
    auto expectedNumberOfSpectra = dataBlock1.getNumberOfSpectra() +
                                   dataBlock2.getNumberOfSpectra() +
                                   dataBlock3.getNumberOfSpectra();
    TSM_ASSERT_EQUALS("The total number of spectra should be the sum of the "
                      "spectra of the sub datablocks",
                      expectedNumberOfSpectra, numberOfSpectra);

    TSM_ASSERT_EQUALS("The min should be the absolute min of 2", min1, min);
    TSM_ASSERT_EQUALS("The max should be the aboslute max of 49", max2, max);
  }

  void test_adding_composites_prouduces_correct_new_composite() {
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

    int64_t min4 = 17;
    int64_t max4 = 20;
    DataBlock dataBlock4;
    dataBlock4.setMinSpectrumID(min4);
    dataBlock4.setMaxSpectrumID(max4);

    int64_t min3 = 23;
    int64_t max3 = 27;
    DataBlock dataBlock3;
    dataBlock3.setMinSpectrumID(min3);
    dataBlock3.setMaxSpectrumID(max3);

    DataBlockComposite dataBlockCompsite1;
    dataBlockCompsite1.addDataBlock(dataBlock1);
    dataBlockCompsite1.addDataBlock(dataBlock3);

    DataBlockComposite dataBlockCompsite2;
    dataBlockCompsite2.addDataBlock(dataBlock2);
    dataBlockCompsite2.addDataBlock(dataBlock4);

    // Act
    auto dataBlockCompositeAdded = dataBlockCompsite1 + dataBlockCompsite2;

    // Assert
    auto dataBlocks = dataBlockCompositeAdded.getIntervals();
    size_t expectedNumberOfDataBlocks = 4;
    TSM_ASSERT_EQUALS("Should have 4 data blocks.", expectedNumberOfDataBlocks,
                      dataBlocks.size());

    auto min = dataBlockCompositeAdded.getMinSpectrumID();
    TSM_ASSERT_EQUALS("Shouldd have a min value of 2", min1, min);

    auto max = dataBlockCompositeAdded.getMaxSpectrumID();
    TSM_ASSERT_EQUALS("Shouldd have a min value of 49", max2, max);

    auto numberOfSpectra = dataBlockCompositeAdded.getNumberOfSpectra();
    auto expectedNumberOfSpectra =
        dataBlock1.getNumberOfSpectra() + dataBlock2.getNumberOfSpectra() +
        dataBlock3.getNumberOfSpectra() + dataBlock4.getNumberOfSpectra();
    TSM_ASSERT_EQUALS("Should have full number of spectra",
                      expectedNumberOfSpectra, numberOfSpectra);
  }

  void test_that_boost_array_can_be_loaded_into_composite() {
    // Arrange
    constexpr int64_t size = 11;
    // Has intervals [1,1], [3,5], [8,11], [16, 16], [21,22]
    boost::shared_array<int> indexArray(
        new int[size]{1, 3, 4, 5, 8, 9, 10, 11, 16, 21, 22});
    DataBlockComposite composite;
    int numberOfPeriods = 1;
    size_t numberOfChannels = 100;
    size_t numberOfSpectra = 11;

    // Act
    Mantid::DataHandling::populateDataBlockCompositeWithContainer(
        composite, indexArray, size, numberOfPeriods, numberOfChannels,
        numberOfSpectra);

    // Assert
    auto intervals = composite.getIntervals();
    TSM_ASSERT_EQUALS("There should be 5 datablocks present", intervals.size(), 5);
    TSM_ASSERT_EQUALS("The min of the first data block should be 1", 1, intervals[0].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the first data block should be 1", 1, intervals[0].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The min of the second data block should be 3", 3, intervals[1].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the second data block should be 5", 5, intervals[1].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The min of the third data block should be 8", 8, intervals[2].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the third data block should be 5", 11, intervals[2].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The min of the fourth data block should be 3", 16, intervals[3].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the fourth data block should be 5", 16, intervals[3].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The min of the fifth data block should be 3", 21, intervals[4].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the fiffth data block should be 5", 22, intervals[4].getMaxSpectrumID());
  }

  void test_that_vector_can_be_loaded_into_composite() {}
};

#endif /* MANTID_DATAHANDLING_DATABLOCKCOMPOSITETEST_H_ */