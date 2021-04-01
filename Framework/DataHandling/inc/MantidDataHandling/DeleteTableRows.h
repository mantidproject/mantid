// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {

/**
Deletes a row from a TableWorkspace.

@author Roman Tolchenov, Tessella plc
@date 12/05/2011
*/
class DLLExport DeleteTableRows : public API::Algorithm {
public:
  /// Default constructor
  DeleteTableRows() {}
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "DeleteTableRows"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Deletes rows from a TableWorkspace."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"CreateEmptyTableWorkspace"}; }
  /// Category
  const std::string category() const override { return "Utility\\Workspaces"; }

private:
  /// Initialize the static base properties
  void init() override;
  /// Execute
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
