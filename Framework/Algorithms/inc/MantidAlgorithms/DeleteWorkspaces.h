// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**
  A simple algorithm to remove multiple workspaces from the ADS.

  @author Nick Draper, Tessella plc
  @date 2017-02-17
*/
class MANTID_ALGORITHMS_DLL DeleteWorkspaces : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "DeleteWorkspaces"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Removes a list of workspaces from memory."; }

  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"DeleteWorkspace"}; }

private:
  /// Overridden init
  void init() override;
  /// Overridden exec
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
