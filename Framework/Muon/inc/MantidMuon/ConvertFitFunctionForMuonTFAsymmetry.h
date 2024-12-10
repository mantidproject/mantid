// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidMuon/DllConfig.h"

namespace Mantid {
namespace Muon {
/**Takes a fitting function and returns a modified fitting function.
To be used as part of TF asymmetry calculations

Required Properties:
<UL>
<LI> InputFunction - The function we want to change </LI>
<LI> NormalisationTable - The name of the table workspace that contains the
normalization constants
</LI>
<LI> WorkspaceList - The workspaces we are interested in </LI>
<LI> Mode - If constructing or destructing the TF asymmetry function </LI>
<LI> OutputFunction - The transformed function </LI>
</UL>


@author Anthony Lim
@date 22/05/2018
*/
class MANTID_MUON_DLL ConvertFitFunctionForMuonTFAsymmetry final : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "ConvertFitFunctionForMuonTFAsymmetry"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm converts adds/removes "
           "the normalisation to the fit function for calculating the TF "
           "asymmetry.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon"; }
  /// Algorithm's seeAlso
  const std::vector<std::string> seeAlso() const override {
    return {"Fit", "EstimateMuonAsymmetryFromCounts", "CalculateMuonAsymmetry"};
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  Mantid::API::IFunction_sptr getTFAsymmFitFunction(const Mantid::API::IFunction_sptr &original,
                                                    const std::vector<double> &norms);
  Mantid::API::IFunction_sptr extractFromTFAsymmFitFunction(const Mantid::API::IFunction_sptr &original);
  Mantid::API::IFunction_sptr extractUserFunction(const Mantid::API::IFunction_sptr &TFFunc);
  void setOutput(const Mantid::API::IFunction_sptr &function);
  std::vector<double> getNorms();
  void configureMultiDomainFunction(std::shared_ptr<Mantid::API::MultiDomainFunction> multiDomainFunction,
                                    const Mantid::API::IFunction_sptr &userFunc, const double normValue);
};

} // namespace Muon
} // namespace Mantid
