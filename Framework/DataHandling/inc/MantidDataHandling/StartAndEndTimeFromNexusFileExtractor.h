#ifndef MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTOR_H_
#define MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTOR_H_

#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/DateAndTime.h"
#include <string>

namespace Mantid {
namespace DataHandling {
/** Extracts the start and the end time from a Nexus file.
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
Mantid::Kernel::DateAndTime DLLExport
extractStartTime(const std::string &filename);
Mantid::Kernel::DateAndTime DLLExport
extractEndTime(const std::string &filename);
} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTOR_H_ */
