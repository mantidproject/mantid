// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
  void addNormalizedFits(size_t numberOfFits, const std::vector<double>);
  void normalizeWorkspace(
      const API::MatrixWorkspace_sptr &normalizedWorkspace,
      const API::MatrixWorkspace_const_sptr &unnormalizedWorkspace,
      size_t workspaceIndex, double N0);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_CALCULATEMUONASYMMETRY_H_*/
