/**
Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#ifndef MANTID_CUSTOMINTERFACES_SLICEDREDUCTIONWORKSPACES_H_
#define MANTID_CUSTOMINTERFACES_SLICEDREDUCTIONWORKSPACES_H_
#include <string>
#include <vector>
#include "ReductionWorkspaces.h"
#include "Slicing.h"

namespace MantidQt {
namespace CustomInterfaces {

class SlicedReductionWorkspaces {
public:
  SlicedReductionWorkspaces(
      std::string const &inputWorkspace,
      std::vector<ReductionWorkspaces> const &sliceWorkspaces);

  std::string const &inputWorkspace() const;
  std::vector<ReductionWorkspaces> const &sliceWorkspaces() const;

private:
  std::string m_inputWorkspace;
  std::vector<ReductionWorkspaces> m_sliceWorkspaces;
};

MANTIDQT_ISISREFLECTOMETRY_DLL SlicedReductionWorkspaces
workspaceNamesForSliced(
    std::vector<std::string> const &summedRunNumbers,
    std::pair<std::string, std::string> const &transmissionRuns,
    Slicing const &slicing);

MANTIDQT_ISISREFLECTOMETRY_DLL std::string postprocessedWorkspaceNameForSliced(
    std::vector<std::vector<std::string> const *> const &summedRunNumbers,
    Slicing const &slicing);

MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator==(SlicedReductionWorkspaces const &lhs,
           SlicedReductionWorkspaces const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator!=(SlicedReductionWorkspaces const &lhs,
           SlicedReductionWorkspaces const &rhs);
}
}
#endif // MANTID_CUSTOMINTERFACES_SLICEDREDUCTIONWORKSPACES_H_
