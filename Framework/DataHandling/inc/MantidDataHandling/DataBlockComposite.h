#ifndef MANTID_DATAHANDLING_DATABLOCK_COMPOSITE_H_
#define MANTID_DATAHANDLING_DATABLOCK_COMPOSITE_H_

#include "MantidDataHandling/DataBlock.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {

class DLLExport DataBlockComposite : public DataBlock {
public:
  int64_t getMinSpectrumID() const override;
  void setMinSpectrumID(int64_t) override;

  int64_t getMaxSpectrumID() const override;
  void setMaxSpectrumID(int64_t) override;

  size_t getNumberOfSpectra() const override;

  std::unique_ptr<DataBlockGenerator> getGenerator() const override;

  void addDataBlock(DataBlock dataBlock);
  std::vector<DataBlock> getIntervals();
  DataBlockComposite operator+(const DataBlockComposite&  other);

private:
  std::vector<DataBlock> m_dataBlocks;
};

/**
* Populates a DataBlockComposite with DataBlocks which are extracted from a
* shared array.
* @param detBlockComposite: the detector block composite which will get
* populated
* @param indexContainer: the container of indices
* @param nArray: the number of array elements
* @param numberOfPeriods: the number of periods
* @param numberOfChannels: the number of channels
* @param numberOfSpectra: the number of spectra
*/
template<typename T>
void DLLExport populateDataBlockCompositeWithContainer(
  DataBlockComposite &dataBlockComposite, T &indexContainer,
  int64_t nArray, int numberOfPeriods, size_t numberOfChannels, size_t numberOfSpectra) {
  // Find all intervals among the index array (this assumes that spectrum index
  // increases monotonically, else we would have to sort first)
  int64_t startValue = indexContainer[0];
  int64_t previousValue = startValue;

  for (int64_t arrayIndex = 1; arrayIndex < nArray; ++arrayIndex) {
    auto isSequential = (indexContainer[arrayIndex] - previousValue) == 1;
    if (!isSequential) {
      // We must have completed an interval, we create a DataBlock and add it
      DataBlock dataBlock(numberOfPeriods, numberOfChannels, numberOfSpectra);
      dataBlock.setMinSpectrumID(startValue);
      dataBlock.setMaxSpectrumID(previousValue);
      dataBlockComposite.addDataBlock(dataBlock);

      // Now reset the startValue to the beginning of the new index
      startValue = indexContainer[arrayIndex];
    }

    // Set the previous value to the current value;
    previousValue = indexContainer[arrayIndex];
  }

  // The last interval would not have been added
  DataBlock dataBlock(numberOfPeriods, numberOfChannels, numberOfSpectra);
  dataBlock.setMinSpectrumID(startValue);
  dataBlock.setMaxSpectrumID(previousValue);
  dataBlockComposite.addDataBlock(dataBlock);

};
//boost::shared_array<int>
}
}

#endif
