// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidReflectometry/DllConfig.h"

namespace Mantid {
namespace Reflectometry {

/** Converts wavelength to momentum transfer and calculates the Qz
  resolution for reflectometers at continuous beam sources.
*/
class MANTID_REFLECTOMETRY_DLL ReflectometryMomentumTransfer
    : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::vector<std::string> seeAlso() const override;
  const std::string summary() const override;

private:
  enum class SumType { LAMBDA, Q };
  struct Beam {
    double incidentAngularSpread{0.};
    double firstSlitAngularSpread{0.};
    double secondSlitAngularSpread{0.};
    double sampleWaviness{0.};
  };
  struct Setup {
    double chopperOpening{0.};
    double chopperPairDistance{0.};
    double chopperPeriod{0.};
    double chopperRadius{0.};
    double detectorResolution{0.};
    size_t foregroundStart{0};
    size_t foregroundEnd{0};
    double l1{0.};
    double l2{0.};
    double pixelSize{0.};
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
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
  static void addResolutionDX(const API::MatrixWorkspace &inWS,
                              API::MatrixWorkspace &outWS, const Setup &setup,
                              const Beam &beam);
  static double angularResolutionSquared(const API::MatrixWorkspace &ws,
                                         const Setup &setup, const Beam &beam);
  void convertToMomentumTransfer(API::MatrixWorkspace_sptr &ws);
  static const Beam createBeamStatistics(const API::MatrixWorkspace &ws);
  const Setup createSetup(const API::MatrixWorkspace &ws);
  double interslitDistance(const API::MatrixWorkspace &ws);
  double slitSize(const API::MatrixWorkspace &ws, const std::string &logEntry);
  static double wavelengthResolutionSquared(const Setup &setup,
                                            const double wavelength);
};

} // namespace Reflectometry
} // namespace Mantid
