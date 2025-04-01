// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/InterpolationOption.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidAlgorithms/SampleCorrections/MCAbsorptionStrategy.h"
#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MantidAlgorithms/SampleCorrections/SparseWorkspace.h"

namespace Mantid {
namespace API {
class Sample;
}
namespace Geometry {
class Instrument;
}

namespace Algorithms {
class DetectorGridDefinition;
class MCInteractionVolume;

/**
  Calculates attenuation due to absorption and scattering in a sample +
  its environment using a Monte Carlo algorithm.
*/
class MANTID_ALGORITHMS_DLL MonteCarloAbsorption : public API::Algorithm {
public:
  virtual ~MonteCarloAbsorption() override = default;
  /// Algorithm's name
  const std::string name() const override { return "MonteCarloAbsorption"; }
  /// Algorithm's version
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"MayersSampleCorrection", "CarpenterSampleCorrection", "PearlMCAbsorption", "VesuvioCalculateMS"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "CorrectionFunctions\\AbsorptionCorrections"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates attenuation due to absorption and scattering in a "
           "sample & its environment using a Monte Carlo.";
  }

protected:
  virtual std::shared_ptr<IMCAbsorptionStrategy> createStrategy(std::shared_ptr<IMCInteractionVolume> interactionVol,
                                                                const IBeamProfile &beamProfile,
                                                                Kernel::DeltaEMode::Type EMode, const size_t nevents,
                                                                const size_t maxScatterPtAttempts,
                                                                const bool regenerateTracksForEachLambda);
  virtual std::shared_ptr<IMCInteractionVolume>
  createInteractionVolume(const API::Sample &sample, const size_t maxScatterPtAttempts,
                          const MCInteractionVolume::ScatteringPointVicinity pointsIn,
                          Geometry::IObject_sptr gaugeVolume = nullptr);
  virtual std::shared_ptr<SparseWorkspace> createSparseWorkspace(const API::MatrixWorkspace &modelWS,
                                                                 const size_t wavelengthPoints, const size_t rows,
                                                                 const size_t columns);
  virtual std::unique_ptr<InterpolationOption> createInterpolateOption();
  std::unique_ptr<IBeamProfile> createBeamProfile(const Geometry::Instrument &instrument,
                                                  const API::Sample &sample) const;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;

  API::MatrixWorkspace_uptr doSimulation(const API::MatrixWorkspace &inputWS, const size_t nevents,
                                         const bool simulateTracksForEachWavelength, const int seed,
                                         const InterpolationOption &interpolateOpt, const bool useSparseInstrument,
                                         const size_t maxScatterPtAttempts,
                                         const MCInteractionVolume::ScatteringPointVicinity pointsIn);
  API::MatrixWorkspace_uptr createOutputWorkspace(const API::MatrixWorkspace &inputWS) const;
  void interpolateFromSparse(API::MatrixWorkspace &targetWS, const SparseWorkspace &sparseWS,
                             const Mantid::Algorithms::InterpolationOption &interpOpt);
};
} // namespace Algorithms
} // namespace Mantid
