// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

using SpectrumPair = std::pair<specnum_t, specnum_t>;

class DataBlockGenerator;

/** DataBlock: The DataBlock class holds information about a contiguous
    block of spectrum numbers. It contains information about the min
    and max number of that range as well as the number of channels and
    periods that are associated with the spectra in the nexus file.
*/

class MANTID_DATAHANDLING_DLL DataBlock {
public:
  DataBlock();
  DataBlock(const Mantid::NeXus::NXInt &data);
  DataBlock(size_t numberOfperiods, size_t numberOfSpectra, size_t numberOfChannels);

  virtual ~DataBlock() = default;

  virtual specnum_t getMinSpectrumID() const;
  virtual void setMinSpectrumID(specnum_t minSpecID);

  virtual specnum_t getMaxSpectrumID() const;
  virtual void setMaxSpectrumID(specnum_t minSpecID);

  virtual size_t getNumberOfSpectra() const;
  virtual size_t getNumberOfPeriods() const;
  virtual size_t getNumberOfChannels() const;

  bool operator==(const DataBlock &other) const;

  virtual std::unique_ptr<DataBlockGenerator> getGenerator() const;

protected:
  size_t m_numberOfPeriods;
  // The number of spectra
  size_t m_numberOfSpectra;
  // The number of time channels per spectrum (N histogram bins -1)
  size_t m_numberOfChannels;

  // minimal spectra Id (by default 1, undefined -- max_value)
  specnum_t m_minSpectraID;
  // maximal spectra Id (by default 1, undefined  -- 0)
  specnum_t m_maxSpectraID;
};

} // namespace DataHandling
} // namespace Mantid
