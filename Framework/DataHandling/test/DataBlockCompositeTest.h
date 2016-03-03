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
        composite, indexArray, size, numberOfPeriods, numberOfSpectra,
        numberOfChannels);

    // Assert
    auto intervals = composite.getIntervals();
    TSM_ASSERT_EQUALS("There should be 5 datablocks present", intervals.size(),
                      5);
    TSM_ASSERT_EQUALS("The min of the first data block should be 1", 1,
                      intervals[0].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the first data block should be 1", 1,
                      intervals[0].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The min of the second data block should be 3", 3,
                      intervals[1].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the second data block should be 5", 5,
                      intervals[1].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The min of the third data block should be 8", 8,
                      intervals[2].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the third data block should be 5", 11,
                      intervals[2].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The min of the fourth data block should be 3", 16,
                      intervals[3].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the fourth data block should be 5", 16,
                      intervals[3].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The min of the fifth data block should be 3", 21,
                      intervals[4].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the fiffth data block should be 5", 22,
                      intervals[4].getMaxSpectrumID());
  }

  void
  test_that_removing_data_blocks_which_dont_overlap_leave_the_composite_unaffected() {
    // Arrange
    std::vector<std::pair<int64_t, int64_t>> intervals = {
        std::make_pair(2, 8), std::make_pair(10, 17), std::make_pair(34, 39)};
    auto dataBlockComposite = getSampleDataBlockComposite(intervals);

    auto copiedDataBlockComposite(dataBlockComposite);

    std::vector<std::pair<int64_t, int64_t>> removeIntervals = {
        std::make_pair(9, 9), std::make_pair(21, 27), std::make_pair(100, 210)};
    auto dataBlockCompositeForRemoval =
        getSampleDataBlockComposite(removeIntervals);

    // Act
    dataBlockComposite.removeSpectra(dataBlockCompositeForRemoval);

    // Assert
    auto original = copiedDataBlockComposite.getIntervals();
    auto newDataBlocks = dataBlockComposite.getIntervals();

    TSM_ASSERT_EQUALS("SHould have the same number of data blocks",
                      original.size(), newDataBlocks.size());
    for (size_t index = 0; index < original.size(); ++index) {
      TSM_ASSERT_EQUALS("Should have the same min spectrum",
                        original[index].getMinSpectrumID(),
                        newDataBlocks[index].getMinSpectrumID());

      TSM_ASSERT_EQUALS("Should have the same max spectrum",
                        original[index].getMaxSpectrumID(),
                        newDataBlocks[index].getMaxSpectrumID());

      TSM_ASSERT_EQUALS("Should have the same number of periods",
                        original[index].getNumberOfPeriods(),
                        newDataBlocks[index].getNumberOfPeriods());

      TSM_ASSERT_EQUALS("Should have the same number of channels",
                        original[index].getNumberOfChannels(),
                        newDataBlocks[index].getNumberOfChannels());

      TSM_ASSERT_EQUALS("Should have the same number of spectra",
                        original[index].getNumberOfSpectra(),
                        newDataBlocks[index].getNumberOfSpectra());
    }
  }

  void test_that_exact_match_removes_everything() {
    // Arrange
    std::vector<std::pair<int64_t, int64_t>> intervals = {
        std::make_pair(2, 8), std::make_pair(10, 17), std::make_pair(34, 39)};
    auto dataBlockComposite = getSampleDataBlockComposite(intervals);

    auto dataBlockCompositeForRemoval = getSampleDataBlockComposite(intervals);
    // Act
    dataBlockComposite.removeSpectra(dataBlockCompositeForRemoval);
    // Assert
    auto newDataBlocks = dataBlockComposite.getIntervals();

    // TSM_ASSERT("There should be no data blocks.", newDataBlocks.empty());
  }

  void test_that_left_hand_overlap_is_handled_correctly_scenario1() {
    // Arrange
    // Scaneario:
    //    original:     |------|
    //    removal:  |------|
    std::vector<std::pair<int64_t, int64_t>> intervals = {
        std::make_pair(5, 10)};
    auto dataBlockComposite = getSampleDataBlockComposite(intervals);
    std::vector<std::pair<int64_t, int64_t>> intervalsRemoval = {
        std::make_pair(1, 7)};
    auto dataBlockCompositeRemoval =
        getSampleDataBlockComposite(intervalsRemoval);

    // Act
    dataBlockComposite.removeSpectra(dataBlockCompositeRemoval);

    // Assert
    auto dataBlock = dataBlockComposite.getIntervals();
    TSM_ASSERT_EQUALS("Should have a single data block", 1, dataBlock.size());
    TSM_ASSERT_EQUALS("Should have a min of 8", 8,
                      dataBlock[0].getMinSpectrumID());
    TSM_ASSERT_EQUALS("Should have a max of 10", 10,
                      dataBlock[0].getMaxSpectrumID());
  }

  void test_that_left_hand_overlap_is_handled_correctly_scenario2() {
    // Arrange
    // Scaneario:
    //    original:        |------|
    //    removal:  |------|
    std::vector<std::pair<int64_t, int64_t>> intervals = {
        std::make_pair(5, 10)};
    auto dataBlockComposite = getSampleDataBlockComposite(intervals);
    std::vector<std::pair<int64_t, int64_t>> intervalsRemoval = {
        std::make_pair(1, 5)};
    auto dataBlockCompositeRemoval =
        getSampleDataBlockComposite(intervalsRemoval);

    // Act
    dataBlockComposite.removeSpectra(dataBlockCompositeRemoval);

    // Assert
    auto dataBlock = dataBlockComposite.getIntervals();
    TSM_ASSERT_EQUALS("Should have a single data block", 1, dataBlock.size());
    TSM_ASSERT_EQUALS("Should have a min of 6", 6,
                      dataBlock[0].getMinSpectrumID());
    TSM_ASSERT_EQUALS("Should have a max of 10", 10,
                      dataBlock[0].getMaxSpectrumID());
  }

  void test_that_right_hand_overlap_is_handled_correctly_scenario1() {
    // Arrange
    // Scaneario:
    //    original: |------|
    //    removal:      |------|
    std::vector<std::pair<int64_t, int64_t>> intervals = {
        std::make_pair(5, 10)};
    auto dataBlockComposite = getSampleDataBlockComposite(intervals);
    std::vector<std::pair<int64_t, int64_t>> intervalsRemoval = {
        std::make_pair(7, 12)};
    auto dataBlockCompositeRemoval =
        getSampleDataBlockComposite(intervalsRemoval);

    // Act
    dataBlockComposite.removeSpectra(dataBlockCompositeRemoval);

    // Assert
    auto dataBlock = dataBlockComposite.getIntervals();
    TSM_ASSERT_EQUALS("Should have a single data block", 1, dataBlock.size());
    TSM_ASSERT_EQUALS("Should have a min of 5", 5,
                      dataBlock[0].getMinSpectrumID());
    TSM_ASSERT_EQUALS("Should have a max of 6", 6,
                      dataBlock[0].getMaxSpectrumID());
  }

  void test_that_right_hand_overlap_is_handled_correctly_scenario2() {
    // Arrange
    // Scaneario:
    //    original: |------|
    //    removal:         |------|
    std::vector<std::pair<int64_t, int64_t>> intervals = {
        std::make_pair(5, 10)};
    auto dataBlockComposite = getSampleDataBlockComposite(intervals);
    std::vector<std::pair<int64_t, int64_t>> intervalsRemoval = {
        std::make_pair(10, 12)};
    auto dataBlockCompositeRemoval =
        getSampleDataBlockComposite(intervalsRemoval);

    // Act
    dataBlockComposite.removeSpectra(dataBlockCompositeRemoval);

    // Assert
    auto dataBlock = dataBlockComposite.getIntervals();
    TSM_ASSERT_EQUALS("Should have a single data block", 1, dataBlock.size());
    TSM_ASSERT_EQUALS("Should have a min of 5", 5,
                      dataBlock[0].getMinSpectrumID());
    TSM_ASSERT_EQUALS("Should have a max of 9", 9,
                      dataBlock[0].getMaxSpectrumID());
  }

  void test_that_fully_contained_overlap_is_handled_correctly_scenario1() {
    // Arrange
    // Scaneario:
    //    original: |------|
    //    removal:   |---|
    std::vector<std::pair<int64_t, int64_t>> intervals = {
        std::make_pair(5, 12)};
    auto dataBlockComposite = getSampleDataBlockComposite(intervals);
    std::vector<std::pair<int64_t, int64_t>> intervalsRemoval = {
        std::make_pair(7, 9)};
    auto dataBlockCompositeRemoval =
        getSampleDataBlockComposite(intervalsRemoval);

    // Act
    dataBlockComposite.removeSpectra(dataBlockCompositeRemoval);

    // Assert
    auto dataBlock = dataBlockComposite.getIntervals();
    TSM_ASSERT_EQUALS("Should have two data block", 2, dataBlock.size());
    TSM_ASSERT_EQUALS("Should have a min of 5", 5,
                      dataBlock[0].getMinSpectrumID());
    TSM_ASSERT_EQUALS("Should have a max of 6", 6,
                      dataBlock[0].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("Should have a min of 10", 10,
                      dataBlock[1].getMinSpectrumID());
    TSM_ASSERT_EQUALS("Should have a max of 12", 12,
                      dataBlock[1].getMaxSpectrumID());
  }

  void test_that_fully_contained_overlap_is_handled_correctly_scenario2() {
    // Arrange
    // Scaneario:
    //    original: |------|
    //    removal:  |---|
    std::vector<std::pair<int64_t, int64_t>> intervals = {
        std::make_pair(5, 12)};
    auto dataBlockComposite = getSampleDataBlockComposite(intervals);
    std::vector<std::pair<int64_t, int64_t>> intervalsRemoval = {
        std::make_pair(5, 9)};
    auto dataBlockCompositeRemoval =
        getSampleDataBlockComposite(intervalsRemoval);

    // Act
    dataBlockComposite.removeSpectra(dataBlockCompositeRemoval);

    // Assert
    auto dataBlock = dataBlockComposite.getIntervals();
    TSM_ASSERT_EQUALS("Should have a single data block", 1, dataBlock.size());
    TSM_ASSERT_EQUALS("Should have a min of 10", 10,
                      dataBlock[0].getMinSpectrumID());
    TSM_ASSERT_EQUALS("Should have a max of 12", 12,
                      dataBlock[0].getMaxSpectrumID());
  }

  void test_that_fully_contained_overlap_is_handled_correctly_scenario3() {
    // Arrange
    // Scaneario:
    //    original: |------|
    //    removal:    |----|
    std::vector<std::pair<int64_t, int64_t>> intervals = {
        std::make_pair(5, 12)};
    auto dataBlockComposite = getSampleDataBlockComposite(intervals);
    std::vector<std::pair<int64_t, int64_t>> intervalsRemoval = {
        std::make_pair(8, 12)};
    auto dataBlockCompositeRemoval =
        getSampleDataBlockComposite(intervalsRemoval);

    // Act
    dataBlockComposite.removeSpectra(dataBlockCompositeRemoval);

    // Assert
    auto dataBlock = dataBlockComposite.getIntervals();
    TSM_ASSERT_EQUALS("Should have a single data block", 1, dataBlock.size());
    TSM_ASSERT_EQUALS("Should have a min of 5", 5,
                      dataBlock[0].getMinSpectrumID());
    TSM_ASSERT_EQUALS("Should have a max of 7", 7,
                      dataBlock[0].getMaxSpectrumID());
  }

  void test_that_full_overlap_is_handled_correctly() {
    // Arrange
    // Scaneario:
    //    original: |------|
    //    removal: |--------|
    std::vector<std::pair<int64_t, int64_t>> intervals = {
        std::make_pair(5, 12)};
    auto dataBlockComposite = getSampleDataBlockComposite(intervals);
    std::vector<std::pair<int64_t, int64_t>> intervalsRemoval = {
        std::make_pair(4, 14)};
    auto dataBlockCompositeRemoval =
        getSampleDataBlockComposite(intervalsRemoval);

    // Act
    dataBlockComposite.removeSpectra(dataBlockCompositeRemoval);

    // Assert
    auto dataBlock = dataBlockComposite.getIntervals();
    TSM_ASSERT("Should have no data blocks", dataBlock.empty());
  }

  void
  test_that_multipiece_overlap_for_single_original_intervals_is_handled_correctly() {
    // Arrange
    // Scaneario:
    //    original:  |------------------|
    //    removal: |-----|  |--|  |-|
    std::vector<std::pair<int64_t, int64_t>> intervals = {
        std::make_pair(5, 16)};
    auto dataBlockComposite = getSampleDataBlockComposite(intervals);
    std::vector<std::pair<int64_t, int64_t>> intervalsRemoval = {
        std::make_pair(4, 7), std::make_pair(9, 10), std::make_pair(13, 13)};
    auto dataBlockCompositeRemoval =
        getSampleDataBlockComposite(intervalsRemoval);

    // Act
    dataBlockComposite.removeSpectra(dataBlockCompositeRemoval);

    // Assert
    auto dataBlock = dataBlockComposite.getIntervals();
    TSM_ASSERT_EQUALS("Should have three data blocks", 3, dataBlock.size());
    TSM_ASSERT_EQUALS("The min of the first ata block should be 8", 8,
                      dataBlock[0].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the first data block should be 8", 8,
                      dataBlock[0].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The min of the second ata block should be 11", 11,
                      dataBlock[1].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the second data block should be 12", 12,
                      dataBlock[1].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The min of the third ata block should be 14", 14,
                      dataBlock[2].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the third data block should be 16", 16,
                      dataBlock[2].getMaxSpectrumID());
  }

  void
  test_that_multipiece_overlap_for_mulitple_original_intervals_is_handled_correctly() {
    // Arrange
    // Scaneario:
    //    original:  |------------------|  |-------|
    //    removal: |-----|  |--|  |-|        |--| |--|
    std::vector<std::pair<int64_t, int64_t>> intervals = {
        std::make_pair(5, 16), std::make_pair(20, 26)};
    auto dataBlockComposite = getSampleDataBlockComposite(intervals);
    std::vector<std::pair<int64_t, int64_t>> intervalsRemoval = {
        std::make_pair(4, 7), std::make_pair(9, 10), std::make_pair(13, 13),
        std::make_pair(21, 22), std::make_pair(25, 30)};
    auto dataBlockCompositeRemoval =
        getSampleDataBlockComposite(intervalsRemoval);

    // Act
    dataBlockComposite.removeSpectra(dataBlockCompositeRemoval);

    // Assert
    auto dataBlock = dataBlockComposite.getIntervals();
    TSM_ASSERT_EQUALS("Should have three data blocks", 5, dataBlock.size());
    TSM_ASSERT_EQUALS("The min of the first ata block should be 8", 8,
                      dataBlock[0].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the first data block should be 8", 8,
                      dataBlock[0].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The min of the second ata block should be 11", 11,
                      dataBlock[1].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the second data block should be 12", 12,
                      dataBlock[1].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The min of the third ata block should be 14", 14,
                      dataBlock[2].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the third data block should be 16", 16,
                      dataBlock[2].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The min of the third ata block should be 20", 20,
      dataBlock[3].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the third data block should be 20", 20,
      dataBlock[3].getMaxSpectrumID());
    TSM_ASSERT_EQUALS("The min of the third ata block should be 23", 23,
      dataBlock[4].getMinSpectrumID());
    TSM_ASSERT_EQUALS("The max of the third data block should be 24", 24,
      dataBlock[4].getMaxSpectrumID());
  }

private:
  DataBlockComposite getSampleDataBlockComposite(
      const std::vector<std::pair<int64_t, int64_t>> &intervals) {
    DataBlockComposite composite;
    for (const auto &interval : intervals) {
      DataBlock dataBlock(1, (interval.second - interval.first + 1), 120);
      dataBlock.setMinSpectrumID(interval.first);
      dataBlock.setMaxSpectrumID(interval.second);
      composite.addDataBlock(dataBlock);
    }
    return composite;
  }
};

#endif /* MANTID_DATAHANDLING_DATABLOCKCOMPOSITETEST_H_ */