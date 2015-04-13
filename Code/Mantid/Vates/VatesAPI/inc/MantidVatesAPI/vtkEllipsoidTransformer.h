#ifndef MANTID_VATES_VTKELLIPSOIDTRANSFORMER_H_
#define MANTID_VATES_VTKELLIPSOIDTRANSFORMER_H_

#include "MantidKernel/System.h"
#include <vtkSmartPointer.h>
#include <vector>
class vtkTransform;
namespace Mantid
{
namespace Kernel
{
  class V3D;
}
}

namespace Mantid
{
namespace VATES
{
/** 
 * Creates a vtkTransform for ellipsoids to rotate them into the correct direction.
 @date 122/02/2015

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

class DLLExport vtkEllipsoidTransformer
{
public:
  vtkEllipsoidTransformer();
  ~vtkEllipsoidTransformer();
  vtkSmartPointer<vtkTransform> generateTransform(std::vector<Mantid::Kernel::V3D> directions);
private:
  Mantid::Kernel::V3D rotateVector(Mantid::Kernel::V3D original, Mantid::Kernel::V3D rotationAxis, double angle);
};
}
}
#endif