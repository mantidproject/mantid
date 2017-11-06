#ifndef VATES_VTK_NULL_DATA_SET
#define VATES_VTK_NULL_DATA_SET

#include "MantidKernel/System.h"

class vtkUnstructuredGrid;

namespace Mantid {
namespace VATES {

/** Generates a vtkUnstructuredGrid with a single point. Note that this is not a
 Null
 Object for a vtkDataSet.

 @date 25/02/2015

 Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport vtkNullUnstructuredGrid {

public:
  vtkNullUnstructuredGrid();

  ~vtkNullUnstructuredGrid();

  vtkUnstructuredGrid *createNullData();
};
}
}
#endif