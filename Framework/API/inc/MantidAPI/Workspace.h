#ifndef MANTID_API_WORKSPACE_H_
#define MANTID_API_WORKSPACE_H_

#include "MantidAPI/Workspace_fwd.h"
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DataItem.h"
#include "MantidKernel/Exception.h"

namespace Mantid {

namespace Kernel {
class Logger;
}

namespace API {
class AnalysisDataServiceImpl;
class WorkspaceHistory;

/** Base Workspace Abstract Class.

    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007

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
class MANTID_API_DLL Workspace : public Kernel::DataItem {
public:
  Workspace();
  ~Workspace();

  /** Returns a clone (copy) of the workspace with covariant return type in all
   * derived classes.
   *
   * Note that this public function is *not* virtual. This has two reasons:
   * - We want to enforce covariant return types. If this public clone() method
   *   was virtual some derived class might fail to reimplement it. Since there
   *   are several levels of inheritance in the Workspace inheritance tree we
   *   cannot just use a pure virtual method in the base class. Thus, we use
   *   this non-virtual interface method which calls the private and virtual
   *   doClone() method. Since it is private, failing to reimplement it will
   *   cause a compiler error as a reminder. Note that this mechanism does not
   *   always work if a derived class does not implement clone(): if doClone()
   *   in a parent is implemented then calling clone on a base class will return
   *   a clone of the parent defining doClone, not the actual instance. This is
   *   more a problem of the inheritance structure, i.e., whether or not all
   *   non-leaf classes are pure virtual and declare doClone()=0.
   * - Covariant return types are in conflict with smart pointers, but if
   *   clone() is not virtual this is a non-issue.
   */
  Workspace_uptr clone() const { return Workspace_uptr(doClone()); }
  Workspace &operator=(const Workspace &other) = delete;
  // DataItem interface
  /** Marks the workspace as safe for multiple threads to edit data
   * simutaneously.
   * Workspace creation is always considered to be a single threaded operation.
   * @return true if the workspace is suitable for multithreaded operations,
   * otherwise false.
   */
  bool threadSafe() const override { return true; }

  void virtual setTitle(const std::string &);
  void setComment(const std::string &);
  virtual const std::string getTitle() const;
  const std::string &getComment() const;
  const std::string &getName() const override;
  bool isDirty(const int n = 1) const;
  /// Get the footprint in memory in bytes.
  virtual size_t getMemorySize() const = 0;
  /// Returns the memory footprint in sensible units
  std::string getMemorySizeAsStr() const;

  /// Returns a reference to the WorkspaceHistory
  WorkspaceHistory &history() { return *m_history; }
  /// Returns a reference to the WorkspaceHistory const
  const WorkspaceHistory &getHistory() const { return *m_history; }

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  Workspace(const Workspace &);

private:
  void setName(const std::string &);
  /// The title of the workspace
  std::string m_title;
  /// A user-provided comment that is attached to the workspace
  std::string m_comment;
  /// The name associated with the object within the ADS (This is required for
  /// workspace algebra
  std::string m_name;
  /// The history of the workspace, algorithm and environment
  std::unique_ptr<WorkspaceHistory> m_history;

  /// Virtual clone method. Not implemented to force implementation in childs.
  virtual Workspace *doClone() const = 0;

  friend class AnalysisDataServiceImpl;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_WORKSPACE_H_*/
