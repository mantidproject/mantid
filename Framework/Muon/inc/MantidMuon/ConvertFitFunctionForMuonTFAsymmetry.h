#ifndef MANTID_ALGORITHM_CONVERTFITFUNCTIONFORMUONTFASYMMETRY_H_
#define MANTID_ALGORITHM_CONVERTFITFUNCTIONFORMUONTFASYMMETRY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction.h"

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

Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ConvertFitFunctionForMuonTFAsymmetry : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override {
    return "ConvertFitFunctionForMuonTFAsymmetry";
  }
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
  Mantid::API::IFunction_sptr
  getTFAsymmFitFunction(const Mantid::API::IFunction_sptr &original,
                        const std::vector<double> &norms);
  Mantid::API::IFunction_sptr
  extractFromTFAsymmFitFunction(const Mantid::API::IFunction_sptr &original);
  Mantid::API::IFunction_sptr
  extractUserFunction(const Mantid::API::IFunction_sptr &TFFunc);
  void setOutput(const Mantid::API::IFunction_sptr &function);
  std::vector<double> getNorms();
};

} // namespace Muon
} // namespace Mantid

#endif /*MANTID_CONVERTFITFUNCTIONFORMUONTFASYMMETRYS_H_*/
