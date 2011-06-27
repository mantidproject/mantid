#ifndef MANTID_VATES_VTKPEAKMARKER_FACTORY_H_
#define MANTID_VATES_VTKPEAKMARKER_FACTORY_H_

/** Create markers that will highlight the position of a Peak
 * from a PeaksWorkspace.
 *
 * Implementation of a vtkDataSetFactory.

 @author Janik Zikovsky
 @date 06/24/2011

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

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkUnstructuredGrid.h>

namespace Mantid
{
namespace VATES
{

class DLLExport vtkPeakMarkerFactory: public vtkDataSetFactory
{
public:

  /// Constructor
  vtkPeakMarkerFactory(const std::string& scalarname, double minThreshold = -10000, double maxThreshold = 10000);

  /// Assignment operator
  vtkPeakMarkerFactory& operator=(const vtkPeakMarkerFactory& other);

  /// Copy constructor.
  vtkPeakMarkerFactory(const vtkPeakMarkerFactory& other);

  /// Destructor
  ~vtkPeakMarkerFactory();

  /// Initialize the object with a workspace.
  virtual void initialize(Mantid::API::Workspace_sptr workspace);

  /// Factory method
  vtkDataSet* create() const;

  vtkDataSet* createMeshOnly() const;

  vtkFloatArray* createScalarArray() const;

  typedef std::vector<std::vector<std::vector<UnstructuredPoint> > > PointMap;

  typedef std::vector<std::vector<UnstructuredPoint> > Plane;

  typedef std::vector<UnstructuredPoint> Column;

  virtual std::string getFactoryTypeName() const
  {
    return "vtkPeakMarkerFactory";
  }

protected:

  virtual void validate() const;

private:

  void validateWsNotNull() const;

  void validateDimensionsPresent() const;

  /// Peaks workspace containg peaks to mark
  Mantid::API::IPeaksWorkspace_sptr m_workspace;

  /// Name of the scalar to provide on mesh.
  std::string m_scalarName;

  /// Threshold for signal value. below which, we do not provide unstructured topologies for.
  double m_minThreshold;

  /// Threshold for signal value, above which, we do not provide unstructured topologies for.
  double m_maxThreshold;

};

}
}

#endif
