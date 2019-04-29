// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT2_H_
#define MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT2_H_

#include "MantidAPI/Algorithm.h"

#include "MantidGeometry/IDTypes.h"

namespace Mantid {
namespace Geometry {
class Instrument;
class ReferenceFrame;
} // namespace Geometry
namespace Kernel {
class V3D;
}
namespace Algorithms {

/** SpecularReflectionPositionCorrect : Algorithm to perform position
corrections based on the specular reflection condition. Version 2.
*/
class DLLExport SpecularReflectionPositionCorrect2 final
    : public API::Algorithm {
public:
  /// Name of this algorithm
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override;
  /// Version
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"SpecularReflectionCalculateTheta", "FindReflectometryLines"};
  }
  /// Category
  const std::string category() const override;

private:
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
  void correctDetectorPosition(API::MatrixWorkspace_sptr &outWS,
                               const std::string &detectorName,
                               const detid_t detectorID,
                               const double twoThetaInRad,
                               const std::string &correctionType,
                               const Geometry::ReferenceFrame &referenceFrame,
                               const Kernel::V3D &samplePosition,
                               const Kernel::V3D &sampleToDetector,
                               const double beamOffsetOld);
  static Kernel::V3D declareDetectorPosition(const Geometry::Instrument &inst,
                                             const std::string &detectorName,
                                             const detid_t detectorID);
  Kernel::V3D declareSamplePosition(const API::MatrixWorkspace &ws);
  double twoThetaFromProperties(const API::MatrixWorkspace &inWS,
                                const double l2);
  double twoThetaFromDirectLine(const std::string &detectorName,
                                const detid_t detectorID,
                                const Kernel::V3D &samplePosition,
                                const double l2, const Kernel::V3D &alongDir,
                                const double beamOffset);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT2_H_ */
