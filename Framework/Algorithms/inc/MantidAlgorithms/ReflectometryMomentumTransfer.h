#ifndef MANTID_ALGORITHMS_REFLECTOMETRYMOMENTUMTRANSFER_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYMOMENTUMTRANSFER_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** Converts wavelength to momentum transfer and calculates the Qz
  resolution for reflectometers at continuous beam sources.

  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_ALGORITHMS_DLL ReflectometryMomentumTransfer
    : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::vector<std::string> seeAlso() const override;
  const std::string summary() const override;

private:
  enum class SumType { LAMBDA, Q };
  struct Setup {
    double chopperOpening{0.};
    double chopperPairDistance{0.};
    double chopperPeriod{0.};
    double chopperRadius{0.};
    double detectorResolution{0.};
    size_t foregroundStart{0};
    size_t foregroundEnd{0};
    size_t directForegroundStart{0};
    size_t directForegroundEnd{0};
    double l1{0.};
    double l2{0.};
    double pixelSize{0.};
    bool polarized{false};
    double slit1Slit2Distance{0.};
    double slit1Size{0.};
    double slit1SizeDirectBeam{0.};
    double slit2SampleDistance{0.};
    double slit2Size{0.};
    double slit2SizeDirectBeam{0.};
    SumType sumType{SumType::LAMBDA};
    double tofChannelWidth{0.};
  };
  void init() override;
  void exec() override;
  double angularResolutionSquared(API::MatrixWorkspace_sptr &ws,
                                  const API::MatrixWorkspace &directWS,
                                  const size_t wsIndex, const Setup &setup,
                                  const double beamFWHM,
                                  const double directBeamFWHM,
                                  const double incidentFWHM,
                                  const double slit1FWHM);
  double beamRMSVariation(API::MatrixWorkspace_sptr &ws, const size_t start,
                          const size_t end);
  void convertToMomentumTransfer(API::MatrixWorkspace_sptr &ws);
  double detectorAngularResolution(const API::MatrixWorkspace &ws,
                                   const size_t wsIndex, const Setup &setup,
                                   const double incidentFWHM);
  const Setup createSetup(const API::MatrixWorkspace &ws,
                          const API::MatrixWorkspace &directWS);
  double incidentAngularSpread(const Setup &setup);
  double interslitDistance(const API::MatrixWorkspace &ws);
  double sampleWaviness(API::MatrixWorkspace_sptr &ws,
                        const API::MatrixWorkspace &directWS,
                        const size_t wsIndex, const Setup &setup,
                        const double beamFWHM, const double directBeamFWHM,
                        const double incidentFWHM);
  double slit1AngularSpread(const Setup &setup);
  double slit2AngularSpread(const API::MatrixWorkspace &ws,
                            const size_t wsIndex, const Setup &setup);
  double slitSize(const API::MatrixWorkspace &ws, const std::string &logEntry);
  double wavelengthResolutionSquared(const API::MatrixWorkspace &ws,
                                     const size_t wsIndex, const Setup &setup,
                                     const double wavelength);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYMOMENTUMTRANSFER_H_ */
