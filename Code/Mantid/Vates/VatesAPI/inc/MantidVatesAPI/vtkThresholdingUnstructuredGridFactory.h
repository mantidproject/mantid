#ifndef MANTID_VATES_VTKUNSTRUCTUREDGRIDFACTORY_H_
#define MANTID_VATES_VTKUNSTRUCTUREDGRIDFACTORY_H_

/** Concrete implementation of vtkDataSetFactory. Creates a vtkUnStructuredGrid. Uses Thresholding technique
 * to create sparse 4D representation of data.

 @author Owen Arnold, Tessella plc
 @date 24/01/2010

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

#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidAPI/IMDWorkspace.h"
#include <vtkUnstructuredGrid.h>
#include <vtkFloatArray.h>
#include <vtkCellData.h>
#include <vtkHexahedron.h>

namespace Mantid
{
namespace VATES
{

template<typename TimeMapper>
class DLLExport vtkThresholdingUnstructuredGridFactory: public vtkDataSetFactory
{
public:

  /// Constructor
  vtkThresholdingUnstructuredGridFactory(const std::string& scalarname,
      const double timestep, double minThreshold = -10000, double maxThreshold = 10000);

  /// Assignment operator
  vtkThresholdingUnstructuredGridFactory& operator=(const vtkThresholdingUnstructuredGridFactory<TimeMapper>& other);

  /// Copy constructor.
  vtkThresholdingUnstructuredGridFactory(const vtkThresholdingUnstructuredGridFactory<TimeMapper>& other);

  /// Destructor
  ~vtkThresholdingUnstructuredGridFactory();

  /// Initialize the object with a workspace.
  virtual void initialize(Mantid::API::IMDWorkspace_sptr workspace);

  /// Factory method
  vtkDataSet* create() const;

  vtkDataSet* createMeshOnly() const;

  vtkFloatArray* createScalarArray() const;

  typedef std::vector<std::vector<std::vector<UnstructuredPoint> > > PointMap;

  typedef std::vector<std::vector<UnstructuredPoint> > Plane;

  typedef std::vector<UnstructuredPoint> Column;

  virtual std::string getFactoryTypeName() const
  {
    return "vtkThresholdingUnstructuredGridFactory";
  }

protected:

  virtual void validate() const;

private:

  void validateWsNotNull() const;

  /// Image from which to draw.
  Mantid::API::IMDWorkspace_sptr m_workspace;

  /// timestep obtained from framework.
  double m_timestep;

  /// Create a hexahedron.
  inline vtkHexahedron* createHexahedron(PointMap& pointMap, const int& i, const int& j, const int& k) const;

  /// Name of the scalar to provide on mesh.
  std::string m_scalarName;

  /// Time mapper.
  TimeMapper m_timeMapper;

  /// Threshold for signal value. below which, we do not provide unstructured topologies for.
  double m_minThreshold;

  /// Threshold for signal value, above which, we do not provide unstructured topologies for.
  double m_maxThreshold;

};

}
}

#endif
