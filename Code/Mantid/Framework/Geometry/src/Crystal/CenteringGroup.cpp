#include "MantidGeometry/Crystal/CenteringGroup.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

#include <boost/assign.hpp>

namespace Mantid
{
namespace Geometry
{

/// String-based constructor which accepts centering symbols such as P, I or F.
CenteringGroup::CenteringGroup(const std::string &centeringSymbol) :
    Group(),
    m_type(),
    m_symbol()
{
    m_type = CenteringGroupCreationHelper::getCenteringType(centeringSymbol);
    m_symbol = centeringSymbol.substr(0, 1);

    setSymmetryOperations(CenteringGroupCreationHelper::getSymmetryOperations(m_type));
}

/// Returns the centering type of the group (distinguishes between Rhombohedral obverse and reverse).
CenteringGroup::CenteringType CenteringGroup::getType() const
{
    return m_type;
}

/// Returns the centering symbol, does not distinguish between Rhombohedral obverse and reverse.
std::string CenteringGroup::getSymbol() const
{
    return m_symbol;
}

/// Map between string symbols and enum-values for centering type.
std::map<std::string, CenteringGroup::CenteringType> CenteringGroupCreationHelper::m_centeringSymbolMap =
        boost::assign::map_list_of
        ("P", CenteringGroup::P)
        ("I", CenteringGroup::I)
        ("A", CenteringGroup::A)
        ("B", CenteringGroup::B)
        ("C", CenteringGroup::C)
        ("F", CenteringGroup::F)
        ("R", CenteringGroup::Robv)
        ("Robv", CenteringGroup::Robv)
        ("Rrev", CenteringGroup::Rrev);

/// Returns centering type enum value if centering symbol exists, throws std::invalid_argument exception otherwise.
CenteringGroup::CenteringType CenteringGroupCreationHelper::getCenteringType(const std::string &centeringSymbol)
{
    auto it = m_centeringSymbolMap.find(centeringSymbol);

    if(it == m_centeringSymbolMap.end()) {
        throw std::invalid_argument("Centering does not exist: " + centeringSymbol);
    }

    return m_centeringSymbolMap[centeringSymbol];
}

/// Returns a vector of symmetry operations for the given centering type or throws std::invalid_argument if an invalid value is supplied.
std::vector<SymmetryOperation> CenteringGroupCreationHelper::getSymmetryOperations(CenteringGroup::CenteringType centeringType)
{
    switch(centeringType) {
    case CenteringGroup::P:
        return getPrimitive();
    case CenteringGroup::I:
        return getBodyCentered();
    case CenteringGroup::A:
        return getACentered();
    case CenteringGroup::B:
        return getBCentered();
    case CenteringGroup::C:
        return getCCentered();
    case CenteringGroup::F:
        return getFCentered();
    case CenteringGroup::Robv:
        return getRobvCentered();
    case CenteringGroup::Rrev:
        return getRrevCentered();
    default:
        throw std::invalid_argument("Unknown centering type.");
    }
}

/// Returns symmetry operations for P-centering.
std::vector<SymmetryOperation> CenteringGroupCreationHelper::getPrimitive()
{
    return SymmetryOperationFactory::Instance().createSymOps("x,y,z");
}

/// Returns symmetry operations for I-centering.
std::vector<SymmetryOperation> CenteringGroupCreationHelper::getBodyCentered()
{
    return SymmetryOperationFactory::Instance().createSymOps("x,y,z; x+1/2,y+1/2,z+1/2");
}

/// Returns symmetry operations for A-centering.
std::vector<SymmetryOperation> CenteringGroupCreationHelper::getACentered()
{
    return SymmetryOperationFactory::Instance().createSymOps("x,y,z; x,y+1/2,z+1/2");
}

/// Returns symmetry operations for B-centering.
std::vector<SymmetryOperation> CenteringGroupCreationHelper::getBCentered()
{
    return SymmetryOperationFactory::Instance().createSymOps("x,y,z; x+1/2,y,z+1/2");
}

/// Returns symmetry operations for C-centering.
std::vector<SymmetryOperation> CenteringGroupCreationHelper::getCCentered()
{
    return SymmetryOperationFactory::Instance().createSymOps("x,y,z; x+1/2,y+1/2,z");
}

/// Returns symmetry operations for F-centering.
std::vector<SymmetryOperation> CenteringGroupCreationHelper::getFCentered()
{
    return SymmetryOperationFactory::Instance().createSymOps("x,y,z; x,y+1/2,z+1/2; x+1/2,y,z+1/2; x+1/2,y+1/2,z");
}

/// Returns symmetry operations for R-centering, obverse setting.
std::vector<SymmetryOperation> CenteringGroupCreationHelper::getRobvCentered()
{
    return SymmetryOperationFactory::Instance().createSymOps("x,y,z; x+1/3,y+2/3,z+2/3; x+2/3,y+1/3,z+1/3");
}

/// Returns symmetry operations for R-centering, reverse setting.
std::vector<SymmetryOperation> CenteringGroupCreationHelper::getRrevCentered()
{
    return SymmetryOperationFactory::Instance().createSymOps("x,y,z; x+1/3,y+2/3,z+1/3; x+2/3,y+1/3,z+2/3");
}

} // namespace Geometry
} // namespace Mantid
