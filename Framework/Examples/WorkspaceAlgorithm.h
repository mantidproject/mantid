// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** An example algorithm iterating over a workspace.

    @author Roman Tolchenov, ISIS, RAL
    @date 02/05/2008
 */
class WorkspaceAlgorithm final : public API::Algorithm {
public:
  /// no arg constructor
  WorkspaceAlgorithm() : API::Algorithm() {}
  /// virtual destructor
  ~WorkspaceAlgorithm() override = default;
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "WorkspaceAlgorithm"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Examples"; }
  // Algorithm's summary. Overriding a virtual method.
  const std::string summary() const override { return "Example summary text."; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
