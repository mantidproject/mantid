#ifndef MANTID_API_WORKSPACE_H_
#define MANTID_API_WORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DataItem.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/Exception.h"

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
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class AnalysisDataServiceImpl;

/** Base Workspace Abstract Class.

    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class MANTID_API_DLL Workspace : public Kernel::DataItem
{
public:

    Workspace();
    Workspace(const Workspace & other);
    virtual ~Workspace();

    // DataItem interface
    /// Name
    virtual const std::string name() const { return this->getName(); }
    /** Marks the workspace as safe for multiple threads to edit data simutaneously.
     * Workspace creation is always considered to be a single threaded operation.
     * @return true if the workspace is suitable for multithreaded operations, otherwise false.
     */
    virtual bool threadSafe() const { return true; }

    void virtual setTitle(const std::string&);
    void setComment(const std::string&);
    virtual const std::string getTitle() const;
    const std::string& getComment() const;
    const std::string& getName() const;
    bool isDirty(const int n=1) const;
    /// Get the footprint in memory in bytes.
    virtual size_t getMemorySize() const = 0;
    /// Returns the memory footprint in sensible units
    std::string getMemorySizeAsStr() const;

    /// Returns a reference to the WorkspaceHistory
    WorkspaceHistory& history() { return m_history; }
    /// Returns a reference to the WorkspaceHistory const
    const WorkspaceHistory& getHistory() const { return m_history; }

private:
    void setName(const std::string&);
    /// The title of the workspace
    std::string m_title;
    /// A user-provided comment that is attached to the workspace
    std::string m_comment;
    /// The name associated with the object within the ADS (This is required for workspace algebra
    std::string m_name;
    /// The history of the workspace, algorithm and environment
    WorkspaceHistory m_history;

    friend class AnalysisDataServiceImpl;

};


///shared pointer to the workspace base class
typedef boost::shared_ptr<Workspace> Workspace_sptr;
///shared pointer to the workspace base class (const version)
typedef boost::shared_ptr<const Workspace> Workspace_const_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_WORKSPACE_H_*/
