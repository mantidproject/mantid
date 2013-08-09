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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class MANTID_API_DLL Workspace : public Kernel::DataItem
{
public:

    /**
     * Holds information about a workspace in a vector of strings.
     * InfoNodes can contian other nodes and form a tree.
     * The purpose is to be returned by the ADS for displaying in the GUI.
     */
    class MANTID_API_DLL InfoNode
    {
    public:
        enum IconType {Default = 0, Matrix, Group, MD, Table};
        /// Constructor
        InfoNode(const Workspace& workspace);
        /// Constructor
        InfoNode(const AnalysisDataServiceImpl*);
        /// Destructor
        ~InfoNode();
        /// Add a new line
        void addLine(const std::string& line);
        /// Add a new InfoNode object.
        void addNode(InfoNode* node);
        /// Add experiment info if workspace inherits from ExperimentInfo.
        void addExperimentInfo(const Workspace& workspace);
        /// Get workspace name
        std::string workspaceName() const {return m_workspaceName;}
        /// Get memory size
        size_t getMemorySize() const {return m_memorySize;}
        /// Get the info lines.
        const std::vector<std::string>& lines() const {return m_info;}
        /// Get the child nodes.
        const std::vector<InfoNode*>& nodes() const {return m_nodes;}
        /// Get icon type
        IconType getIconType() const {return m_icon;}
    private:
        /// Information about a single workspace. Each string shouldn't be too long.
        std::vector<std::string> m_info;
        /// Child nodes (eg a WorkspaceGroup will have a child node for each item).
        std::vector<InfoNode*> m_nodes;
        /// Icon type to use for this workspace
        IconType m_icon;
        /// Workspace name
        std::string m_workspaceName;
        /// Memory size taken by the workspace
        size_t m_memorySize;
    };

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
    /// Add info about this workspace to a parent InfoNode.
    void addInfoNodeTo(InfoNode& parentNode) const;

protected:
    /// Create and return a new InfoNode describing this workspace.
    virtual InfoNode* createInfoNode() const;

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
