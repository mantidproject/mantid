#ifndef MANTID_API_DETECTORINFO_H_
#define MANTID_API_DETECTORINFO_H_

#include "MantidAPI/DllConfig.h"

#include <vector>

namespace Mantid {
using detid_t = int32_t;
namespace Geometry {
class Instrument;
}
namespace API {

/** DetectorInfo : TODO: DESCRIPTION

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
class MANTID_API_DLL DetectorInfo {
public:
  DetectorInfo(const Geometry::Instrument &instrument);

  const std::vector<detid_t> detectorIDs() const;

private:
  std::vector<detid_t> m_detectorIDs;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_DETECTORINFO_H_ */
