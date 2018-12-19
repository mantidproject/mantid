// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SPECULARREFLECTIONCALCULATETHETA_H_
#define MANTID_ALGORITHMS_SPECULARREFLECTIONCALCULATETHETA_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/SpecularReflectionAlgorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** SpecularReflectionCorrectTheta : Calculates a theta value based on the
  specular reflection condition.
*/
class DLLExport SpecularReflectionCalculateTheta
    : public SpecularReflectionAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculate the specular reflection two theta scattering angle "
           "(degrees) from the detector and sample locations .";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SPECULARREFLECTIONCALCULATETHETA_H_ */
