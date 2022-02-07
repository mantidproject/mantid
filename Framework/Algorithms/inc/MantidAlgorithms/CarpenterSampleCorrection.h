// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/DllConfig.h"
#include <vector>

namespace Mantid {
namespace Algorithms {
/** Multiple scattering absorption correction, originally used to
    correct vanadium spectrum at IPNS.  Algorithm originally worked
    out by Jack Carpenter and Asfia Huq and implmented in Java by
    Alok Chatterjee.  Translated to C++ by Dennis Mikkelson.
 */
class MANTID_ALGORITHMS_DLL CarpenterSampleCorrection : public API::DistributedDataProcessorAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override;

  /// Algorithm's version for identification overriding a virtual method
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CalculateCarpenterSampleCorrection",
            "CylinderAbsorption",
            "MonteCarloAbsorption",
            "MayersSampleCorrection",
            "PearlMCAbsorption",
            "VesuvioCalculateMS"};
  }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Applies both absorption and multiple scattering corrections, "
           "originally used to correct vanadium spectrum at IPNS.";
  }

  // Algorithm's alias for identification overriding a virtual method
  const std::string alias() const override { return "MultipleScatteringCylinderAbsorption"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  API::WorkspaceGroup_sptr calculateCorrection(API::MatrixWorkspace_sptr &inputWksp, double radius, double coeff1,
                                               double coeff2, double coeff3, bool doAbs, bool doMS);

  API::MatrixWorkspace_sptr multiply(const API::MatrixWorkspace_sptr &lhsWS, const API::MatrixWorkspace_sptr &rhsWS);
  API::MatrixWorkspace_sptr minus(const API::MatrixWorkspace_sptr &lhsWS, const API::MatrixWorkspace_sptr &rhsWS);
};

} // namespace Algorithms
} // namespace Mantid
