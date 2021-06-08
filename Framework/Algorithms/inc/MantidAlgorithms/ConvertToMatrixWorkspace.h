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
/** Creates a copy of the matrix workspace representation of the input
 workspace. At the moment, this
 is only available for MatrixWorkspaces and EventWorkspaces.

 Required Properties:
 <UL>
 <LI> InputWorkspace  - The name of the input workspace. </LI>
 <LI> OutputWorkspace - The name of the output workspace. </LI>
 </UL>

 @author Stuart Campbell, ORNL
 @date 10/12/2010
 */
class MANTID_ALGORITHMS_DLL ConvertToMatrixWorkspace : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ConvertToMatrixWorkspace"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts an EventWorkspace into a Workspace2D, using the input "
           "workspace's current X bin values.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"ConvertToEventWorkspace", "Rebin"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Events"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
