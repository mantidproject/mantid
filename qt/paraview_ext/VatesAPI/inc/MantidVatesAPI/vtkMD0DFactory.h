#ifndef MANTID_VATES_VTK_MD_0D_FACTORY_H_
#define MANTID_VATES_VTK_MD_0D_FACTORY_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "vtkUnstructuredGrid.h"

namespace Mantid {
namespace VATES {

/** 0D Factory. This type is responsible for rendering IMDWorkspaces with 0D.

 Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport vtkMD0DFactory : public vtkDataSetFactory {
public:
  /// Constructor
  vtkMD0DFactory();

  /// Destructor
  ~vtkMD0DFactory() override;

  /// Factory Method.
  vtkSmartPointer<vtkDataSet>
  create(ProgressAction &progressUpdating) const override;

  void initialize(const Mantid::API::Workspace_sptr &workspace) override;

  std::string getFactoryTypeName() const override { return "vtkMD0DFactory"; }

protected:
  void validate() const override;
};
} // namespace VATES
} // namespace Mantid
#endif
