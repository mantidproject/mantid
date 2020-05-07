// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"

namespace Mantid {
namespace API {
class Sample;
}
namespace Geometry {
class Instrument;
}
namespace Algorithms {

/** Takes an existing sample log, and calculates its first or second
 * derivative, and adds it as a new log.

  @author Danny Hindson
  @date 2020-05-07
*/
class MANTID_ALGORITHMS_DLL AddAbsorptionWeightedPathLengths
    : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override {
    return "AddAbsorptionWeightedPathLengths";
  };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Add absorption weighted path lengths to each peak in a peaks "
           "workspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"MonteCarloAbsorption, SetSample, SaveReflections"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Peaks"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  std::unique_ptr<IBeamProfile>
  createBeamProfile(const Geometry::Instrument &instrument,
                    const API::Sample &sample) const;
};

} // namespace Algorithms
} // namespace Mantid