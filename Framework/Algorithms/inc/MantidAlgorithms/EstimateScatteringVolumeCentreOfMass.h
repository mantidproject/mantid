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
#include "MantidGeometry/Rasterize.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

namespace Mantid {

namespace API {
class Sample;
}

namespace Algorithms {

class MANTID_ALGORITHMS_DLL EstimateScatteringVolumeCentreOfMass : public API::Algorithm {
public:
  /// (Empty) Constructor
  EstimateScatteringVolumeCentreOfMass();
  /// Algorithm's category for identification
  const std::string category() const override { return "CorrectionFunctions\\AbsorptionCorrections"; }
  /// Algorithm's name for identification
  const std::string name() const override { return "EstimateScatteringVolumeCentreOfMass"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Estimate the centre of mass of the illuminated volume of the sample";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;

  /// Dice the illuminated volume into elements. With a GaugeVolume log the gauge is the
  /// integration volume and the raster comes back in the lab frame; without one the illuminated
  /// volume is the sample itself and the raster is in the sample shape's own frame.
  Geometry::Raster rasterizeScatteringVolume(const Geometry::IObject &sampleObject,
                                             const Kernel::Matrix<double> &gonioR, const Kernel::V3D &beamDirection);
  /// The centre of mass of the elements, weighted by element volume. The weighting matters
  /// because Rasterize dices a cylinder into annular segments of differing size, so treating
  /// every element equally would bias the centroid towards the finely divided inner annuli.
  static Kernel::V3D calcVolumeWeightedCentre(const Geometry::Raster &raster);
  static const Geometry::IObject_sptr extractValidSampleObject(const API::Sample &sample);
  /// The part of the workspace's goniometer rotation that the sample shape does not already carry.
  const Kernel::Matrix<double> outstandingSampleRotation(const Geometry::IObject &sampleObject) const;

  API::MatrixWorkspace_sptr m_inputWS; ///< A pointer to the input workspace
  double m_cubeSide;                   ///< Element size of raster
};

} // namespace Algorithms
} // namespace Mantid
