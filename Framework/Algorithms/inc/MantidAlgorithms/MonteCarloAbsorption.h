// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MONTECARLOABSORPTION_H_
#define MANTID_ALGORITHMS_MONTECARLOABSORPTION_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/InterpolationOption.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"

namespace Mantid {
namespace API {
class Sample;
}
namespace Geometry {
class Instrument;
}

namespace Algorithms {
class DetectorGridDefinition;
/**
  Calculates attenuation due to absorption and scattering in a sample +
  its environment using a Monte Carlo algorithm.
*/
class DLLExport MonteCarloAbsorption : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "MonteCarloAbsorption"; }
  /// Algorithm's version
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"MayersSampleCorrection", "CarpenterSampleCorrection",
            "PearlMCAbsorption", "VesuvioCalculateMS"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "CorrectionFunctions\\AbsorptionCorrections";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates attenuation due to absorption and scattering in a "
           "sample & its environment using a Monte Carlo.";
  }

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;

  API::MatrixWorkspace_uptr
  doSimulation(const API::MatrixWorkspace &inputWS, const size_t nevents,
               const bool simulateTracksForEachWavelength, const int seed,
               const InterpolationOption &interpolateOpt,
               const bool useSparseInstrument,
               const size_t maxScatterPtAttempts);
  API::MatrixWorkspace_uptr
  createOutputWorkspace(const API::MatrixWorkspace &inputWS) const;
  std::unique_ptr<IBeamProfile>
  createBeamProfile(const Geometry::Instrument &instrument,
                    const API::Sample &sample) const;
  void interpolateFromSparse(
      API::MatrixWorkspace &targetWS, const API::MatrixWorkspace &sparseWS,
      const Mantid::Algorithms::InterpolationOption &interpOpt,
      const DetectorGridDefinition &detGrid);
};
} // namespace Algorithms
} // namespace Mantid

#endif // MANTID_ALGORITHMS_MONTECARLOABSORPTION_H_
