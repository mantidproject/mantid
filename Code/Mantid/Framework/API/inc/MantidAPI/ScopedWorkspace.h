#ifndef MANTID_API_SCOPEDWORKSPACE_H_
#define MANTID_API_SCOPEDWORKSPACE_H_

#include <string>

#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/System.h"
#include "MantidAPI/Workspace.h"

namespace Mantid
{
namespace API
{
  /** ScopedWorkspace : scoped workspace ADS entry.

    This class is provided for situations when you need a workspace to be in the ADS to run an 
    algorithm, but you don't really need to keep it there after the algorithm has finished. In
    these circumstances you can create ScopedWorkspace, set it's name as a workspace property
    for the algorithm and retrieve it when algorithm has finished. The workspace will be
    removed from the ADS when the object goes out of scope, or exception is thrown.

    Primarily, it was created to overcome some limitations of WorkspaceProperties, but it can be
    useful in other places, e.g. tests.
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport ScopedWorkspace 
  {
  public:
    /// Empty constructor
    ScopedWorkspace();

    /// Workspace constructor
    ScopedWorkspace(Workspace_sptr ws);

    /// Destructor
    virtual ~ScopedWorkspace();

    /// Returns ADS name of the workspace
    std::string name() const { return m_name; }

    /// Retrieve workspace from the ADS
    Workspace_sptr retrieve() const;

    /// Removes the workspace entry from the ADS
    void remove();

    /// Operator for conversion to boolean
    operator bool() const;

    /// Make ADS entry to point to the given workspace
    void set(Workspace_sptr newWS);

  private:
    DISABLE_COPY_AND_ASSIGN(ScopedWorkspace);

    /// ADS name of the workspace
    const std::string m_name;
 
    /// Generates a tricky name which is unique within ADS 
    static std::string generateUniqueName();

    /// Generates a random alpha-numeric string
    static std::string randomString(size_t len);
  
    /// Length of workspace names generated
    static const size_t NAME_LENGTH;
  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_SCOPEDWORKSPACE_H_ */
