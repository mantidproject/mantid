// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**
   Required properties:
   <UL>
   <LI>OutputWorkspace - The name of workspace to create</LI>
   <LI>DataValue - The value to place in this workspace</LI>
   </UL>

   Optional properties:
  <UL>
  <LI>ErrorValue - An error value</LI>
  </UL>
   @author Martyn Gigg, Tessella Support Services plc
   @date 28/01/2009
*/
class MANTID_ALGORITHMS_DLL CreateSingleValuedWorkspace : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "CreateSingleValuedWorkspace"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Creates a 2D workspace containing a single X, Y & E value."; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"CreateWorkspace"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces"; }

protected:
  Parallel::ExecutionMode
  getParallelExecutionMode(const std::map<std::string, Parallel::StorageMode> &storageModes) const override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};
} // namespace Algorithms
} // namespace Mantid
