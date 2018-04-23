#ifndef MANTID_VATES_VTK_MD_HISTO_HEX4D_FACTORY_H_
#define MANTID_VATES_VTK_MD_HISTO_HEX4D_FACTORY_H_

/** Concrete implementation of vtkDataSetFactory. Creates a vtkUnStructuredGrid.
 Uses Thresholding technique
 * to create sparse 4D representation of data.

 @author Owen Arnold, Tessella plc
 @date 24/01/2010

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
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkUnstructuredGrid.h>

namespace Mantid {
namespace VATES {

template <typename TimeMapper>
class DLLExport vtkMDHistoHex4DFactory : public vtkMDHistoHexFactory {
public:
  /// Constructor
  vtkMDHistoHex4DFactory(const VisualNormalization normalizationOption,
                         const double timestep);

  /// Assignment operator
  vtkMDHistoHex4DFactory &
  operator=(const vtkMDHistoHex4DFactory<TimeMapper> &other);

  /// Copy constructor.
  vtkMDHistoHex4DFactory(const vtkMDHistoHex4DFactory<TimeMapper> &other);

  /// Destructor
  ~vtkMDHistoHex4DFactory() override;

  /// Initialize the object with a workspace.
  void initialize(const Mantid::API::Workspace_sptr &workspace) override;

  /// Factory method
  vtkSmartPointer<vtkDataSet>
  create(ProgressAction &progressUpdating) const override;

  std::string getFactoryTypeName() const override {
    return "vtkMDHistoHex4DFactory";
  }

protected:
  void validate() const override;

private:
  using PointMap = std::vector<std::vector<std::vector<UnstructuredPoint>>>;
  using Plane = std::vector<std::vector<UnstructuredPoint>>;
  using Column = std::vector<UnstructuredPoint>;

  /// timestep obtained from framework.
  double m_timestep;

  /// Time mapper.
  TimeMapper m_timeMapper;
};
}
}

#endif
