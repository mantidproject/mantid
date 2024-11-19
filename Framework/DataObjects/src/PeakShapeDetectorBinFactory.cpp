// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/PeakShapeDetectorBinFactory.h"
#include "MantidDataObjects/PeakShapeDetectorBin.h"
#include "MantidJson/Json.h"
#include "MantidKernel/SpecialCoordinateSystem.h"

#include <json/json.h>
#include <stdexcept>

using namespace Mantid::Kernel;

namespace Mantid::DataObjects {

/**
 * @brief Create the PeakShapeDetectorBin
 * @param source : source JSON
 * @return PeakShape via this factory or it's successors
 */
Mantid::Geometry::PeakShape *PeakShapeDetectorBinFactory::create(const std::string &source) const {
  Json::Value root;
  Mantid::Geometry::PeakShape *peakShape = nullptr;
  if (Mantid::JsonHelpers::parse(source, &root)) {
    const std::string shape = root["shape"].asString();
    if (shape == PeakShapeDetectorBin::detectorBinShapeName()) {
      const std::string algorithmName(root["algorithm_name"].asString());
      const int algorithmVersion(root["algorithm_version"].asInt());
      const auto frame(static_cast<SpecialCoordinateSystem>(root["frame"].asInt()));

      std::vector<std::tuple<int32_t, double, double>> detectorBinList;
      const Json::Value detectorList = root["detectors"];
      for (int index = 0; index < detectorList.size(); index++) {
        const Json::Value detBinVal = detectorList[index];
        detectorBinList.emplace_back(detBinVal["detId"].asInt(), detBinVal["startX"].asDouble(),
                                     detBinVal["endX"].asDouble());
      }
      peakShape = new PeakShapeDetectorBin(detectorBinList, frame, algorithmName, algorithmVersion);
    } else {
      if (m_successor) {
        peakShape = m_successor->create(source);
      } else {
        throw std::invalid_argument("PeakShapeDetectorBinFactory:: No successor "
                                    "factory able to process : " +
                                    source);
      }
    }
  } else {
    throw std::invalid_argument("PeakShapeDetectorBinFactory:: Source JSON for "
                                "the peak shape is not valid: " +
                                source);
  }
  return peakShape;
}

/**
 * @brief Set successor
 * @param successorFactory : successor
 */
void PeakShapeDetectorBinFactory::setSuccessor(std::shared_ptr<const PeakShapeFactory> successorFactory) {
  this->m_successor = successorFactory;
}

} // namespace Mantid::DataObjects
