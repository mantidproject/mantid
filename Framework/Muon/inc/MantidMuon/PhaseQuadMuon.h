// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHM_PHASEQUADMUON_H_
#define MANTID_ALGORITHM_PHASEQUADMUON_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidMuon/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**Algorithm for calculating Muon spectra.

@author Raquel Alvarez, ISIS, RAL
@date 1/12/2014
*/
class MANTID_MUON_DLL PhaseQuadMuon : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "PhaseQuad"; }
  /// Summary of algorithm's purpose
  const std::string summary() const override {
    return "Generates a quadrature phase signal.";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"MuonMaxent"};
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Validate inputs
  std::map<std::string, std::string> validateInputs() override;
  /// Calculate the normalization constants
  std::vector<double> getExponentialDecay(const API::MatrixWorkspace_sptr &ws);
  /// Create squashograms
  API::MatrixWorkspace_sptr squash(const API::MatrixWorkspace_sptr &ws,
                                   const API::ITableWorkspace_sptr &phase,
                                   const std::vector<double> &n0);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_PHASEQUAD_H_*/
