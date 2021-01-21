// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** Algorithm to clone a MDEventWorkspace to a new one.
 * Can also handle file-backed MDEventWorkspace's

  @author Janik Zikovsky
  @date 2011-08-15
*/
class DLLExport CloneMDWorkspace : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "CloneMDWorkspace"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Clones (copies) an existing MDEventWorkspace or MDHistoWorkspace "
           "into a new one.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override { return R"(MDAlgorithms\Utility\Workspaces;MDAlgorithms\Creation)"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  template <typename MDE, size_t nd> void doClone(const typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);
};

} // namespace MDAlgorithms
} // namespace Mantid
