// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2001 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**
  A simple algorithm to remove a workspace from the ADS. Basically so that it is
  logged
  and can be recreated from a history record

  @author Martyn Gigg, Tessella plc
  @date 2011-01-24
*/
class MANTID_ALGORITHMS_DLL DeleteWorkspace : public API::DistributedAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "DeleteWorkspace"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Removes a workspace from memory."; }

  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"DeleteWorkspaces"}; }

private:
  /// Overridden init
  void init() override;
  /// Overridden exec
  void exec() override;

  const std::string workspaceMethodName() const override { return "delete"; }
  const std::string workspaceMethodInputProperty() const override { return "Workspace"; }
};

} // namespace Algorithms
} // namespace Mantid
