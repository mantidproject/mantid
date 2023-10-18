// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/PeakShapeBase.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include <json/json.h>

#include <utility>

namespace Mantid::DataObjects {

PeakShapeBase::PeakShapeBase(Kernel::SpecialCoordinateSystem frame, std::string algorithmName, int algorithmVersion)
    : m_frame(frame), m_algorithmName(std::move(algorithmName)), m_algorithmVersion(algorithmVersion) {}

/**
 * @brief PeakShapeBase::frame
 * @return The coordinate frame used
 */
Kernel::SpecialCoordinateSystem PeakShapeBase::frame() const { return m_frame; }

/**
 * @brief PeakShapeBase::buildCommon. Serialize to JSON object and return the
 * JSON value for further modification
 * @param root : Ref to root value to write to
 */
void PeakShapeBase::buildCommon(Json::Value &root) const {
  Json::Value shapeJson(this->shapeName());
  Json::Value algorithmNameJson(this->algorithmName());
  Json::Value algorithmVersionJson(this->algorithmVersion());
  Json::Value frameJson(this->frame());
  root["shape"] = shapeJson;
  root["algorithm_name"] = algorithmNameJson;
  root["algorithm_version"] = algorithmVersionJson;
  root["frame"] = frameJson;
}

/**
 * @brief Get the algorithm name
 * @return Algorithm name
 */
std::string PeakShapeBase::algorithmName() const { return m_algorithmName; }

/**
 * @brief Get the algorithmVersion
 * @return Algorithm version
 */
int PeakShapeBase::algorithmVersion() const { return m_algorithmVersion; }

bool PeakShapeBase::operator==(const PeakShapeBase &other) const { return other.frame() == this->frame(); }

} // namespace Mantid::DataObjects
