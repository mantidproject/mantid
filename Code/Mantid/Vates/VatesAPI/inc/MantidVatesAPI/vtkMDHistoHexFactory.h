#ifndef MANTID_VATES_VTK_MD_HISTO_HEX_FACTORY_H_
#define MANTID_VATES_VTK_MD_HISTO_HEX_FACTORY_H_

/** Concrete implementation of vtkDataSetFactory. Creates a vtkUnStructuredGrid. Uses Thresholding technique
 * to create sparse 3D representation of data. 

 @author Owen Arnold, Tessella plc
 @date 06/05/2011

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

#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include <vtkUnstructuredGrid.h>
#include <vtkFloatArray.h>
#include <vtkCellData.h>
#include <vtkHexahedron.h>
#include "MantidMDEvents/MDHistoWorkspace.h"

namespace Mantid
{
namespace VATES
{

class DLLExport vtkMDHistoHexFactory: public vtkDataSetFactory
{
public:

  /// Constructor
  vtkMDHistoHexFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarname);

  /// Assignment operator
  vtkMDHistoHexFactory& operator=(const vtkMDHistoHexFactory& other);

  /// Copy constructor.
  vtkMDHistoHexFactory(const vtkMDHistoHexFactory& other);

  /// Destructor
  ~vtkMDHistoHexFactory();

  /// Initialize the object with a workspace.
  virtual void initialize(Mantid::API::Workspace_sptr workspace);

  /// Factory method
  vtkDataSet* create(ProgressAction& progressUpdating) const;

  virtual std::string getFactoryTypeName() const
  {
    return "vtkMDHistoHexFactory";
  }

protected:

  virtual void validate() const;

  vtkDataSet* create3Dor4D(size_t timestep, bool do4D, ProgressAction & update) const;

  void validateWsNotNull() const;

  void validateDimensionsPresent() const;

  /// Image from which to draw.
  Mantid::MDEvents::MDHistoWorkspace_sptr m_workspace;

  /// Name of the scalar to provide on mesh.
  std::string m_scalarName;

  /// Threshold range.
  mutable ThresholdRange_scptr m_thresholdRange;

};

}
}

#endif
