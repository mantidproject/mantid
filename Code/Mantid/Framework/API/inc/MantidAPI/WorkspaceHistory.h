#ifndef MANTID_API_WORKSPACEHISTORY_H_
#define MANTID_API_WORKSPACEHISTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidKernel/EnvironmentHistory.h"
#include <ctime>
#include <vector>

namespace Mantid
{
namespace API
{
  class IAlgorithm;
  /** This class stores information about the Workspace History used by algorithms
    on a workspace and the environment history.

    @author Dickon Champion, ISIS, RAL
    @date 21/01/2008

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
class DLLExport WorkspaceHistory
{
public:
  WorkspaceHistory();
  virtual ~WorkspaceHistory();
  WorkspaceHistory(const WorkspaceHistory&);
  /// Retrieve the algorithm history list
  const std::vector<AlgorithmHistory>& getAlgorithmHistories() const;
  /// Retrieve the environment history
  const Kernel::EnvironmentHistory& getEnvironmentHistory() const;
  /// Append an workspace history to this one
  void addHistory(const WorkspaceHistory& otherHistory);
  /// Append an algorithm history to this one
  void addHistory(const AlgorithmHistory& algHistory);
  /// How many entries are there
  size_t size() const;
  /// Retrieve an algorithm history by index
  const AlgorithmHistory & getAlgorithmHistory(const size_t index) const;
  /// Create an algorithm from a history record at a given index
  boost::shared_ptr<IAlgorithm> getAlgorithm(const size_t index) const;
  /// Convenience function for retrieving the last algorithm
  boost::shared_ptr<IAlgorithm> lastAlgorithm() const;

  /// Pretty print the entire history
  void printSelf(std::ostream&, const int indent  = 0) const;

private:
  /// Private, unimplemented copy assignment operator
  WorkspaceHistory& operator=(const WorkspaceHistory& );

  /// The environment of the workspace
  const Kernel::EnvironmentHistory m_environment;
  /// The algorithms which have been called on the workspace
  std::vector<AlgorithmHistory> m_algorithms;
  
};

DLLExport std::ostream& operator<<(std::ostream&, const WorkspaceHistory&);

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_WORKSPACEHISTORY_H_*/
