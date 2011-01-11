
#ifndef GENERATESTRUCTUREDGRID_H_
#define GENERATESTRUCTUREDGRID_H_

/** Creates a vtkStructuredGrid (mesh only) from a MDImage.

 @author Owen Arnold, Tessella plc
 @date 11/01/2010

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
#include "MDDataObjects/MDWorkspace.h"
#include <vtkDataSet.h>

namespace Mantid
{
namespace VATES
{
class GenerateStructuredGrid
{
public:

  GenerateStructuredGrid(Mantid::MDDataObjects::MDWorkspace_sptr workspace);

  ~GenerateStructuredGrid();

  //Generate a structured mesh from the workspace;
  vtkDataSet* execute();

private:
  Mantid::MDDataObjects::MDWorkspace_sptr m_workspace;
};
}
}



#endif /* GENERATESTRUCTUREDGRID_H_ */
