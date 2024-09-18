// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {

namespace Algorithms {
/** Takes a workspace as input and rebins the data according to the input rebin
   parameters.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the workspace to take as input. Must
   contain histogram data. </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result. </LI>
    <LI> RebinParameters - The new bin boundaries in the form
   X1,deltaX1,X2,deltaX2,X3,... </LI>
    </UL>

    The algorithms used in the VectorHelper::rebin() and
   VectorHelper::createAxisFromRebinParams()
    are based on the algorithms used in OPENGENIE /src/transform_utils.cxx
   (Freddie Akeroyd, ISIS).
    When calculating the bin boundaries, if the last bin ends up with a width
   being less than 25%
    of the penultimate one, then the two are combined.

    @author Dickon Champion, STFC
    @date 25/02/2008
 */
class MANTID_ALGORITHMS_DLL Rebin : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Rebin"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Rebins data with new X bin boundaries. For EventWorkspaces, you "
           "can very quickly rebin in-place by keeping the same output name "
           "and PreserveEvents=true.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Rebin"; }
  /// Algorithm's aliases
  const std::string alias() const override { return "rebin"; }
  /// Algorithm's seeAlso
  const std::vector<std::string> seeAlso() const override {
    return {"RebinToWorkspace", "Rebin2D", "Rebunch", "Regroup", "RebinByPulseTimes", "RebinByTimeAtSample"};
  }
  std::map<std::string, std::string> validateInputs() override;

  static std::vector<double> rebinParamsFromInput(const std::vector<double> &inParams,
                                                  const API::MatrixWorkspace &inputWS, Kernel::Logger &logger,
                                                  const std::string &binModeName = "Default");

protected:
  const std::string workspaceMethodName() const override { return "rebin"; }
  const std::string workspaceMethodOnTypes() const override { return "MatrixWorkspace"; }
  const std::string workspaceMethodInputProperty() const override { return "InputWorkspace"; }

  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  void propagateMasks(const API::MatrixWorkspace_const_sptr &inputWS, const API::MatrixWorkspace_sptr &outputWS,
                      const int hist, const bool IgnoreBinErrors = false);
};

} // namespace Algorithms
} // namespace Mantid
