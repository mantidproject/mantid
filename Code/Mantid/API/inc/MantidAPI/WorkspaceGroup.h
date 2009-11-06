#ifndef MANTID_API_WORKSPACEGROUP_H
#define MANTID_API_WORKSPACEGROUP_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Workspace.h"

namespace Mantid
{

//----------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}

namespace API
{
/** Class to hold a set of workspaces.
    The workspace group can be an entry in the AnalysisDataService. 
    Its constituent workspaces should also have individual ADS entries.
    Workspace groups can be used in algorithms in the same way as single workspaces.

    @author Sofia Antony, ISIS, RAL
    @date 12/06/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratory

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
class DLLExport WorkspaceGroup : public Workspace
{
public:
  WorkspaceGroup();
  ~WorkspaceGroup();
  
  virtual const std::string id() const { return "WorkspaceGroup"; }
  virtual long int getMemorySize() const { return 0; }
  void add(const std::string& wsName);
  /// Returns the workspaces that make up this group
  const std::vector<std::string>& getNames() const { return m_wsNames; }
  /// Return the number of entries within the group
  int getNumberOfEntries() const { return static_cast<int>(m_wsNames.size()); }
  void print() const;
  void remove(const std::string& name);
  void removeAll();
  void deepRemoveAll();
 
private:
  /// Private, unimplemented copy constructor
  WorkspaceGroup(const WorkspaceGroup& ref);
  /// Private, unimplemented copy assignment operator
  const WorkspaceGroup& operator=(const WorkspaceGroup&);

  std::vector<std::string> m_wsNames; ///< The list of workspace names in the group
  static Kernel::Logger& g_log;       ///< Static reference to the logger
};

/// Shared pointer to a workspace group class
typedef boost::shared_ptr<WorkspaceGroup> WorkspaceGroup_sptr;
/// Shared pointer to a workspace group class (const version)
typedef boost::shared_ptr<const WorkspaceGroup> WorkspaceGroup_const_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_WORKSPACEGROUP_H*/
