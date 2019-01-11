// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_DATABLOCK_H_
#define MANTID_DATAHANDLING_DATABLOCK_H_

#include "MantidDataHandling/DllConfig.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

class DataBlockGenerator;

/** DataBlock: The DataBlock class holds information about a contiguous
    block of spectrum numbers. It contains information about the min
    and max number of that range as well as the number of channels and
    periods that are associated with the spectra in the nexus file.
*/

class DLLExport DataBlock {
public:
  DataBlock();
  DataBlock(const Mantid::NeXus::NXInt &data);
  DataBlock(int numberOfperiods, size_t numberOfSpectra,
            size_t numberOfChannels);

  virtual ~DataBlock() = default;

  virtual int64_t getMinSpectrumID() const;
  virtual void setMinSpectrumID(int64_t minSpecID);

  virtual int64_t getMaxSpectrumID() const;
  virtual void setMaxSpectrumID(int64_t minSpecID);

  virtual size_t getNumberOfSpectra() const;
  virtual int getNumberOfPeriods() const;
  virtual size_t getNumberOfChannels() const;

  bool operator==(const DataBlock &other) const;

  virtual std::unique_ptr<DataBlockGenerator> getGenerator() const;

protected:
  int m_numberOfPeriods;
  // The number of spectra
  size_t m_numberOfSpectra;
  // The number of time channels per spectrum (N histogram bins -1)
  size_t m_numberOfChannels;

  // minimal spectra Id (by default 1, undefined -- max_value)
  int64_t m_minSpectraID;
  // maximal spectra Id (by default 1, undefined  -- 0)
  int64_t m_maxSpectraID;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_DATABLOCK_H_ */
