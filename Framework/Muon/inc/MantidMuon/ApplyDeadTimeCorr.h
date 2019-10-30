// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_APPLYDEADTIMECORR_H_
#define MANTID_ALGORITHMS_APPLYDEADTIMECORR_H_

#include "MantidAPI/Algorithm.h"
#include "MantidMuon/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** ApplyDeadTimeCorr : The dead-time is here applied according to the
  non-extendable paralyzable dead time model.

  @author Robert Whitley, RAL
  @date 2011-11-30
*/
class MANTID_MUON_DLL ApplyDeadTimeCorr : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "ApplyDeadTimeCorr"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Apply deadtime correction to each spectrum of a workspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"CalMuonDeadTime"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Muon;CorrectionFunctions\\EfficiencyCorrections";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_APPLYDEADTIMECORR_H_ */
