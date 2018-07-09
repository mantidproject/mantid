#ifndef MANTID_ALGORITHMS_WORKFLOWALGORITHMRUNNER_H_
#define MANTID_ALGORITHMS_WORKFLOWALGORITHMRUNNER_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** Controls the data flow and the order of algorithm execution.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak
  Ridge National Laboratory & European Spallation Source

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
class MANTID_ALGORITHMS_DLL WorkflowAlgorithmRunner : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "WorkflowAlgorithmRunner"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Manages dependency resolution, input/output mapping and running of "
           "algorithms.";
  }

  /// Algorithm's version
  int version() const override { return 1; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Configures a row in `setupTable`.
  template <typename QUEUE, typename MAP>
  void
  configureRow(API::ITableWorkspace_sptr setupTable,
               API::ITableWorkspace_sptr propertyTable, const size_t currentRow,
               QUEUE &queue, const MAP &ioMap,
               std::shared_ptr<std::unordered_set<size_t>> rowsBeingQueued =
                   nullptr) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_WORKFLOWALGORITHMRUNNER_H_ */
