#ifndef MANTID_GEOMETRY_QLAB_H_
#define MANTID_GEOMETRY_QLAB_H_

#include "MantidKernel/MDUnit.h"
#include "MantidKernel/System.h"
#include "MantidKernel/UnitLabel.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidGeometry/DllConfig.h"
#include <memory>

namespace Mantid {
namespace Geometry {

/** QLab : Q in the lab frame MDFrame.

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
class MANTID_GEOMETRY_DLL QLab : public MDFrame{
public:

  QLab();
  virtual ~QLab();
  Mantid::Kernel::UnitLabel getUnitLabel() const;
  const Mantid::Kernel::MDUnit& getMDUnit() const;
  bool canConvertTo(const Mantid::Kernel::MDUnit& otherUnit) const;
  virtual std::string name() const;
  QLab *clone() const;

  // Type name
  static const std::string QLabName;

private:

  /// Fixed to be inverse angstroms
  const std::unique_ptr<const Mantid::Kernel::MDUnit> m_unit;

};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_QLAB_H_ */
