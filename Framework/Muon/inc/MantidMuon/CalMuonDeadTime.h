// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHM_CALMUONDEADTIME_H_
#define MANTID_ALGORITHM_CALMUONDEADTIME_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidMuon/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**Algorithm for calculating Muon dead times.

@author Anders Markvardsen, ISIS, RAL
@date 1/12/2011
*/
class MANTID_MUON_DLL CalMuonDeadTime : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CalMuonDeadTime"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculate Muon deadtime for each spectra in a workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"ApplyDeadTimeCorr"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_CALMUONDEADTIME_H_*/
