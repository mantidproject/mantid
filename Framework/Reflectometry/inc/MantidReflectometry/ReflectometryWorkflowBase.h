// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidGeometry/Instrument.h"
#include "MantidReflectometry/DllConfig.h"

#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <vector>

namespace Mantid {

namespace HistogramData {
class HistogramX;
}
namespace Reflectometry {

/** ReflectometryWorkflowBase : Abstract workflow algortithm base class
 containing common implementation functionality usable
 *  by concrete reflectometry workflow algorithms.
 */
class MANTID_REFLECTOMETRY_DLL ReflectometryWorkflowBase
    : public API::DataProcessorAlgorithm {
public:
  // Class typedefs
  using MinMax = boost::tuple<double, double>;
  using OptionalMinMax = boost::optional<MinMax>;
  using OptionalDouble = boost::optional<double>;
  using OptionalMatrixWorkspace_sptr =
      boost::optional<Mantid::API::MatrixWorkspace_sptr>;
  using WorkspaceIndexList = std::vector<int>;
  using OptionalWorkspaceIndexes = boost::optional<std::vector<int>>;
  using DetectorMonitorWorkspacePair =
      boost::tuple<Mantid::API::MatrixWorkspace_sptr,
                   Mantid::API::MatrixWorkspace_sptr>;
  using OptionalInteger = boost::optional<int>;

  /// Convert the input workspace to wavelength, splitting according to the
  /// properties provided.
  DetectorMonitorWorkspacePair
  toLam(const Mantid::API::MatrixWorkspace_sptr &toConvert,
        const std::string &processingCommands,
        const OptionalInteger &monitorIndex, const MinMax &wavelengthMinMax,
        const OptionalMinMax &backgroundMinMax);

  /// Convert the detector spectrum of the input workspace to wavelength
  API::MatrixWorkspace_sptr
  toLamDetector(const std::string &processingCommands,
                const API::MatrixWorkspace_sptr &toConvert,
                const MinMax &wavelengthMinMax);

protected:
  /// Determine if the property has it's default value.
  bool isPropertyDefault(const std::string &propertyName) const;

  /// Get a workspace index list.
  const std::string getWorkspaceIndexList() const;

  /// Get min max indexes.
  void fetchOptionalLowerUpperPropertyValue(
      const std::string &propertyName, bool isPointDetector,
      OptionalWorkspaceIndexes &optionalUpperLower) const;

  /// Get the min/max property values
  MinMax getMinMax(const std::string &minProperty,
                   const std::string &maxProperty) const;
  OptionalMinMax getOptionalMinMax(
      Mantid::API::Algorithm *const alg, const std::string &minProperty,
      const std::string &maxProperty,
      const Mantid::Geometry::Instrument_const_sptr &inst,
      const std::string &minIdfName, const std::string &maxIdfName) const;
  /// Get the transmission correction properties
  void getTransmissionRunInfo(
      OptionalMatrixWorkspace_sptr &firstTransmissionRun,
      OptionalMatrixWorkspace_sptr &secondTransmissionRun,
      OptionalDouble &stitchingStart, OptionalDouble &stitchingDelta,
      OptionalDouble &stitchingEnd, OptionalDouble &stitchingStartOverlap,
      OptionalDouble &stitchingEndOverlap) const;

  /// Init common index inputs
  void initIndexInputs();
  /// Init common wavelength inputs
  void initWavelengthInputs();
  /// Init common stitching inputs
  void initStitchingInputs();

private:
  /// Validate the transmission correction property inputs
  void validateSecondTransmissionInputs(
      const bool firstTransmissionInWavelength) const;

  /// Validate the the first transmission workspace.
  bool validateFirstTransmissionInputs() const;

  /// Convert the monitor parts of the input workspace to wavelength
  API::MatrixWorkspace_sptr
  toLamMonitor(const API::MatrixWorkspace_sptr &toConvert,
               const OptionalInteger &monitorIndex,
               const OptionalMinMax &backgroundMinMax);

  /// Make a unity workspace
  API::MatrixWorkspace_sptr
  makeUnityWorkspace(const HistogramData::HistogramX &x);
};
} // namespace Reflectometry
} // namespace Mantid
