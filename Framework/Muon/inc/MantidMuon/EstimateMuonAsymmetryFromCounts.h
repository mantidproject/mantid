// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHM_ESTIMATEMUONASYMMETRYFROMCOUNTS_H_
#define MANTID_ALGORITHM_ESTIMATEMUONASYMMETRYFROMCOUNTS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidMuon/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**Takes a muon workspace as input and estimates the asymmetry, using a simple
method.

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
<LI> Spectra - The spectra to be adjusted (by default all spectra are done)</LI>
<LI> StartX - The minimum time to include in the analysis </LI>
<LI> EndX - The maximum time to include in the analysis </LI>
</UL>


@author
@date 03/03/2017
*/
class MANTID_MUON_DLL EstimateMuonAsymmetryFromCounts : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override {
    return "EstimateMuonAsymmetryFromCounts";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm gives an estimate "
           "for the asymmetry.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon"; }
  /// Algorithm's seeAlso
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertFitFunctionForMuonTFAsymmetry", "CalculateMuonAsymmetry"};
  };

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ESTIMATEMUONASYMMETRYFROMCOUNTS_H_*/
