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

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at:
<https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport DataBlock {
public:
  DataBlock();
  DataBlock(const Mantid::NeXus::NXInt &data);
  DataBlock(int numberOfperiods, size_t numberOfSpectra,
            size_t numberOfChannels);

  virtual ~DataBlock();

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
