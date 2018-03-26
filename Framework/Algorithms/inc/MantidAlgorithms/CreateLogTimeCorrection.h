#ifndef MANTID_ALGORITHMS_CREATELOGTIMECORRECTION_H_
#define MANTID_ALGORITHMS_CREATELOGTIMECORRECTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include <vector>

namespace Mantid {
namespace Geometry {
class DetectorInfo;
}
namespace Algorithms {

/** CreateLogTimeCorrection : Create correction file and workspace to correct
  event time against
  recorded log time for each pixel.

  It is assumed that the log time will be the same time as neutron arrives
  sample,
  and the input event workspace contains the neutron with time recorded at the
  detector.

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
class DLLExport CreateLogTimeCorrection : public API::Algorithm {
public:
  const std::string name() const override { return "CreateLogTimeCorrection"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Create log time correction table.  Correction for each pixel is "
           "based on L1 and L2.";
  }

  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"ChangePulsetime", "ShiftLogTime"};
  }
  const std::string category() const override {
    return "Events\\EventFiltering";
  }

private:
  /// Implement abstract Algorithm methods
  void init() override;
  /// Implement abstract Algorithm methods
  void exec() override;

  /// Log geometry information
  void logGeometryInformation(const Geometry::DetectorInfo &detectorInfo) const;

  /// Calculate the log time correction for each pixel, i.e., correcton from
  /// event time at detector to time at sample
  std::vector<double>
  calculateCorrections(const Geometry::DetectorInfo &detectorInfo) const;

  /// Write L2 map and correction map to a TableWorkspace
  DataObjects::TableWorkspace_sptr
  generateCorrectionTable(const Geometry::DetectorInfo &detectorInfo,
                          const std::vector<double> &corrections) const;

  /// Write correction map to a text file
  void writeCorrectionToFile(const std::string filename,
                             const Geometry::DetectorInfo &detectorInfo,
                             const std::vector<double> &corrections) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATELOGTIMECORRECTION_H_ */
