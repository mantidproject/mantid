#ifndef MANTID_ALGORITHMS_REFLECTOMETRYWORKFLOWBASE_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYWORKFLOWBASE_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/System.h"

#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <vector>

namespace Mantid {

namespace HistogramData {
class HistogramX;
}
namespace Algorithms {

/** ReflectometryWorkflowBase : Abstract workflow algortithm base class
 containing common implementation functionality usable
 *  by concrete reflectometry workflow algorithms.

 Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ReflectometryWorkflowBase : public API::DataProcessorAlgorithm {
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
  toLam(Mantid::API::MatrixWorkspace_sptr toConvert,
        const std::string &processingCommands,
        const OptionalInteger monitorIndex, const MinMax &wavelengthMinMax,
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
  OptionalMinMax getOptionalMinMax(Mantid::API::Algorithm *const alg,
                                   const std::string &minProperty,
                                   const std::string &maxProperty,
                                   Mantid::Geometry::Instrument_const_sptr inst,
                                   std::string minIdfName,
                                   std::string maxIdfName) const;
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
               const OptionalInteger monitorIndex,
               const OptionalMinMax &backgroundMinMax);

  /// Make a unity workspace
  API::MatrixWorkspace_sptr
  makeUnityWorkspace(const HistogramData::HistogramX &x);
};
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYWORKFLOWBASE_H_ */
