#ifndef MANTID_DATAHANDLING_SCANNINGWORKSPACEHELPER_H_
#define MANTID_DATAHANDLING_SCANNINGWORKSPACEHELPER_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/DateAndTime.h"

#include <vector>

namespace Mantid {
namespace DataHandling {

/** ScanningWorkspaceHelper : TODO: DESCRIPTION

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
class MANTID_DATAHANDLING_DLL ScanningWorkspaceHelper {

public:
  ScanningWorkspaceHelper(size_t nDetectors, size_t nTimeIndexes, size_t nBins);

  void setTimeRanges(std::vector<std::pair<Kernel::DateAndTime, Kernel::DateAndTime>>);
  void setTimeRanges(Kernel::DateAndTime startTime, std::vector<double> durations);

  API::MatrixWorkspace_sptr buildWorkspace();

private:
  size_t m_nDetectors;
  size_t m_nTimeIndexes;
  size_t m_nBins;

  std::vector<std::pair<Kernel::DateAndTime, Kernel::DateAndTime>> m_timeRanges;

  void verifyTimeIndexSize(size_t inputSize, std::string description);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SCANNINGWORKSPACEHELPER_H_ */