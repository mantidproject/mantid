// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** Controls the data flow and the order of algorithm execution.
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
  void configureRow(API::ITableWorkspace_sptr setupTable, API::ITableWorkspace_sptr propertyTable,
                    const size_t currentRow, QUEUE &queue, const MAP &ioMap,
                    std::shared_ptr<std::unordered_set<size_t>> rowsBeingQueued = nullptr) const;
};

} // namespace Algorithms
} // namespace Mantid
