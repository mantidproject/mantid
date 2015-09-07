#ifndef MANTID_GEOMETRY_GENERALFRAME_H_
#define MANTID_GEOMETRY_GENERALFRAME_H_

#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/System.h"
#include "MantidKernel/UnitLabel.h"
#include <memory>

namespace Mantid {
namespace Geometry {

/** GeneralFrame : Any MDFrame that isn't related to momemtum transfer

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
class DLLExport GeneralFrame : public MDFrame {
public:
  GeneralFrame(const std::string& frameName, const Kernel::UnitLabel& unit);
  GeneralFrame(const std::string& frameName, std::unique_ptr<Mantid::Kernel::MDUnit> unit);
  virtual ~GeneralFrame();
  Kernel::UnitLabel getUnitLabel() const;
  const Kernel::MDUnit &getMDUnit() const;
  bool canConvertTo(const Kernel::MDUnit &otherUnit) const;
  std::string name() const;
  virtual GeneralFrame* clone() const;
private:
  /// Label unit
  const std::unique_ptr<Mantid::Kernel::MDUnit> m_unit;
  /// Frame name
  const std::string m_frameName;
};
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_GENERALFRAME_H_ */
