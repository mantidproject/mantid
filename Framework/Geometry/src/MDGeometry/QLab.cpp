// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"

namespace Mantid {
namespace Geometry {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
QLab::QLab() : m_unit(new Mantid::Kernel::InverseAngstromsUnit) {}

const std::string QLab::QLabName = "QLab";

Kernel::UnitLabel QLab::getUnitLabel() const {
  // Forward request on
  return m_unit->getUnitLabel();
}

const Kernel::MDUnit &QLab::getMDUnit() const { return *m_unit; }

bool QLab::setMDUnit(const Mantid::Kernel::MDUnit & /*newUnit*/) {
  return false;
}

bool QLab::canConvertTo(const Mantid::Kernel::MDUnit &otherUnit) const {
  /*
   Inter frame conversion is possible, but requires additional information.
   Forbidden for time being.
  */
  return *this->m_unit == otherUnit;
}

std::string QLab::name() const { return QLab::QLabName; }

QLab *QLab::clone() const { return new QLab; }

Mantid::Kernel::SpecialCoordinateSystem
QLab::equivalientSpecialCoordinateSystem() const {
  return Mantid::Kernel::SpecialCoordinateSystem::QLab;
}

bool QLab::isQ() const { return true; }

bool QLab::isSameType(const MDFrame &frame) const {
  auto isSameType = true;
  try {
    const auto &tmp = dynamic_cast<const QLab &>(frame);
    UNUSED_ARG(tmp);
  } catch (std::bad_cast &) {
    isSameType = false;
  }
  return isSameType;
}

} // namespace Geometry
} // namespace Mantid
