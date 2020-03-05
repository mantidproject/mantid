// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHM_RRFMUON_H_
#define MANTID_ALGORITHM_RRFMUON_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/ListValidator.h"
#include "MantidMuon/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**Algorithm for calculating Muon decay envelope.

@author Raquel Alvarez, ISIS, RAL
@date 5/12/2014
*/
class MANTID_MUON_DLL RRFMuon : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RRFMuon"; }
  /// Summary of algorithm's purpose
  const std::string summary() const override {
    return "Calculate Muon asymmetry in the rotating reference frame (RRF).";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"CalculateMuonAsymmetry"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Get conversion factor from frequency units to input workspace units
  double unitConversionFactor(std::string uin, std::string uuser);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_RRFMUON_H_*/
