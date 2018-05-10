#ifndef MANTID_GEOMETRY_HKL_H_
#define MANTID_GEOMETRY_HKL_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/System.h"
#include "MantidKernel/UnitLabel.h"
#include <memory>

namespace Mantid {
namespace Geometry {

/** HKL : HKL MDFrame

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
class MANTID_GEOMETRY_DLL HKL : public MDFrame {
public:
  HKL(const HKL &other);
  HKL &operator=(const HKL &other);
  HKL(std::unique_ptr<Kernel::MDUnit> &unit);
  HKL(Kernel::MDUnit *unit);
  static const std::string HKLName;

  // MDFrame interface
  Kernel::UnitLabel getUnitLabel() const override;
  const Kernel::MDUnit &getMDUnit() const override;
  bool setMDUnit(const Mantid::Kernel::MDUnit &newUnit) override;
  bool canConvertTo(const Kernel::MDUnit &otherUnit) const override;
  bool isQ() const override;
  bool isSameType(const MDFrame &frame) const override;
  std::string name() const override;
  HKL *clone() const override;
  Mantid::Kernel::SpecialCoordinateSystem
  equivalientSpecialCoordinateSystem() const override;

private:
  std::unique_ptr<Kernel::MDUnit> m_unit;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_HKL_H_ */
