// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/BoxControllerSettingsAlgorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"

namespace Mantid {
namespace MDAlgorithms {

/** Convert a MatrixWorkspace containing to a MD workspace for
 * viewing the detector face.

  @date 2012-03-08
*/
class DLLExport ConvertToDetectorFaceMD
    : public API::BoxControllerSettingsAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Convert a MatrixWorkspace containing to a MD workspace for viewing "
           "the detector face.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertToMD"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  std::map<int, Geometry::RectangularDetector_const_sptr> getBanks();

  template <class T, class MDE, size_t nd>
  void convertEventList(
      std::shared_ptr<Mantid::DataObjects::MDEventWorkspace<MDE, nd>> outWS,
      size_t workspaceIndex, coord_t x, coord_t y, coord_t bankNum,
      uint16_t runIndex, int32_t detectorID);

  /// The input event workspace
  Mantid::DataObjects::EventWorkspace_sptr in_ws;

  /// Width in pixels of the widest detector
  int m_numXPixels = 0;
  /// Height in pixels of the widest detector
  int m_numYPixels = 0;

  // Map between the detector ID and the workspace index
  std::vector<size_t> m_detID_to_WI;
  detid_t m_detID_to_WI_offset = 0;
};

} // namespace MDAlgorithms
} // namespace Mantid
