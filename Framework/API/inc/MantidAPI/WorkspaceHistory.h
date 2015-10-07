#ifndef MANTID_API_WORKSPACEHISTORY_H_
#define MANTID_API_WORKSPACEHISTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidKernel/EnvironmentHistory.h"
#include <ctime>
#include <set>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
namespace NeXus {
class File;
}

namespace Mantid {
namespace API {
class IAlgorithm;
class HistoryView;

/** This class stores information about the Workspace History used by algorithms
  on a workspace and the environment history.

  @author Dickon Champion, ISIS, RAL
  @date 21/01/2008

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
class MANTID_API_DLL WorkspaceHistory {
public:
  /// Default constructor
  WorkspaceHistory();
  /// Destructor
  virtual ~WorkspaceHistory();
  /// Copy constructor
  WorkspaceHistory(const WorkspaceHistory &);
  /// Retrieve the algorithm history list
  const AlgorithmHistories &getAlgorithmHistories() const;
  /// Retrieve the environment history
  const Kernel::EnvironmentHistory &getEnvironmentHistory() const;
  /// Append an workspace history to this one
  void addHistory(const WorkspaceHistory &otherHistory);
  /// Append an algorithm history to this one
  void addHistory(AlgorithmHistory_sptr algHistory);
  /// How many entries are there
  size_t size() const;
  /// Is the history empty
  bool empty() const;
  /// remove all algorithm history objects from the workspace history
  void clearHistory();
  /// Retrieve an algorithm history by index
  AlgorithmHistory_const_sptr getAlgorithmHistory(const size_t index) const;
  /// Add operator[] access
  AlgorithmHistory_const_sptr operator[](const size_t index) const;
  /// Create an algorithm from a history record at a given index
  boost::shared_ptr<IAlgorithm> getAlgorithm(const size_t index) const;
  /// Convenience function for retrieving the last algorithm
  boost::shared_ptr<IAlgorithm> lastAlgorithm() const;
  /// Create a flat view of the workspaces algorithm history
  boost::shared_ptr<HistoryView> createView() const;

  /// Pretty print the entire history
  void printSelf(std::ostream &, const int indent = 0) const;

  /// Save the workspace history to a nexus file
  void saveNexus(::NeXus::File *file) const;
  /// Load the workspace history from a nexus file
  void loadNexus(::NeXus::File *file);

private:
  /// Private, unimplemented copy assignment operator
  WorkspaceHistory &operator=(const WorkspaceHistory &);
  /// Recursive function to load the algorithm history tree from file
  void loadNestedHistory(
      ::NeXus::File *file,
      AlgorithmHistory_sptr parent = boost::shared_ptr<AlgorithmHistory>());
  /// Parse an algorithm history string loaded from file
  AlgorithmHistory_sptr parseAlgorithmHistory(const std::string &rawData);
  /// Find the history entries at this level in the file.
  std::set<int> findHistoryEntries(::NeXus::File *file);
  /// The environment of the workspace
  const Kernel::EnvironmentHistory m_environment;
  /// The algorithms which have been called on the workspace
  Mantid::API::AlgorithmHistories m_algorithms;
};

MANTID_API_DLL std::ostream &operator<<(std::ostream &,
                                        const WorkspaceHistory &);

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_WORKSPACEHISTORY_H_*/
