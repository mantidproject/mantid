// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** ApplyDetailedBalance : The algorithm calculates the imaginary part of the
  dynamic susceptibility chi''=Pi*(1-exp(-E/T))*S

  @author Andrei Savici, ORNL
  @date 2011-09-01
*/
class MANTID_ALGORITHMS_DLL ApplyDetailedBalance : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "ApplyDetailedBalance"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Transform scattering intensity to dynamic susceptibility."; }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override { return "Inelastic\\Corrections"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
