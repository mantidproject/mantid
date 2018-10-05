// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REFLECTOMETRYCORRECTDETECTORANGLE_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYCORRECTDETECTORANGLE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Algorithms {

/** Corrects the angle of a reflectometry line detector.
 */
class MANTID_ALGORITHMS_DLL ReflectometryCorrectDetectorAngle
    : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::vector<std::string> seeAlso() const override;
  const std::string summary() const override;

private:
  struct ComponentPositions {
    Kernel::V3D sample;
    Kernel::V3D detector;
    double l2;
  };

  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  void correctDetectorPosition(API::MatrixWorkspace_sptr &ws,
                               ComponentPositions const &positions,
                               double const twoTheta);
  double correctedTwoTheta(API::MatrixWorkspace const &ws, const double l2);
  void moveComponent(API::MatrixWorkspace_sptr &ws, std::string const &name,
                     Kernel::V3D const &position);
  double offsetAngleFromCentre(API::MatrixWorkspace const &ws, double const l2,
                               double const linePosition);
  void rotateComponent(API::MatrixWorkspace_sptr &ws, std::string const &name,
                       Kernel::V3D const &rotationAxis, double const angle);
  ComponentPositions sampleAndDetectorPositions(API::MatrixWorkspace const &ws);
  double signedDetectorAngle(API::MatrixWorkspace const &ws);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYCORRECTDETECTORANGLE_H_ */
