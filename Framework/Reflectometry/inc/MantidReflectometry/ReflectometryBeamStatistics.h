// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "MantidReflectometry/DllConfig.h"

namespace Mantid {
namespace Reflectometry {

/** Calculates statistical quantities of a reflectometry workspace.
 */
class MANTID_REFLECTOMETRY_DLL ReflectometryBeamStatistics
    : public API::Algorithm {
public:
  struct LogEntry {
    const static std::string BEAM_RMS_VARIATION;
    const static std::string BENT_SAMPLE;
    const static std::string FIRST_SLIT_ANGULAR_SPREAD;
    const static std::string INCIDENT_ANGULAR_SPREAD;
    const static std::string SAMPLE_WAVINESS;
    const static std::string SECOND_SLIT_ANGULAR_SPREAD;
  };
  static double
  slitSeparation(const Geometry::Instrument_const_sptr &instrument,
                 const std::string &slit1Name, const std::string &slit2Name);
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

private:
  struct Statistics {
    double beamRMSVariation{0.};
    bool bentSample{false};
    double firstSlitAngularSpread{0.};
    double incidentAngularSpread{0.};
    double sampleWaviness{0.};
    double secondSlitAngularSpread{0.};
  };
  struct Setup {
    double detectorResolution{0.};
    size_t foregroundStart{0};
    size_t foregroundEnd{0};
    size_t directForegroundStart{0};
    size_t directForegroundEnd{0};
    double l2{0.};
    double directL2{0.};
    double pixelSize{0.};
    double slit1Slit2Distance{0.};
    double slit1Size{0.};
    double slit1SizeDirectBeam{0.};
    double slit2SampleDistance{0.};
    double slit2Size{0.};
    double slit2SizeDirectBeam{0.};
  };
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  double beamRMSVariation(API::MatrixWorkspace_sptr &ws, const size_t start,
                          const size_t end);
  static bool bentSample(const Setup &setup, const double sampleWaviness,
                         const double firstSlitAngularSpread);
  const Setup createSetup(const API::MatrixWorkspace &ws,
                          const API::MatrixWorkspace &directWS);
  static double detectorAngularResolution(const Setup &setup,
                                          const double incidentFWHM);
  static double firstSlitAngularSpread(const Setup &setup);
  double incidentAngularSpread(const Setup &setup);
  double interslitDistance(const API::MatrixWorkspace &ws);
  static void rmsVariationToLogs(API::MatrixWorkspace &ws,
                                 const double variation);
  double sampleWaviness(const Setup &setup, const double beamFWHM,
                        const double directBeamFWHM, const double incidentFWHM);
  double secondSlitAngularSpread(const Setup &setup);
  double slitSize(const API::MatrixWorkspace &ws, const std::string &logEntry);
  static void statisticsToLogs(API::MatrixWorkspace &ws,
                               const Statistics &statistics);
};

} // namespace Reflectometry
} // namespace Mantid
