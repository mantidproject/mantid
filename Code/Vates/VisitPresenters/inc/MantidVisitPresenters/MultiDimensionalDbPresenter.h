#ifndef MULTIDIMENSIONAL_DBPRESENTER_H_
#define MULTIDIMENSIONAL_DBPRESENTER_H_

#include <vtkStructuredGrid.h>
#include <vtkFieldData.h>
#include "MDDataObjects/MDWorkspace.h"

namespace Mantid
{
namespace VATES
{
/**

 Applies indirection between visualisation framework and Mantid. This type drives data loading operations.

 @author Owen Arnold, Tessella plc
 @date 21/12/2010

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


class MultiDimensionalDbPresenter
{

private:

  // Flag indicates successful execution.
  bool m_isExecuted;

  // Rebinned dataset in form of MDWorkspace.
  Mantid::MDDataObjects::MDWorkspace_sptr m_MDWorkspace;

  // Store meta data for current operations.
  void metaDataToFieldData(vtkFieldData* fieldData, std::string metaData,
      const char* id) const;

  //Verify that execution has occured otherwise should not be able to access scalar data or mesh.
  void verifyExecution() const;

public:

  /// Constructor loads data.
  MultiDimensionalDbPresenter();

  // Performs the rebinning.
  void execute(const std::string& fileName);

  /// Gets the vtk mesh;
  vtkDataSet* getMesh() const;

  /// Gets the vtk scalar data for the mesh
  vtkDataArray* getScalarData(int timeBin) const;

  /// Gets the number of timesteps in the workspace.
  int getNumberOfTimesteps() const;

  /// Destructor
  ~MultiDimensionalDbPresenter();


};
}
}


#endif /* MULTIDIMENSIONALDB_PRESENTER_H_ */
