#ifndef MULTIDIMENSIONAL_DBPRESENTER_H_
#define MULTIDIMENSIONAL_DBPRESENTER_H_

#include <vtkStructuredGrid.h>
#include <vtkFieldData.h>
#include "MantidAPI/IMDWorkspace.h"
#include "MantidVatesAPI/RebinningXMLGenerator.h"

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

typedef std::vector<int> VecExtents;

class vtkDataSetFactory;
class DLLExport MultiDimensionalDbPresenter
{

private:

  // Flag indicates successful execution.
  bool m_isExecuted;

  // Rebinned dataset in form of MDWorkspace.
  Mantid::API::IMDWorkspace_sptr m_workspace;

  //Verify that execution has occured otherwise should not be able to access scalar data or mesh.
  void verifyExecution() const;

protected:

  /*Interrogates the AnalysisDataService instance to find the workspace with the expected id.
  Seam method. supports testability given that the AnalysisDataService is a singleton and therefore very hard to fake/mock.*/
  virtual void extractWorkspaceImplementation(const std::string& wsId);

public:

  /// Constructor loads data.
  MultiDimensionalDbPresenter();

  // Performs the rebinning.
  void execute(API::Algorithm& algorithm, const std::string wsId);

  /// Gets the vtk mesh;
  vtkDataSet* getMesh(RebinningXMLGenerator& serializer, vtkDataSetFactory& vtkFactory) const;

  /// Gets the vtk scalar data for the mesh. Generated scalar data is provided with the specified name.
  vtkDataArray* getScalarDataFromTimeBin(vtkDataSetFactory& vtkFactory) const;

  /// Gets the vtk scalar data for the mesh. Generated scalar data is provided with a specified name.
  vtkDataArray* getScalarDataFromTime(vtkDataSetFactory& vtkFactory) const;

  /// Gets the number of timesteps in the workspace.
  size_t getNumberOfTimesteps() const;

  /// Get the actual timestep values to use.
  std::vector<double> getTimesteps() const;

  /// Get the actual cycle values to use.
  std::vector<int> getCycles() const;

  /// Get x axis name so that it may be applied to labels.
  std::string getXAxisName() const;

  /// Get y axis name so that it may be applied to labels.
  std::string getYAxisName() const;

  /// Get z axis name so that it may be applied to labels.
  std::string getZAxisName() const;

  /// Destructor
  ~MultiDimensionalDbPresenter();

  /// Get extents
  VecExtents getExtents() const;
};
}
}


#endif /* MULTIDIMENSIONALDB_PRESENTER_H_ */
