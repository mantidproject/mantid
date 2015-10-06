#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"

namespace Mantid {
namespace Geometry {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
QLab::QLab() : m_unit(new Mantid::Kernel::InverseAngstromsUnit) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
QLab::~QLab() {}

const std::string QLab::QLabName = "QLab";

Kernel::UnitLabel QLab::getUnitLabel() const {
  // Forward request on
  return m_unit->getUnitLabel();
}

const Kernel::MDUnit &QLab::getMDUnit() const { return *m_unit; }

bool QLab::canConvertTo(const Mantid::Kernel::MDUnit &otherUnit) const {
  /*
   Inter frame conversion is possible, but requires additional information.
   Forbidden for time being.
  */
  return *this->m_unit == otherUnit;
}

std::string QLab::name() const { return QLab::QLabName; }

QLab *QLab::clone() const { return new QLab; }

} // namespace Geometry
} // namespace Mantid
