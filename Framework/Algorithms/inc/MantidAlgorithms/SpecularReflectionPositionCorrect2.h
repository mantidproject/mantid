// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT2_H_
#define MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT2_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** SpecularReflectionPositionCorrect : Algorithm to perform vertical position
corrections based on the specular reflection condition. Version 2.
*/
class DLLExport SpecularReflectionPositionCorrect2 : public API::Algorithm {
public:
  /// Name of this algorithm
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override;
  /// Version
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"SpecularReflectionCalculateTheta",
            "ReflectometryCorrectDetectorAngle"};
  }
  /// Category
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT2_H_ */
