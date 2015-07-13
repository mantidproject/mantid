#ifndef MANTID_GEOMETRY_MDFRAME_H_
#define MANTID_GEOMETRY_MDFRAME_H_

#include "MantidKernel/System.h"

#include "MantidKernel/MDUnit.h"
#include "MantidKernel/UnitLabel.h"

namespace Mantid {
namespace Geometry {

/** MDFrame : The coordinate frame for a dimension, or set of dimensions in a multidimensional workspace.

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
class DLLExport MDFrame {
public:
    virtual Mantid::Kernel::UnitLabel getUnitLabel() const = 0;
    virtual Mantid::Kernel::MDUnit getMDUnit() const = 0;
    virtual bool canConvertTo(const Mantid::Kernel::MDUnit& other) const = 0;
    virtual std::string name() const = 0;

};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_MDFRAME_H_ */
