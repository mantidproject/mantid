// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** RemovePromptPulse : TODO: DESCRIPTION

  @author
  @date 2011-07-18
*/
class MANTID_ALGORITHMS_DLL RemovePromptPulse : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override;

  /// Algorithm's version for identification
  int version() const override;

  /// Algorithm's category for identification
  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override { return "Remove the prompt pulse for a time of flight measurement."; }

private:
  /// Sets documentation strings for this algorithm

  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Try to get the frequency from a given name.
  double getFrequency(const API::Run &run);
  void getTofRange(const API::MatrixWorkspace_const_sptr &wksp, double &tmin, double &tmax);
  std::vector<double> calculatePulseTimes(const double tmin, const double tmax, const double period,
                                          const double width);
};

} // namespace Algorithms
} // namespace Mantid
