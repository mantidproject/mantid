// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHM_MUONREMOVEEXPDECAY_H_
#define MANTID_ALGORITHM_MUONREMOVEEXPDECAY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidMuon/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**Takes a muon workspace as input and removes the exponential decay from a time
channel.
     This is done by multiplying the data by exp(t/tmuon).

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
<LI> Spectra - The spectra to be adjusted (by default all spectra are done)</LI>
</UL>


@author
@date 11/07/2008
*/
class MANTID_MUON_DLL MuonRemoveExpDecay : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RemoveExpDecay"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm removes the exponential decay from a muon "
           "workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Fit"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  // Remove exponential decay from Y and E
  HistogramData::Histogram
  removeDecay(const HistogramData::Histogram &histogram) const;
  // calculate Muon normalisation constant
  double calNormalisationConst(API::MatrixWorkspace_sptr ws, int wsIndex);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_MUONREMOVEEXPDECAY_H_*/
