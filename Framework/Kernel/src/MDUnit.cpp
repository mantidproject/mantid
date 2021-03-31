// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/UnitLabelTypes.h"
#include <boost/regex.hpp>

namespace Mantid {
namespace Kernel {

bool MDUnit::operator==(const MDUnit &other) const {
  return typeid(*this) == typeid(other) && this->canConvertTo(other);
}

//----------------------------------------------------------------------------------------------
// QUnit
//----------------------------------------------------------------------------------------------

bool QUnit::isQUnit() const { return true; }

//----------------------------------------------------------------------------------------------
// End QUnit
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
// RLU
//----------------------------------------------------------------------------------------------
ReciprocalLatticeUnit::ReciprocalLatticeUnit() : m_unitLabel(UnitLabel("")) {}

ReciprocalLatticeUnit::ReciprocalLatticeUnit(const UnitLabel &unitLabel) : m_unitLabel(unitLabel) {}

UnitLabel ReciprocalLatticeUnit::getUnitLabel() const {
  if (isSpecialRLUUnitLabel()) {
    return m_unitLabel;
  } else {
    return Units::Symbol::RLU;
  }
}

bool ReciprocalLatticeUnit::canConvertTo(const MDUnit &other) const { return other.isQUnit(); }

ReciprocalLatticeUnit *ReciprocalLatticeUnit::clone() const {
  if (isSpecialRLUUnitLabel()) {
    return new ReciprocalLatticeUnit(m_unitLabel);
  } else {
    return new ReciprocalLatticeUnit;
  }
}

bool ReciprocalLatticeUnit::isSpecialRLUUnitLabel() const {
  boost::regex pattern("in.*A.*\\^-1");
  return boost::regex_match(m_unitLabel.ascii(), pattern);
}

//----------------------------------------------------------------------------------------------
// End RLU
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
// Inverse Angstrom Unit
//----------------------------------------------------------------------------------------------

UnitLabel InverseAngstromsUnit::getUnitLabel() const { return Units::Symbol::InverseAngstrom; }

bool InverseAngstromsUnit::canConvertTo(const MDUnit &other) const { return other.isQUnit(); }

InverseAngstromsUnit *InverseAngstromsUnit::clone() const { return new InverseAngstromsUnit; }

//----------------------------------------------------------------------------------------------
// Inverse Angstrom Unit
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
//  LabelUnit
//----------------------------------------------------------------------------------------------

LabelUnit::LabelUnit(const UnitLabel &unitLabel) : m_unitLabel(unitLabel) {}

UnitLabel LabelUnit::getUnitLabel() const { return m_unitLabel; }

bool LabelUnit::canConvertTo(const MDUnit &other) const { return this->getUnitLabel() == other.getUnitLabel(); }

bool LabelUnit::isQUnit() const {
  boost::regex pattern("(A\\^-1)");
  boost::smatch match; // Unused.
  return boost::regex_search(m_unitLabel.ascii(), match, pattern);
}

LabelUnit *LabelUnit::clone() const { return new LabelUnit(m_unitLabel); }

//----------------------------------------------------------------------------------------------
// End RLU
//----------------------------------------------------------------------------------------------

} // namespace Kernel
} // namespace Mantid
