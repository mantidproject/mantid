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

namespace Mantid {

namespace API {
class IMDHistoWorkspace;
}

namespace MDAlgorithms {
/** Creates a single spectrum Workspace2D with X,Y, and E copied from an first
 non-integrated dimension of a IMDHistoWorkspace.

 Required Properties:
 <UL>
 <LI> InputWorkspace  - The name of the input IMDHistoWorkspace.. </LI>
 <LI> OutputWorkspace - The name of the output matrix workspace. </LI>
 <LI> Normalization -   Signal normalization method: NoNormalization,
 VolumeNormalization, or NumEventsNormalization</LI>
 </UL>

 @author Roman Tolchenov, Tessella plc
 @date 17/04/2012
 */
class DLLExport ConvertMDHistoToMatrixWorkspace : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ConvertMDHistoToMatrixWorkspace"; };

  /// Summary of algorithms purpose
  const std::string summary() const override { return "Converts if it can a IMDHistoWorkspace to a Workspace2D."; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertToMD", "CreateMDHistoWorkspace", "ConvertTableToMatrixWorkspace", "MDHistoToWorkspace2D"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces;MDAlgorithms\\Transforms"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Make MatrixWorkspace with 1 spectrum
  void make1DWorkspace();
  /// Make 2D MatrixWorkspace
  void make2DWorkspace();
  /// Calculate the stride for a dimension
  size_t calcStride(const API::IMDHistoWorkspace &workspace, size_t dim) const;
};

} // namespace MDAlgorithms
} // namespace Mantid
