#include "MantidDataObjects/PeakShapeBase.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include <json/json.h>

namespace Mantid {
namespace DataObjects {

PeakShapeBase::PeakShapeBase(Kernel::SpecialCoordinateSystem frame,
                             std::string algorithmName, int algorithmVersion)
    : m_frame(frame), m_algorithmName(algorithmName),
      m_algorithmVersion(algorithmVersion) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PeakShapeBase::~PeakShapeBase() {}

/**
 * @brief Copy constructor
 * @param other : source of the copy
 */
PeakShapeBase::PeakShapeBase(const PeakShapeBase &other): m_frame(other.frame()),
      m_algorithmName(other.algorithmName()),
      m_algorithmVersion(other.algorithmVersion()) {}

/**
 * @brief Assignment operator
 * @param other : source of the assignment
 * @return Ref to assigned object.
 */
PeakShapeBase &PeakShapeBase::operator=(const PeakShapeBase &other) {
  if (this != &other) {
    m_algorithmName = other.algorithmName();
    m_algorithmVersion = other.algorithmVersion();
    m_frame = other.frame();
  }
  return *this;
}

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
  Json::Value shape(this->shapeName());
  Json::Value algorithmName(this->algorithmName());
  Json::Value algorithmVersion(this->algorithmVersion());
  Json::Value frame(this->frame());
  root["shape"] = shape;
  root["algorithm_name"] = algorithmName;
  root["algorithm_version"] = algorithmVersion;
  root["frame"] = frame;
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

bool PeakShapeBase::operator==(const PeakShapeBase &other) const {
  return other.frame() == this->frame();
}


} // namespace DataObjects
} // namespace Mantid
