#ifndef VATES_API_SAVE_MD_WORKSPACE_TO_VTK_H_
#define VATES_API_SAVE_MD_WORKSPACE_TO_VTK_H_
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include <map>

namespace Mantid {
namespace VATES {

/** SaveMDWorkspaceToVTK : Defines an algorithm to save MDWorkspaces
to a VTK compatible format in order to load them into ParaView.
MDHistoWorkspaces are stored in the vts and MDEvent Workspaces
are stored in the vtu file format. Note that currently only 3D workspaces
are supported.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class SaveMDWorkspaceToVTKImpl;

class DLLExport SaveMDWorkspaceToVTK : public Mantid::API::Algorithm {
public:
  SaveMDWorkspaceToVTK();
  ~SaveMDWorkspaceToVTK() override;
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  std::unique_ptr<SaveMDWorkspaceToVTKImpl> saver;
};
} // namespace VATES
} // namespace Mantid
#endif
