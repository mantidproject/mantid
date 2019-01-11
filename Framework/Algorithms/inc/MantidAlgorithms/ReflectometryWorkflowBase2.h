// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REFLECTOMETRYWORKFLOWBASE2_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYWORKFLOWBASE2_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/Instrument_fwd.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
namespace Mantid {
namespace Algorithms {

/** ReflectometryWorkflowBase2 : base class containing common implementation
 functionality usable by concrete reflectometry workflow algorithms. Version 2.
 */
class DLLExport ReflectometryWorkflowBase2
    : public API::DataProcessorAlgorithm {
protected:
  /// Initialize reduction-type properties
  void initReductionProperties();
  /// Initialize monitor properties
  void initMonitorProperties();
  /// Initialize direct beam properties
  void initDirectBeamProperties();
  /// Initialize transmission properties
  void initTransmissionProperties();
  /// Initialize properties for stitching transmission runs
  void initStitchProperties();
  /// Initialize corection algorithm properties
  void initAlgorithmicProperties(bool autodetect = false);
  /// Initialize momentum transfer properties
  void initMomentumTransferProperties();
  /// Initialize properties for diagnostics
  void initDebugProperties();
  /// Validate reduction-type properties
  std::map<std::string, std::string> validateReductionProperties() const;
  /// Validate direct beam properties
  std::map<std::string, std::string> validateDirectBeamProperties() const;
  /// Validate transmission properties
  std::map<std::string, std::string> validateTransmissionProperties() const;
  /// Validate wavelength range
  std::map<std::string, std::string> validateWavelengthRanges() const;
  /// Convert a workspace from TOF to wavelength
  Mantid::API::MatrixWorkspace_sptr
  convertToWavelength(Mantid::API::MatrixWorkspace_sptr inputWS);
  /// Crop a workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr
  cropWavelength(Mantid::API::MatrixWorkspace_sptr inputWS,
                 const bool useArgs = false, const double argMin = 0.0,
                 const double argMax = 0.0);
  // Create a detector workspace from input workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr
  makeDetectorWS(Mantid::API::MatrixWorkspace_sptr inputWS,
                 const bool convert = true);
  // Create a monitor workspace from input workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr
  makeMonitorWS(Mantid::API::MatrixWorkspace_sptr inputWS,
                bool integratedMonitors);
  // Rebin detectors to monitors
  Mantid::API::MatrixWorkspace_sptr
  rebinDetectorsToMonitors(Mantid::API::MatrixWorkspace_sptr detectorWS,
                           Mantid::API::MatrixWorkspace_sptr monitorWS);
  // Read monitor properties from instrument
  void
  populateMonitorProperties(Mantid::API::IAlgorithm_sptr alg,
                            Mantid::Geometry::Instrument_const_sptr instrument);
  /// Populate processing instructions
  std::string
  findProcessingInstructions(Mantid::Geometry::Instrument_const_sptr instrument,
                             Mantid::API::MatrixWorkspace_sptr inputWS) const;
  /// Populate transmission properties
  bool populateTransmissionProperties(Mantid::API::IAlgorithm_sptr alg) const;
  /// Find theta from a named log value
  double getThetaFromLogs(Mantid::API::MatrixWorkspace_sptr inputWs,
                          const std::string &logName);
  // Retrieve the run number from the logs of the input workspace.
  std::string getRunNumber(Mantid::API::MatrixWorkspace const &ws) const;

  void convertProcessingInstructions(Instrument_const_sptr instrument,
                                     MatrixWorkspace_sptr inputWS);
  void convertProcessingInstructions(MatrixWorkspace_sptr inputWS);
  std::string m_processingInstructionsWorkspaceIndex;
  std::string m_processingInstructions;

protected:
  std::string
  convertToSpectrumNumber(const std::string &workspaceIndex,
                          Mantid::API::MatrixWorkspace_const_sptr ws) const;

  std::string convertProcessingInstructionsToWorkspaceIndices(
      const std::string &instructions,
      Mantid::API::MatrixWorkspace_const_sptr ws) const;

  std::string convertToWorkspaceIndex(const std::string &spectrumNumber,
                                      MatrixWorkspace_const_sptr ws) const;

  std::string convertProcessingInstructionsToSpectrumNumbers(
      const std::string &instructions,
      Mantid::API::MatrixWorkspace_const_sptr ws) const;
};
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYWORKFLOWBASE2_H_ */
