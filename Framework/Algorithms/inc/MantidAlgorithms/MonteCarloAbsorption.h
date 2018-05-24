#ifndef MANTID_ALGORITHMS_MONTECARLOABSORPTION_H_
#define MANTID_ALGORITHMS_MONTECARLOABSORPTION_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidAlgorithms/InterpolationOption.h"

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

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
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

  API::MatrixWorkspace_uptr doSimulation(
      const API::MatrixWorkspace &inputWS, const size_t nevents, int nlambda,
      const int seed, const InterpolationOption &interpolateOpt,
      const bool useSparseInstrument, const size_t maxScatterPtAttempts);
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
}
}

#endif // MANTID_ALGORITHMS_MONTECARLOABSORPTION_H_
