#ifndef MANTID_DATAHANDLING_SCANNINGWORKSPACEHELPER_H_
#define MANTID_DATAHANDLING_SCANNINGWORKSPACEHELPER_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Quat.h"

#include <vector>

namespace Mantid {
namespace DataHandling {

/** ScanningWorkspaceBuilder : TODO: DESCRIPTION

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_DATAHANDLING_DLL ScanningWorkspaceBuilder {

public:
  enum class IndexingType { DEFAULT, TIME_ORIENTED, DETECTOR_ORIENTED };

  ScanningWorkspaceBuilder(size_t nDetectors, size_t nTimeIndexes,
                           size_t nBins);

  void setInstrument(boost::shared_ptr<const Geometry::Instrument> instrument);
  void setTimeRanges(const std::vector<
      std::pair<Kernel::DateAndTime, Kernel::DateAndTime>> &timeRanges);
  void setTimeRanges(const Kernel::DateAndTime &startTime,
                     const std::vector<double> &durations);
  void setPositions(std::vector<std::vector<Kernel::V3D>> &positions);
  void setRotations(std::vector<std::vector<Kernel::Quat>> &rotations);
  void setInstrumentAngles(std::vector<double> &instrumentAngles);

  void setIndexingType(IndexingType indexingType);

  API::MatrixWorkspace_sptr buildWorkspace();

private:
  size_t m_nDetectors;
  size_t m_nTimeIndexes;
  size_t m_nBins;

  boost::shared_ptr<const Geometry::Instrument> m_instrument;
  std::vector<std::pair<Kernel::DateAndTime, Kernel::DateAndTime>> m_timeRanges;
  std::vector<std::vector<Kernel::V3D>> m_positions;
  std::vector<std::vector<Kernel::Quat>> m_rotations;
  std::vector<double> m_instrumentAngles;

  IndexingType m_indexingType;

  void buildPositions(API::DetectorInfo &outputDetectorInfo) const;
  void buildRotations(API::DetectorInfo &outputDetectorInfo) const;
  void buildInstrumentAngles(API::DetectorInfo &outputDetectorInfo) const;

  Indexing::IndexInfo
  createTimeOrientedIndexInfo(const API::DetectorInfo &detectorInfo);
  Indexing::IndexInfo
  createDetectorOrientedIndexInfo(const API::DetectorInfo &detectorInfo);

  void verifyTimeIndexSize(size_t timeIndexSize,
                           const std::string &description) const;
  void verifyDetectorSize(size_t detectorSize,
                          const std::string &description) const;
  void validateInputs() const;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SCANNINGWORKSPACEBUILDER_H_ */
