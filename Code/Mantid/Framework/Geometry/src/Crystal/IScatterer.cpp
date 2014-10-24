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

/// Default constructor, vector is wrapped to interval [0, 1).
IScatterer::IScatterer(const Kernel::V3D &position) :
    PropertyManager(),
    m_position(getWrappedVector(position)),
    m_cell(),
    m_spaceGroup()
{
    recalculateEquivalentPositions();
}

void IScatterer::initialize()
{
    declareProperty(new Kernel::PropertyWithValue<V3D>("Position", V3D(0.0, 0.0, 0.0)), "Position of the scatterer");

    IValidator_sptr unitCellStringValidator = boost::make_shared<UnitCellStringValidator>();
    declareProperty(new Kernel::PropertyWithValue<std::string>("UnitCell", "1.0 1.0 1.0 90.0 90.0 90.0", unitCellStringValidator), "Unit cell.");

    IValidator_sptr spaceGroupValidator = boost::make_shared<ListValidator<std::string> >(SpaceGroupFactory::Instance().subscribedSpaceGroupSymbols());
    declareProperty(new Kernel::PropertyWithValue<std::string>("SpaceGroup", "P 1", spaceGroupValidator), "Space group.");

    declareProperties();
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

IValidator_sptr UnitCellStringValidator::clone() const
{
    return boost::make_shared<UnitCellStringValidator>(*this);
}

std::string UnitCellStringValidator::checkValidity(const std::string &unitCellString) const
{
    boost::regex unitCellRegex("((\\d+\\.\\d+\\s+){2}|(\\d+\\.\\d+\\s+){5})(\\d+\\.\\d+\\s*)");

    if(!boost::regex_match(unitCellString, unitCellRegex)) {
        return "Unit cell string is invalid.";
    }

    return "";
}



} // namespace Geometry
} // namespace Mantid
