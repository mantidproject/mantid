// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/PeakShapeDetectorBin.h"
#include "MantidJson/Json.h"
#include "MantidKernel/cow_ptr.h"
#include <json/json.h>

#include <utility>

namespace Mantid::DataObjects {

/**
 * Create PeakShapeDetectorBin
 * @param detectorBinList The list containing each detector Ids associated with its start and end points in TOF or
 * dSpacing domains
 * @param frame SpecialCoordinateSystem frame, default is None
 * @param algorithmName Name of the algorithm using this shape
 * @param algorithmVersion Version of the above algorithm
 */
PeakShapeDetectorBin::PeakShapeDetectorBin(std::vector<std::tuple<int32_t, double, double>> detectorBinList,
                                           Kernel::SpecialCoordinateSystem frame, std::string algorithmName,
                                           int algorithmVersion)
    : PeakShapeBase(frame, std::move(algorithmName), algorithmVersion), m_detectorBinList(detectorBinList) {

  if (detectorBinList.size() == 0) {
    throw std::invalid_argument("No data provided for detector data");
  }
}

bool PeakShapeDetectorBin::operator==(const PeakShapeDetectorBin &other) const {
  return PeakShapeBase::operator==(other) && (this->m_detectorBinList == other.m_detectorBinList);
}

std::string PeakShapeDetectorBin::toJSON() const {
  Json::Value root;
  PeakShapeBase::buildCommon(root);
  Json::Value detBinList;
  for (auto const &[detectorId, startX, endX] : m_detectorBinList) {
    Json::Value detBinVal;
    detBinVal["detId"] = Json::Value(detectorId);
    detBinVal["startX"] = Json::Value(startX);
    detBinVal["endX"] = Json::Value(endX);
    detBinList.append(detBinVal);
  }

  root["detectors"] = detBinList;
  return Mantid::JsonHelpers::jsonToString(root);
}

Mantid::Geometry::PeakShape *PeakShapeDetectorBin::clone() const { return new PeakShapeDetectorBin(*this); }

std::string PeakShapeDetectorBin::shapeName() const { return PeakShapeDetectorBin::detectorBinShapeName(); }

std::optional<double> PeakShapeDetectorBin::radius(RadiusType type) const { return std::nullopt; }

const std::string PeakShapeDetectorBin::detectorBinShapeName() { return "PeakShapeDetectorBin"; }

} // namespace Mantid::DataObjects
