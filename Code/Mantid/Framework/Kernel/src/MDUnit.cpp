#include "MantidKernel/MDUnit.h"
#include "MantidKernel/UnitLabelTypes.h"
#include <boost/regex.hpp>


namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MDUnit::MDUnit() {}

bool MDUnit::operator==(const MDUnit &other) const {
  return typeid(*this) == typeid(other) && this->canConvertTo(other);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MDUnit::~MDUnit() {}

//----------------------------------------------------------------------------------------------
// QUnit
//----------------------------------------------------------------------------------------------
QUnit::~QUnit() {}

bool QUnit::isQUnit() const { return true; }

//----------------------------------------------------------------------------------------------
// End QUnit
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
// RLU
//----------------------------------------------------------------------------------------------
UnitLabel ReciprocalLatticeUnit::getUnitLabel() const {
  return Units::Symbol::RLU;
}

bool ReciprocalLatticeUnit::canConvertTo(const MDUnit &other) const {
  return other.isQUnit();
}

ReciprocalLatticeUnit *ReciprocalLatticeUnit::clone() const {
  return new ReciprocalLatticeUnit;
}

ReciprocalLatticeUnit::~ReciprocalLatticeUnit() {}
//----------------------------------------------------------------------------------------------
// End RLU
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
// Inverse Angstrom Unit
//----------------------------------------------------------------------------------------------

UnitLabel InverseAngstromsUnit::getUnitLabel() const {
  return Units::Symbol::InverseAngstrom;
}

bool InverseAngstromsUnit::canConvertTo(const MDUnit &other) const {
  return other.isQUnit();
}

InverseAngstromsUnit::~InverseAngstromsUnit() {}

InverseAngstromsUnit *InverseAngstromsUnit::clone() const {
  return new InverseAngstromsUnit;
}

//----------------------------------------------------------------------------------------------
// Inverse Angstrom Unit
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
//  LabelUnit
//----------------------------------------------------------------------------------------------

LabelUnit::LabelUnit(const UnitLabel &unitLabel) : m_unitLabel(unitLabel) {}

UnitLabel LabelUnit::getUnitLabel() const { return m_unitLabel; }

bool LabelUnit::canConvertTo(const MDUnit &other) const {
  return this->getUnitLabel() == other.getUnitLabel();
}

bool LabelUnit::isQUnit() const {
  boost::regex pattern("(A\\^-1)");
  boost::smatch match; // Unused.
  return boost::regex_search(m_unitLabel.ascii(), match, pattern);
}

LabelUnit::~LabelUnit() {}

LabelUnit *LabelUnit::clone() const { return new LabelUnit(m_unitLabel); }

//----------------------------------------------------------------------------------------------
// End RLU
//----------------------------------------------------------------------------------------------

} // namespace Kernel
} // namespace Mantid
