#ifndef MANTID_DATAOBJECTS_MDFRAMESTOSPECIALCOORDINATESYTEM_H_
#define MANTID_DATAOBJECTS_MDFRAMESTOSPECIALCOORDINATESYTEM_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/System.h"
#include "boost/optional.hpp"
namespace Mantid {
namespace DataObjects {

/** MDFrameFromMDWorkspace: Each dimension of the MDWorkspace contains an
    MDFrame. The acutal frame which is common to all dimensions is extracted.

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport MDFramesToSpecialCoordinateSystem {
public:
  boost::optional<Mantid::Kernel::SpecialCoordinateSystem>
  operator()(const Mantid::API::IMDWorkspace *workspace) const;

private:
  void checkQCompatibility(
      Mantid::Kernel::SpecialCoordinateSystem specialCoordinateSystem,
      boost::optional<Mantid::Kernel::SpecialCoordinateSystem> qFrameType)
      const;
  bool
  isUnknownFrame(Mantid::Geometry::IMDDimension_const_sptr dimension) const;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_MDFRAMESTOSPECIALCOORDINATESYTEM_H_ */
