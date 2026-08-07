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
  double m_cubeSide;                   ///< Element size of raster
  const Kernel::V3D calcAveragePosition(const std::vector<Kernel::V3D> &pos);
  const Geometry::IObject_sptr extractValidSampleObject(const API::Sample &sample);
  /// Build the integration elements making up the scattering volume, returned as voxel centres in
  /// the lab frame. This is the single source of the volume for both the plain geometric centre of
  /// mass and the neutron weighted per-detector centres, so the two cannot drift apart.
  const std::vector<Kernel::V3D> generateScatteringVolumeElements(const Geometry::IObject &sampleObject,
                                                                  const Kernel::Matrix<double> &gonioR,
                                                                  const Kernel::V3D &beamDirection);
  /// Rasterise the workspace's lab-frame GaugeVolume directly, transforming each candidate voxel
  /// into the sample shape's frame via gonioR.inv() before testing it against the sample. Keeping
  /// the gauge in its own frame avoids inflating its axis-aligned bounding box for non-axis-aligned
  /// goniometer rotations, which would otherwise admit voxels lying outside the actual gauge
  /// volume. Returns the accepted element positions directly in the lab frame.
  const std::vector<Kernel::V3D> rasterizeLabGaugeElements(const Geometry::IObject &sampleObject,
                                                           const Kernel::Matrix<double> &gonioR);

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
};

} // namespace Algorithms
} // namespace Mantid
