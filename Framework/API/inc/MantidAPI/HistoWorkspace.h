#ifndef MANTID_API_HISTOWORKSPACE_H_
#define MANTID_API_HISTOWORKSPACE_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {

/** HistoWorkspace is an abstract base class for MatrixWorkspace types that are
  NOT event workspaces. This type has to exist as a helper for workspace
  creation: Many algorithms create a new MatrixWorkspace from a parent workspace
  without keeping the events, but keeping any potential sub type of
  MatrixWorkspace. HistoWorkspace provides a common base type for all non-event
  MatrixWorkspaces. See DataObjects/WorkspaceCreation.h for mor details.

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
class MANTID_API_DLL HistoWorkspace : public MatrixWorkspace {
public:
  HistoWorkspace(
      const Parallel::StorageMode storageMode = Parallel::StorageMode::Cloned)
      : MatrixWorkspace(storageMode) {}

  /// Returns a clone of the workspace
  std::unique_ptr<HistoWorkspace> clone() const {
    return std::unique_ptr<HistoWorkspace>(doClone());
  }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<HistoWorkspace> cloneEmpty() const {
    return std::unique_ptr<HistoWorkspace>(doCloneEmpty());
  }

private:
  HistoWorkspace *doClone() const override = 0;
  HistoWorkspace *doCloneEmpty() const override = 0;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_HISTOWORKSPACE_H_ */
