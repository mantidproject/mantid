#ifndef MANTID_VATES_VTK_MD_HISTO_HEX_FACTORY_H_
#define MANTID_VATES_VTK_MD_HISTO_HEX_FACTORY_H_

/** Concrete implementation of vtkDataSetFactory. Creates a vtkUnStructuredGrid.
 Uses Thresholding technique
 * to create sparse 3D representation of data.

 @author Owen Arnold, Tessella plc
 @date 06/05/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>

namespace Mantid {
namespace VATES {

class DLLExport vtkMDHistoHexFactory : public vtkDataSetFactory {
public:
  /// Constructor
  vtkMDHistoHexFactory(const VisualNormalization normalizationOption);

  /// Assignment operator
  vtkMDHistoHexFactory &operator=(const vtkMDHistoHexFactory &other);

  /// Copy constructor.
  vtkMDHistoHexFactory(const vtkMDHistoHexFactory &other);

  /// Destructor
  ~vtkMDHistoHexFactory() override;

  /// Initialize the object with a workspace.
  void initialize(const Mantid::API::Workspace_sptr &workspace) override;

  /// Factory method
  vtkSmartPointer<vtkDataSet>
  create(ProgressAction &progressUpdating) const override;

  std::string getFactoryTypeName() const override {
    return "vtkMDHistoHexFactory";
  }

protected:
  void validate() const override;

  vtkSmartPointer<vtkDataSet> create3Dor4D(size_t timestep,
                                           ProgressAction &update) const;

  void validateWsNotNull() const;

  void validateDimensionsPresent() const;

  /// Image from which to draw.
  Mantid::DataObjects::MDHistoWorkspace_sptr m_workspace;

  /// Normalization option
  VisualNormalization m_normalizationOption;
};
} // namespace VATES
} // namespace Mantid

#endif
