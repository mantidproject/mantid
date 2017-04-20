#ifndef MANTID_ALGORITHMS_REFLECTOMETRYWORKFLOWBASE2_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYWORKFLOWBASE2_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/Instrument_fwd.h"

namespace Mantid {
namespace Algorithms {

/** ReflectometryWorkflowBase2 : base class containing common implementation
 functionality usable by concrete reflectometry workflow algorithms. Version 2.

 Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ReflectometryWorkflowBase2
    : public API::DataProcessorAlgorithm {
protected:
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
  cropWavelength(Mantid::API::MatrixWorkspace_sptr inputWS);
  // Create a detector workspace from input workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr
  makeDetectorWS(Mantid::API::MatrixWorkspace_sptr inputWS);
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
  std::string populateProcessingInstructions(
      Mantid::API::IAlgorithm_sptr alg,
      Mantid::Geometry::Instrument_const_sptr instrument,
      Mantid::API::MatrixWorkspace_sptr inputWS) const;
  /// Populate transmission properties
  bool populateTransmissionProperties(Mantid::API::IAlgorithm_sptr alg) const;
};
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYWORKFLOWBASE2_H_ */
