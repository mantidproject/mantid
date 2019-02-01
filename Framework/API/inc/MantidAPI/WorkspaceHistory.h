// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
*/
class MANTID_API_DLL WorkspaceHistory {
public:
  /// Default constructor
  WorkspaceHistory();
  /// Destructor
  virtual ~WorkspaceHistory();
  /// Copy constructor
  WorkspaceHistory(const WorkspaceHistory &);
  /// Deleted copy assignment operator
  WorkspaceHistory &operator=(const WorkspaceHistory &) = delete;
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
  /// Add an operator== that compares algorithm historys
  bool operator==(const WorkspaceHistory &otherHistory) const;
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
