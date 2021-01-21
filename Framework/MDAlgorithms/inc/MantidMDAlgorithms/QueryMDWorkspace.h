// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

/** QueryMDWorkspace : Query an MDWorkspace in order to extract overview
  information as a table workspace. Signal and Error squared values as well as
  extent information are exported.

  @date 2011-11-22
*/
class DLLExport QueryMDWorkspace : public Mantid::API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "QueryMDWorkspace"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Query the IMDWorkspace in order to extract summary information.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  /// Algorithm's category for identification
  const std::string category() const override { return "MDAlgorithms\\Utility\\Workspace"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  template <typename MDE, size_t nd> void getBoxData(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);
};

} // namespace MDAlgorithms
} // namespace Mantid
