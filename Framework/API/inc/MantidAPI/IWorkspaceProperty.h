#ifndef MANTID_API_IWORKSPACEPROPERTY_H_
#define MANTID_API_IWORKSPACEPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Workspace_fwd.h"

namespace Mantid {
namespace API {
/** An interface that is implemented by WorkspaceProperty.
    Used for non templated workspace operations.

    @author Nick Draper, Tessella Support Services plc
    @author Russell Taylor, Tessella Support Services plc
    @date 11/12/2007

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class IWorkspaceProperty {
public:
  /// Store a workspace into the AnalysisDataService
  virtual bool store() = 0;
  /// Clear the stored pointer
  virtual void clear() = 0;
  /// Get a pointer to the workspace
  virtual Workspace_sptr getWorkspace() const = 0;
  /// Is the input workspace property optional (can be blank)?
  virtual bool isOptional() const = 0;
  /// Will the workspace be locked when starting an algorithm?
  virtual bool isLocking() const = 0;
  /// Returns the direction of the workspace property
  //-virtual const unsigned int direction() const = 0;
  /// Virtual destructor
  virtual ~IWorkspaceProperty() = default;
  /// Sets a flag indicating whether this is the master rank in MPI builds.
  virtual void setIsMasterRank(bool isMasterRank) = 0;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IWORKSPACEPROPERTY_H_*/
