#include "MantidDataObjects/PeakShapeNone.h"
#include <stdexcept>
#include <jsoncpp/json/json.h>

namespace Mantid {
namespace DataObjects {

/**
 * @brief Constructor
 * @param peakCentre : Peak centre
 * @param frame : Coordinate frame
 * @param algorithmName : Algorithm name
 * @param algorithmVersion : Algorithm Version
 */
PeakShapeNone::PeakShapeNone(const Kernel::VMD &peakCentre,
                             API::SpecialCoordinateSystem frame,
                             std::string algorithmName, int algorithmVersion)
    : PeakShapeBase(peakCentre, frame, algorithmName, algorithmVersion) {}

/**
 * @brief Destructor
 */
PeakShapeNone::~PeakShapeNone() {}

/**
 * @brief Copy constructor
 * @param other : source of the copy
 */
PeakShapeNone::PeakShapeNone(const PeakShapeNone &other)
    : PeakShapeBase(other) {}

/**
 * @brief Assignment operator
 * @param other : source of the assignment
 * @return Ref to assigned object.
 */
PeakShapeNone &PeakShapeNone::operator=(const PeakShapeNone &other) {
  if (this != &other) {
    PeakShapeBase::operator=(other);
  }
  return *this;
}

/**
 * @brief Serialize to JSON object
 * @return JSON object as std::string
 */
std::string PeakShapeNone::toJSON() const {
  Json::Value root;
  PeakShapeBase::buildCommon(root);

  Json::StyledWriter writer;
  return writer.write(root);
}

/**
 * @brief Clone object as deep copy
 * @return pointer to new object
 */
PeakShapeNone *PeakShapeNone::clone() const { return new PeakShapeNone(*this); }

/**
 * @brief Return the unique shape name associated with this type
 * @return Shape name
 */
std::string PeakShapeNone::shapeName() const { return "none"; }

/**
 * @brief Overriden equals operator
 * @param other : To compare against
 * @return : True if equal otherwise false
 */
bool PeakShapeNone::operator==(const PeakShapeNone &other) const {
  return PeakShapeBase::operator==(other);
}

} // namespace DataObjects
} // namespace Mantid
