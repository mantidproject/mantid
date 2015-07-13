#include "MantidKernel/MDUnit.h"
#include "MantidKernel/UnitLabelTypes.h"

namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MDUnit::MDUnit() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MDUnit::~MDUnit() {}

//----------------------------------------------------------------------------------------------
// QUnit
//----------------------------------------------------------------------------------------------
QUnit::~QUnit()
{

}

bool QUnit::isQUnit() const
{
    return true;
}

//----------------------------------------------------------------------------------------------
// End QUnit
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
// RLU
//----------------------------------------------------------------------------------------------
UnitLabel ReciprocalLatticeUnit::getUnitLabel() const
{
    return Units::Symbol::RLU;
}

bool ReciprocalLatticeUnit::canConvertTo(MDUnit &other) const
{
    return other.isQUnit();
}

ReciprocalLatticeUnit::~ReciprocalLatticeUnit()
{

}
//----------------------------------------------------------------------------------------------
// End RLU
//----------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------
// Inverse Angstrom Unit
//----------------------------------------------------------------------------------------------

UnitLabel InverseAngstromsUnit::getUnitLabel() const
{
    return Units::Symbol::InverseAngstrom;
}

bool InverseAngstromsUnit::canConvertTo(MDUnit &other) const
{
    return other.isQUnit();
}

InverseAngstromsUnit::~InverseAngstromsUnit()
{

}

//----------------------------------------------------------------------------------------------
// Inverse Angstrom Unit
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
//  LabelUnit
//----------------------------------------------------------------------------------------------

LabelUnit::LabelUnit(const std::string& unitLabel) : m_unitLabel(unitLabel) {

}

UnitLabel LabelUnit::getUnitLabel() const
{
    return UnitLabel(m_unitLabel);
}

bool LabelUnit::canConvertTo(MDUnit &other) const
{
    return this->getUnitLabel() == other.getUnitLabel();
}

bool LabelUnit::isQUnit() const
{
    return false;
}

LabelUnit::~LabelUnit()
{

}

//----------------------------------------------------------------------------------------------
// End RLU
//----------------------------------------------------------------------------------------------


} // namespace Kernel
} // namespace Mantid
