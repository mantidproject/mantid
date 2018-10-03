// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REMOVEPROMPTPULSE_H_
#define MANTID_ALGORITHMS_REMOVEPROMPTPULSE_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** RemovePromptPulse : TODO: DESCRIPTION

  @author
  @date 2011-07-18
*/
class DLLExport RemovePromptPulse : public API::DistributedAlgorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override;

  /// Algorithm's version for identification
  int version() const override;

  /// Algorithm's category for identification
  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Remove the prompt pulse for a time of flight measurement.";
  }

private:
  /// Sets documentation strings for this algorithm

  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Try to get the frequency from a given name.
  double getFrequency(const API::Run &run);
  std::vector<double> calculatePulseTimes(const double tmin, const double tmax,
                                          const double period);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REMOVEPROMPTPULSE_H_ */
