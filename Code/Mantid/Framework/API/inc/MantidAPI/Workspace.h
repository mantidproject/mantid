#ifndef MANTID_API_WORKSPACE_H_
#define MANTID_API_WORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{

//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}

namespace API
{
/** Base Workspace Abstract Class.

    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport Workspace
{
public:
    /// Return the workspace typeID (normally the type name as a string)
    virtual const std::string id() const = 0;

    Workspace();
    virtual ~Workspace() {}
    void setTitle(const std::string&);
    void setComment(const std::string&);
    void setName(const std::string&);
    const std::string& getTitle() const;
    const std::string& getComment() const;
    const std::string& getName() const;
    /// Get the footprint in memory in bytes.
    virtual size_t getMemorySize() const = 0;

    /** Marks the workspace as safe for multiple threads to edit data simutaneously.
     * Workspace creation is always considered to be a single threaded operation.
     * @return true if the workspace is suitable for multithreaded operations, otherwise false.
     */
    virtual bool threadSafe() const { return true; }

    /// Returns a reference to the WorkspaceHistory
    WorkspaceHistory& history() { return m_history; }
    /// Returns a reference to the WorkspaceHistory const
    const WorkspaceHistory& getHistory() const { return m_history; }


private:
    /// The title of the workspace
    std::string m_title;
    /// A user-provided comment that is attached to the workspace
    std::string m_comment;
    /// The name associated with the object within the ADS (This is required for workspace algebra
    std::string m_name;
    /// The history of the workspace, algorithm and environment
    WorkspaceHistory m_history;

};


///shared pointer to the workspace base class
typedef boost::shared_ptr<Workspace> Workspace_sptr;
///shared pointer to the workspace base class (const version)
typedef boost::shared_ptr<const Workspace> Workspace_const_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_WORKSPACE_H_*/
