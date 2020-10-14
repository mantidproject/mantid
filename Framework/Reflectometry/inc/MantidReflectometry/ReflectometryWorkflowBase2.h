// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "MantidReflectometry/DllConfig.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
namespace Mantid {
namespace Reflectometry {

/** ReflectometryWorkflowBase2 : base class containing common implementation
 functionality usable by concrete reflectometry workflow algorithms. Version 2.
 */
class MANTID_REFLECTOMETRY_DLL ReflectometryWorkflowBase2
    : public API::DataProcessorAlgorithm {
protected:
  /// Initialize reduction-type properties
  void initReductionProperties();
  /// Initialize monitor properties
  void initMonitorProperties();
  /// Initialize direct beam properties
  void initDirectBeamProperties();
  /// Initialize background subtraction properties
  void initBackgroundProperties();
  /// Initialize transmission properties
  void initTransmissionProperties();
  void initTransmissionOutputProperties();
  /// Initialize properties for stitching transmission runs
  void initStitchProperties();
  /// Initialize corection algorithm properties
  void initAlgorithmicProperties(bool autodetect = false);
  /// Initialize momentum transfer properties
  void initMomentumTransferProperties();
  /// Initialize properties for diagnostics
  void initDebugProperties();
  /// Validate background-type properties
  std::map<std::string, std::string> validateBackgroundProperties() const;
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
  convertToWavelength(const Mantid::API::MatrixWorkspace_sptr &inputWS);
  /// Crop a workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr
  cropWavelength(const Mantid::API::MatrixWorkspace_sptr &inputWS,
                 const bool useArgs = false, const double argMin = 0.0,
                 const double argMax = 0.0);
  // Create a detector workspace from input workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr
  makeDetectorWS(Mantid::API::MatrixWorkspace_sptr inputWS,
                 const bool convert = true, const bool sum = true);
  // Create a monitor workspace from input workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr
  makeMonitorWS(const Mantid::API::MatrixWorkspace_sptr &inputWS,
                bool integratedMonitors);
  // Rebin detectors to monitors
  Mantid::API::MatrixWorkspace_sptr
  rebinDetectorsToMonitors(const Mantid::API::MatrixWorkspace_sptr &detectorWS,
                           const Mantid::API::MatrixWorkspace_sptr &monitorWS);
  // Read monitor properties from instrument
  void populateMonitorProperties(
      const Mantid::API::IAlgorithm_sptr &alg,
      const Mantid::Geometry::Instrument_const_sptr &instrument);
  /// Populate processing instructions
  std::string findProcessingInstructions(
      const Mantid::Geometry::Instrument_const_sptr &instrument,
      const Mantid::API::MatrixWorkspace_sptr &inputWS) const;
  /// Populate transmission properties
  bool
  populateTransmissionProperties(const Mantid::API::IAlgorithm_sptr &alg) const;
  /// Find theta from a named log value
  double getThetaFromLogs(const Mantid::API::MatrixWorkspace_sptr &inputWs,
                          const std::string &logName);
  // Retrieve the run number from the logs of the input workspace.
  std::string getRunNumber(Mantid::API::MatrixWorkspace const &ws) const;

  void convertProcessingInstructions(const Instrument_const_sptr &instrument,
                                     const MatrixWorkspace_sptr &inputWS);
  void convertProcessingInstructions(const MatrixWorkspace_sptr &inputWS);
  std::string m_processingInstructionsWorkspaceIndex;
  std::string m_processingInstructions;

protected:
  std::string convertToSpectrumNumber(
      const std::string &workspaceIndex,
      const Mantid::API::MatrixWorkspace_const_sptr &ws) const;
  std::string convertProcessingInstructionsToSpectrumNumbers(
      const std::string &instructions,
      const Mantid::API::MatrixWorkspace_const_sptr &ws) const;

  void setWorkspacePropertyFromChild(const Algorithm_sptr &alg,
                                     std::string const &propertyName);
};
} // namespace Reflectometry
} // namespace Mantid
