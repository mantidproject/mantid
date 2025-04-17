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

protected:
  API::MatrixWorkspace_sptr m_inputWS;         ///< A pointer to the input workspace
  const Geometry::IObject *m_sampleObject;     ///< Local cache of sample object.
  Kernel::V3D m_beamDirection;                 ///< The direction of the beam.
  std::vector<Kernel::V3D> m_elementPositions; ///< Cached element positions
  std::vector<double> m_averagePos;            ///< Average position of raster elements
  double m_cubeSide;                           ///< Element size of raster
  std::shared_ptr<const Geometry::IObject> constructGaugeVolume();
  std::vector<double> calcAveragePosition(const std::vector<Kernel::V3D> &pos);

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void constructSample(API::Sample &sample);
};

} // namespace Algorithms
} // namespace Mantid
