// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
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
/** Creates a single spectrum matrix workspace from some columns of a table
 workspace.

 Required Properties:
 <UL>
 <LI> InputWorkspace  - The name of the input table workspace. </LI>
 <LI> ColumnX - The column name for the X vector.
 <LI> ColumnY - The column name for the Y vector.
 <LI> ColumnE (optional) - The column name for the E vector. If not set the
 errors will be filled with 1s.
 <LI> OutputWorkspace - The name of the output matrix workspace. </LI>
 </UL>

 @author Roman Tolchenov, Tessella plc
 @date 25/01/2012
 */
class MANTID_ALGORITHMS_DLL ConvertTableToMatrixWorkspace : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ConvertTableToMatrixWorkspace"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Creates a single spectrum matrix workspace from some columns of a "
           "table workspace.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"ConvertMDHistoToMatrixWorkspace"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
