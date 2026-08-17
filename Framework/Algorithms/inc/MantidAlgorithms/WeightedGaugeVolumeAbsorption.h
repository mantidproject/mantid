// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/AnyShapeAbsorption.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidAlgorithms/SampleCorrections/RadialCollimatorProfile.h"

namespace Mantid {
namespace Algorithms {

/** Attenuation correction weighted by the spatial resolution function.

    AbsorptionCorrection treats every volume element as scattering equally into every detector. That
    holds when the whole sample is uniformly lit and each detector sees all of it, but not for a
    strain or texture measurement, where the illuminated region is a small gauge volume shaped by
    slits and each detector views it through a radial collimator.

    This algorithm integrates the same quadrature over the same elements, but weights each element by
    the spatial resolution function SRF(r) = P_i(r) P_s(r) P_d(r) of Creek, Santisteban & Edwards
    (2005): P_i the incident beam profile, P_s the attenuation along both legs, and P_d the
    collimator acceptance. P_d depends on the detector, so the weighted volume - and hence the
    apparent scattering centre - differs from detector to detector.

    As well as the attenuation factor it can report, per detector, the neutron weighted centre of
    gravity of the volume that detector actually sees. For a gauge volume only partly immersed in the
    sample that centre is displaced from the geometric centre, which is the leading cause of
    pseudo-strain in near-surface measurements.

    Limitations, all inherited from the analytical treatment: single scattering only, attenuation
    through the sample alone with no container or environment, and a cost that scales as the number
    of detectors times the number of elements.
*/
class MANTID_ALGORITHMS_DLL WeightedGaugeVolumeAbsorption : public AnyShapeAbsorption {
public:
  /// Algorithm's name
  const std::string name() const override { return "WeightedGaugeVolumeAbsorption"; }

  const std::vector<std::string> seeAlso() const override {
    return {"AbsorptionCorrection", "CuboidGaugeVolumeAbsorption",          "DefineGaugeVolume",
            "MonteCarloAbsorption", "EstimateScatteringVolumeCentreOfMass", "SetBeam"};
  }

  const std::string summary() const override {
    return "Calculates the attenuation in a gauge volume, weighting each integration element by the "
           "incident beam profile and the collimator acceptance as well as the attenuation. Can also "
           "report the neutron weighted scattering centre seen by each detector.";
  }

  /// Algorithm's version
  int version() const override { return 1; }

protected:
  void defineProperties() override;
  void retrieveProperties() override;
  void initialiseCachedDistances() override;
  void calculateElementWeights(const Geometry::IDetector &detector, std::vector<double> &weights) const override;
  void perSpectrumHook(size_t wsIndex, const std::vector<double> &L2s, const std::vector<double> &weights) override;

private:
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
  /// Build the outputs accumulated by perSpectrumHook. Called after the base class has run.
  void setDerivedOutputs();

  /// P_i, evaluated once per element since the beam does not know about the detector
  std::vector<double> m_incidentIntensity;
  /// P_d, absent when the instrument carries no calibrated collimator width
  std::unique_ptr<RadialCollimatorProfile> m_collimator;
  Kernel::V3D m_samplePos;

  /// Per spectrum, accumulated by perSpectrumHook
  std::vector<Kernel::V3D> m_scatteringCentres;
  std::vector<double> m_centreWeights;
  std::vector<double> m_illuminatedFraction;

  /// Wavelengths the centre of gravity is summed over, matching the bin loop's subsample
  std::vector<double> m_lambdas;
  double m_linearCoefTotScattForCentres{0.0};
};

} // namespace Algorithms
} // namespace Mantid
