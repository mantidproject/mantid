// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
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
*/
class MANTID_ALGORITHMS_DLL CreateLogTimeCorrection : public API::Algorithm {
public:
  const std::string name() const override { return "CreateLogTimeCorrection"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Create log time correction table.  Correction for each pixel is "
           "based on L1 and L2.";
  }

  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"ChangePulsetime", "ShiftLogTime"}; }
  const std::string category() const override { return "Events\\EventFiltering"; }

private:
  /// Implement abstract Algorithm methods
  void init() override;
  /// Implement abstract Algorithm methods
  void exec() override;

  /// Log geometry information
  void logGeometryInformation(const Geometry::DetectorInfo &detectorInfo) const;

  /// Calculate the log time correction for each pixel, i.e., correcton from
  /// event time at detector to time at sample
  std::vector<double> calculateCorrections(const Geometry::DetectorInfo &detectorInfo) const;

  /// Write L2 map and correction map to a TableWorkspace
  DataObjects::TableWorkspace_sptr generateCorrectionTable(const Geometry::DetectorInfo &detectorInfo,
                                                           const std::vector<double> &corrections) const;

  /// Write correction map to a text file
  void writeCorrectionToFile(const std::string &filename, const Geometry::DetectorInfo &detectorInfo,
                             const std::vector<double> &corrections) const;
};

} // namespace Algorithms
} // namespace Mantid
