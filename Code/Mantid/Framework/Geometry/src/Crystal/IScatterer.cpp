#include "MantidGeometry/Crystal/IScatterer.h"
#include <stdexcept>

#include <boost/regex.hpp>
#include <boost/make_shared.hpp>
#include "MantidKernel/ListValidator.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

namespace Mantid
{
namespace Geometry
{

using namespace Kernel;

/// Default constructor.
IScatterer::IScatterer() :
    PropertyManager(),
    m_position(),
    m_cell(UnitCell(1, 1, 1, 90, 90, 90)),
    m_spaceGroup(),
    m_isInitialized(false)
{
    recalculateEquivalentPositions();
}

/// Initialization method that declares three basic properties and sets initialized state to true.
void IScatterer::initialize()
{
    /* This is required for default behavior. It's not possible to call it
     * from the constructure, because it's not guaranteed that the space group
     * factory has been filled at the time the ScattererFactory is filled.
     */
    setSpaceGroup(SpaceGroupFactory::Instance().createSpaceGroup("P 1"));

    declareProperty(new Kernel::PropertyWithValue<V3D>("Position", V3D(0.0, 0.0, 0.0)), "Position of the scatterer");

    IValidator_sptr unitCellStringValidator = boost::make_shared<UnitCellStringValidator>();
    declareProperty(new Kernel::PropertyWithValue<std::string>("UnitCell", "1.0 1.0 1.0 90.0 90.0 90.0", unitCellStringValidator), "Unit cell.");

    IValidator_sptr spaceGroupValidator = boost::make_shared<ListValidator<std::string> >(SpaceGroupFactory::Instance().subscribedSpaceGroupSymbols());
    declareProperty(new Kernel::PropertyWithValue<std::string>("SpaceGroup", "P 1", spaceGroupValidator), "Space group.");

    declareProperties();

    m_isInitialized = true;
}

/// Returns whether the instance has been initialized.
bool IScatterer::isInitialized()
{
    return m_isInitialized;
}

/// Sets the position of the scatterer to the supplied coordinates - vector is wrapped to [0, 1) and equivalent positions are recalculated.
void IScatterer::setPosition(const Kernel::V3D &position)
{
    m_position = getWrappedVector(position);

    recalculateEquivalentPositions();
}

/// Returns the position of the scatterer.
Kernel::V3D IScatterer::getPosition() const
{
    return m_position;
}

/// Returns all equivalent positions of the scatterer according to the assigned space group.
std::vector<Kernel::V3D> IScatterer::getEquivalentPositions() const
{
    return m_equivalentPositions;
}

/// Assigns a unit cell, which may be required for certain calculations.
void IScatterer::setCell(const UnitCell &cell)
{
    m_cell = cell;
}

/// Returns the cell which is currently set.
UnitCell IScatterer::getCell() const
{
    return m_cell;
}

/// Sets the space group, which is required for calculation of equivalent positions.
void IScatterer::setSpaceGroup(const SpaceGroup_const_sptr &spaceGroup)
{
    m_spaceGroup = spaceGroup;

    recalculateEquivalentPositions();
}

/**
 * Additional property processing
 *
 * Takes care of handling new property values, for example for construction of a space group
 * from string and so on.
 *
 * Please note that derived classes should not re-implement this method, as
 * the processing for the base properties is absolutely necessary. Instead, all deriving
 * classes should override the method afterScattererPropertySet, which is called
 * from this method.
 */
void IScatterer::afterPropertySet(const std::string &propertyName)
{
    if(propertyName == "Position") {
        PropertyWithValue<V3D> *position = dynamic_cast<PropertyWithValue<V3D> *>(getPointerToProperty("Position"));
        setPosition((*position)());
    } else if(propertyName == "SpaceGroup") {
        setSpaceGroup(SpaceGroupFactory::Instance().createSpaceGroup(getProperty("SpaceGroup")));
    } else if(propertyName == "UnitCell") {
        setCell(strToUnitCell(getProperty("UnitCell")));
    }

    afterScattererPropertySet(propertyName);
}

/// Returns the assigned space group.
SpaceGroup_const_sptr IScatterer::getSpaceGroup() const
{
    return m_spaceGroup;
}

/// Uses the stored space group to calculate all equivalent positions or if present.
void IScatterer::recalculateEquivalentPositions()
{
    m_equivalentPositions.clear();

    if(m_spaceGroup) {
        m_equivalentPositions = m_spaceGroup * m_position;
    } else {
        m_equivalentPositions.push_back(m_position);
    }
}

/// Return a clone of the validator.
IValidator_sptr UnitCellStringValidator::clone() const
{
    return boost::make_shared<UnitCellStringValidator>(*this);
}

/// Check if the string is valid input for Geometry::strToUnitCell.
std::string UnitCellStringValidator::checkValidity(const std::string &unitCellString) const
{
    boost::regex unitCellRegex("((\\d+(\\.\\d+){0,1}\\s+){2}|(\\d+(\\.\\d+){0,1}\\s+){5})(\\d+(\\.\\d+){0,1}\\s*)");

    if(!boost::regex_match(unitCellString, unitCellRegex)) {
        return "Unit cell string is invalid: " + unitCellString;
    }

    return "";
}



} // namespace Geometry
} // namespace Mantid
