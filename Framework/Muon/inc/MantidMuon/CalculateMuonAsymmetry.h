#ifndef MANTID_ALGORITHM_CALCULATEMUONASYMMETRY_H_
#define MANTID_ALGORITHM_CALCULATEMUONASYMMETRY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace Algorithms {
/**Takes a muon workspace as input and calculates the
asymmetry, using a function fo the form

N_0* [ 1 + sum_j f(t,{lambda}_j) ]

where the sum is over a set of functions and {lambda}_j is the jth
set of input parameters. The above equation is fitted to the normalised
counts to get the asymmetry.

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
<LI> Spectra - The spectra to be adjusted (by default all spectra are done)</LI>
<LI> StartX - The minimum time to include the analysis </LI>
<LI> EndX - The maximum time to include in the analysis </LI>
<LI> FittingFucntion - The composite function to be used in the fitting (sum_j
f(t,{lambda}_j) ) </LI>
<LI> InputDataType - If the input data is counts or asymmetry </LI>
<LI> Minimizer - The minimizer method to use for the calculation </LI>
<LI> MaxIterations - The maximum number of iterations in the calculation </LI>
</UL>


@author
@date 03/03/2017

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
class DLLExport CalculateMuonAsymmetry : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CalculateMuonAsymmetry"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm calculates the asymmetry for a transverse field.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"Fit", "ConvertFitFunctionForMuonTFAsymmetry",
            "EstimateMuonAsymmetryFromCounts"};
  }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  // calculate Muon normalisation constant
  std::vector<double> getNormConstants(std::vector<std::string> wsNames);
  std::map<std::string, std::string> validateInputs() override;
  double getNormValue(API::CompositeFunction_sptr &func);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_CALCULATEMUONASYMMETRY_H_*/
