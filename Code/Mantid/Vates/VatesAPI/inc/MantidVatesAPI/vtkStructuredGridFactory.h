
#ifndef GENERATESTRUCTUREDGRID_H_
#define GENERATESTRUCTUREDGRID_H_

/** Creates a vtkStructuredGrid from a MDImage.

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
#include <vtkStructuredGrid.h>
#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkCellData.h>
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidAPI/IMDWorkspace.h"

namespace Mantid
{
namespace VATES
{

template<typename TimeMapper>
class DLLExport vtkStructuredGridFactory : public vtkDataSetFactory
{
public:

  /// Constructor
  vtkStructuredGridFactory(const std::string& scalarName, const double timeValue);

  /// Assignment operator
  vtkStructuredGridFactory& operator=(const vtkStructuredGridFactory<TimeMapper>& other);

  /// Copy constructor.
  vtkStructuredGridFactory(const vtkStructuredGridFactory<TimeMapper>& other);

  /// Destructor
  ~vtkStructuredGridFactory();

  /// Constructional method. Have to explicitly ask for a mesh only version.
  static vtkStructuredGridFactory constructAsMeshOnly();

  /// Initialize the object with a workspace.
  virtual void initialize(Mantid::API::IMDWorkspace_sptr workspace);

  /// Factory method
  vtkStructuredGrid* create() const;

  /// Generates the geometry of the mesh only.
  vtkStructuredGrid* createMeshOnly() const;

  /// Generates a scalar array for signal.
  vtkFloatArray* createScalarArray() const;

protected:

  /// Validate the object.
  virtual void validate() const;

private:

  /// Private constructor for use by constructional static member
  vtkStructuredGridFactory();

  Mantid::API::IMDWorkspace_sptr m_workspace;
  std::string m_scalarName;
  double m_timeValue;
  bool m_meshOnly;
  TimeMapper m_timeMapper;
};

}
}



#endif
