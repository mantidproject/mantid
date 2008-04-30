#ifndef IWORKSPACEPROPERTY_H_
#define IWORKSPACEPROPERTY_H_

#include "MantidAPI/Workspace.h"

namespace Mantid
{
namespace API
{
/** @class IWorkspaceProperty IWorkspaceProperty.h Kernel/IWorkspaceProperty.h

    An interface that is implemented by WorkspaceProperty.
    Used for non templated workspace operations.

    @author Nick Draper, Tessella Support Services plc
    @date 21-01-2008
    
    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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
    
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class IWorkspaceProperty
{
public:
  /// Store a workspace into the AnalysisDataService
  virtual Workspace_sptr getWorkspace() = 0;
  /// Returns the direction of the workspace property
  virtual const unsigned int direction()const =0;
  /// Virtual destructor
  virtual ~IWorkspaceProperty() {}
};

} // namespace Kernel
} // namespace Mantid

#endif /*IWORKSPACEPROPERTY_H_*/
