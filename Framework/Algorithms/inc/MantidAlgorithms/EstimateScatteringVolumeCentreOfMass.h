// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/V3D.h"

namespace Mantid {

namespace API {
class Sample;
}
namespace Geometry {
class IObject;
} // namespace Geometry

namespace Algorithms {

class MANTID_ALGORITHMS_DLL EstimateScatteringVolumeCentreOfMass : public API::Algorithm {
public:
  /// (Empty) Constructor
  EstimateScatteringVolumeCentreOfMass();
  /// Algorithm's category for identification
  const std::string category() const override { return "CorrectionFunctions\\EstimateScatteringVolumeCentreOfMass"; }
  /// Algorithm's name for identification
  const std::string name() const override { return "EstimateScatteringVolumeCentreOfMass"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Estimate the centre of mass of the illuminated volume of the sample";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };

  API::MatrixWorkspace_sptr m_inputWS; ///< A pointer to the input workspace
  Kernel::V3D m_beamDirection;         ///< The direction of the beam.
  double m_cubeSide;                   ///< Element size of raster
  const std::shared_ptr<const Geometry::IObject> getGaugeVolumeObject();
  const Kernel::V3D calcAveragePosition(const std::vector<Kernel::V3D> &pos);
  const Kernel::V3D
  rasterizeGaugeVolumeAndCalculateMeanElementPosition(const std::shared_ptr<const IObject> integrationVolume,
                                                      const std::shared_ptr<const IObject> sampleObject);
  const std::shared_ptr<const Geometry::IObject> extractValidSampleObject(const API::Sample &sample);

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
