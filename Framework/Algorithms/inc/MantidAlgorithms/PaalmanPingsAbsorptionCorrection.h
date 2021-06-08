// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"

namespace Mantid {

namespace API {
class Sample;
}
namespace Geometry {
class IDetector;
class IObject;
} // namespace Geometry

namespace Algorithms {
/** PaalmanPingsAbsorptionCorrection : calculate paalman-pings absorption terms
 */
/** Expansion of the AbsorptionCorrection algorithm to calculate full
   paalman-pings absorption terms

*/
class MANTID_ALGORITHMS_DLL PaalmanPingsAbsorptionCorrection : public API::Algorithm {
public:
  /// (Empty) Constructor
  PaalmanPingsAbsorptionCorrection();
  /// Algorithm's name
  const std::string name() const override { return "PaalmanPingsAbsorptionCorrection"; }
  /// Algorithm's category for identification
  const std::string category() const override { return "CorrectionFunctions\\AbsorptionCorrections"; }
  /// Algorithm's summary
  const std::string summary() const override {
    return "Calculates the attenuation due to absorption "
           "and single scattering in a generic sample shape for all "
           "Paalmin-pings terms.";
  }

  const std::vector<std::string> seeAlso() const override {
    return {"SetSampleMaterial", "AbsorptionCorrection", "ApplyPaalmanPingsCorrection",
            "PaalmanPingsMonteCarloAbsorption"};
  }

  /// Algorithm's version
  int version() const override { return (1); }

private:
  /// Initialisation code
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  /// Execution code
  void exec() override;

  void retrieveBaseProperties();
  void constructSample(API::Sample &sample);
  void calculateDistances(const Geometry::IDetector &detector, std::vector<double> &sample_L2s,
                          std::vector<double> &sample_container_L2s, std::vector<double> &container_L2s,
                          std::vector<double> &container_sample_L2s) const;
  void doIntegration(double &integral, double &crossIntegral, const double linearCoefAbs,
                     const double linearCoefTotScatt, const std::vector<double> &elementVolumes,
                     const std::vector<double> &L1s, const std::vector<double> &L2s, const double linearCoefAbs2,
                     const double linearCoefTotScatt2, const std::vector<double> &L1s2, const std::vector<double> &L2s2,
                     const size_t startIndex, const size_t endIndex) const;
  void defineProperties();
  void retrieveProperties();
  void initialiseCachedDistances();

  API::MatrixWorkspace_sptr m_inputWS;                  ///< A pointer to the input workspace
  const Geometry::IObject *m_sampleObject;              ///< Local cache of sample object.
  const Geometry::IObject *m_containerObject;           ///< Local cache of container object.
  Kernel::V3D m_beamDirection;                          ///< The direction of the beam.
  std::vector<double> m_sampleL1s,                      ///< Cached sample L1 distances
      m_sample_containerL1s,                            ///< Cached L1 distances through container hitting
                                                        ///< sample
      m_sampleElementVolumes;                           ///< Cached sample element volumes
  std::vector<Kernel::V3D> m_sampleElementPositions;    ///< Cached sample element positions
  size_t m_numSampleVolumeElements;                     ///< The number of sample volume elements
  double m_sampleVolume;                                ///< The total volume of the sample
  std::vector<double> m_containerL1s,                   ///< Cached container L1 distances
      m_container_sampleL1s,                            ///< Cached L1 distances through sample hitting
                                                        ///< container
      m_containerElementVolumes;                        ///< Cached container element volumes
  std::vector<Kernel::V3D> m_containerElementPositions; ///< Cached container element positions
  size_t m_numContainerVolumeElements;                  ///< The number of container volume elements
  double m_containerVolume;                             ///< The total volume of the container
  Kernel::Material m_material;
  Kernel::Material m_containerMaterial;
  double m_ampleLinearCoefTotScatt;     ///< The total scattering cross-section in
                                        ///< 1/m for the sample
  double m_containerLinearCoefTotScatt; ///< The total scattering cross-section
                                        ///< in 1/m for the container
  int64_t m_num_lambda;                 ///< The number of points in wavelength, the rest is
  /// interpolated linearly
  int64_t m_xStep; ///< The step in bin number between adjacent points

  /// Create the gague volume for the correction
  std::shared_ptr<const Geometry::IObject> constructGaugeVolume();
  double m_cubeSide; ///< The length of the side of an element cube in m
};

} // namespace Algorithms
} // namespace Mantid
