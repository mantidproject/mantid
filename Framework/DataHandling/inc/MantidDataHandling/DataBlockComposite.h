// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DataBlock.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {

/** DataBlockComposite: The DataBlockComposite handles a collection
    of DataBlocks. It represents a set of contiguous spectrum numbers
    which are to be consumed elsewhere.
*/
class MANTID_DATAHANDLING_DLL DataBlockComposite : public DataBlock {
public:
  specnum_t getMinSpectrumID() const override;
  void setMinSpectrumID(specnum_t) override;

  specnum_t getMaxSpectrumID() const override;
  void setMaxSpectrumID(specnum_t) override;

  size_t getNumberOfSpectra() const override;
  size_t getNumberOfChannels() const override;
  size_t getNumberOfPeriods() const override;

  std::unique_ptr<DataBlockGenerator> getGenerator() const override;

  bool operator==(const DataBlockComposite &other) const;

  // DataBlockComposite only mehtods
  void addDataBlock(const DataBlock &dataBlock);
  std::vector<DataBlock> getDataBlocks();
  DataBlockComposite operator+(const DataBlockComposite &other);
  void removeSpectra(DataBlockComposite &toRemove);
  void truncate(specnum_t specMin, specnum_t specMax);
  std::vector<specnum_t> getAllSpectrumNumbers();
  bool isEmpty();

private:
  std::vector<DataBlock> m_dataBlocks;
};

/**
 * Populates a DataBlockComposite with DataBlocks which are extracted from a
 * indexable collection (array-type). Note that std::is_array does not
 * work on std::shared_array which is one of the use cases. Hence this
 * function could get abused. Monitor spectra get their own data block
 * @param dataBlockComposite: the detector block composite which will get
 * populated
 * @param indexContainer: the container of indices
 * @param nArray: the number of array elements
 * @param numberOfPeriods: the number of periods
 * @param numberOfChannels: the number of channels
 * @param monitorSpectra: a collection of monitor spectrum numbers
 */
template <typename T>
void DLLExport populateDataBlockCompositeWithContainer(DataBlockComposite &dataBlockComposite, T &indexContainer,
                                                       int64_t nArray, size_t numberOfPeriods, size_t numberOfChannels,
                                                       std::vector<specnum_t> monitorSpectra) {
  auto isMonitor = [&monitorSpectra](specnum_t index) {
    return std::find(std::begin(monitorSpectra), std::end(monitorSpectra), index) != std::end(monitorSpectra);
  };

  // Handles the case when an element is a monitor. It needs to crate a data
  // block
  // for potential specturm numbers before the monitor and a data block for
  // the
  // monitor itself
  struct HandleWhenElementIsMonitor {
    void operator()(Mantid::DataHandling::DataBlockComposite &dataBlockComposite, size_t numberOfPeriods,
                    size_t numberOfChannels, specnum_t previousValue, specnum_t startValue) {
      if (previousValue - startValue > 0) {
        auto numberOfSpectra = previousValue - startValue; /* Should be from [start,
                                                              previousValue -1]*/
        DataBlock dataBlock(numberOfPeriods, numberOfSpectra, numberOfChannels);
        dataBlock.setMinSpectrumID(startValue);
        dataBlock.setMaxSpectrumID(previousValue - 1);
        dataBlockComposite.addDataBlock(dataBlock);
      }

      // Save out the monitor
      DataBlock dataBlock(numberOfPeriods, 1, numberOfChannels);
      dataBlock.setMinSpectrumID(previousValue);
      dataBlock.setMaxSpectrumID(previousValue);
      dataBlockComposite.addDataBlock(dataBlock);
    }
  };

  // Handles the case when the element made a jump, ie there seems to
  // be a gap between neighbouring spetrum numbers. Then we need to
  // write out this range as a data block.
  struct HandleWhenElementMadeAJump {
    void operator()(Mantid::DataHandling::DataBlockComposite &dataBlockComposite, size_t numberOfPeriods,
                    size_t numberOfChannels, specnum_t previousValue, specnum_t startValue) {
      auto numberOfSpectra = previousValue - startValue + 1;
      DataBlock dataBlock(numberOfPeriods, numberOfSpectra, numberOfChannels);
      dataBlock.setMinSpectrumID(startValue);
      dataBlock.setMaxSpectrumID(previousValue);
      dataBlockComposite.addDataBlock(dataBlock);
    }
  };

  HandleWhenElementIsMonitor handleWhenElementIsMonitor;
  HandleWhenElementMadeAJump handleWhenElementMadeAJump;

  auto startValue = indexContainer[0];
  auto previousValue = startValue;
  for (int64_t arrayIndex = 1; arrayIndex < nArray; ++arrayIndex) {
    // There are two ways to write data out. Either when we have a jump of
    // the indices or there is a monitor. In case of a monitor we also need
    // to clear the data that was potentially before the monitor.

    if (isMonitor(previousValue)) {
      handleWhenElementIsMonitor(dataBlockComposite, numberOfPeriods, numberOfChannels, previousValue, startValue);
      startValue = indexContainer[arrayIndex];
    } else if ((indexContainer[arrayIndex] - previousValue) != 1) {
      // We must have completed an interval, we create a DataBlock and add
      // it
      handleWhenElementMadeAJump(dataBlockComposite, numberOfPeriods, numberOfChannels, previousValue, startValue);
      startValue = indexContainer[arrayIndex];
    }

    // Set the previous value to the current value;
    previousValue = indexContainer[arrayIndex];
  }

  // The last interval would not have been added.
  if (isMonitor(previousValue)) {
    handleWhenElementIsMonitor(dataBlockComposite, numberOfPeriods, numberOfChannels, previousValue, startValue);
  } else {
    handleWhenElementMadeAJump(dataBlockComposite, numberOfPeriods, numberOfChannels, previousValue, startValue);
  }
}
} // namespace DataHandling
} // namespace Mantid
