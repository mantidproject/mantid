#ifndef MANTID_CRYSTAL_CALIBRATIONHELPERS_H_
#define MANTID_CRYSTAL_CALIBRATIONHELPERS_H_

#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Crystal {

/** CalibrationHelpers : TODO: DESCRIPTION

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
class CalibrationHelpers {
public:
  static void
  updateSourceParams(boost::shared_ptr<const Geometry::IComponent> bankConst,
                     boost::shared_ptr<Geometry::ParameterMap> pmap,
                     boost::shared_ptr<const Geometry::ParameterMap> pmapSv);
  static void fixUpSourceParameterMap(
      boost::shared_ptr<const Geometry::Instrument> newInstrument,
      double const L0, Kernel::V3D const newSampPos,
      boost::shared_ptr<const Geometry::ParameterMap> const pmapOld);
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_CALIBRATIONHELPERS_H_ */