#ifndef MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTOR_H_
#define MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTOR_H_

#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/DateAndTime.h"
#include <string>

namespace Mantid {
namespace DataHandling {

/** StartAndEndTimeFromNexusFileExtractor : Extracts the start and the end time
 from
 * a Nexus file.

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport StartAndEndTimeFromNexusFileExtractor {
public:
  Mantid::Kernel::DateAndTime extractStartTime(std::string filename) const;
  Mantid::Kernel::DateAndTime extractEndTime(std::string filename) const;

private:
  enum class NexusType { Muon, Processed, ISIS, TofRaw };
  enum class TimeType : unsigned char { StartTime, EndTime };
  Mantid::Kernel::DateAndTime extractDateAndTime(TimeType type,
                                                 std::string filename) const;
  NexusType whichNexusType(std::string filename) const;
  Mantid::Kernel::DateAndTime handleMuonNexusFile(TimeType type,
                                                  std::string filename) const;
  Mantid::Kernel::DateAndTime
  handleProcessedNexusFile(TimeType type, std::string filename) const;
  Mantid::Kernel::DateAndTime handleISISNexusFile(TimeType type,
                                                  std::string filename) const;
  Mantid::Kernel::DateAndTime handleTofRawNexusFile(TimeType type,
                                                    std::string filename) const;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTOR_H_ */
