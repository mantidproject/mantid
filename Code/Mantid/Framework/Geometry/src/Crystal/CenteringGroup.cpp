#include "MantidGeometry/Crystal/CenteringGroup.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

#include <boost/assign.hpp>

namespace Mantid {
namespace Geometry {

/// String-based constructor which accepts centering symbols such as P, I or F.
CenteringGroup::CenteringGroup(const std::string &centeringSymbol)
    : Group(), m_type(), m_symbol() {
  m_type = CenteringGroupCreator::Instance().getCenteringType(centeringSymbol);
  m_symbol = centeringSymbol.substr(0, 1);

  setSymmetryOperations(
      CenteringGroupCreator::Instance().getSymmetryOperations(m_type));
}

/// Returns the centering type of the group (distinguishes between Rhombohedral
/// obverse and reverse).
CenteringGroup::CenteringType CenteringGroup::getType() const { return m_type; }

/// Returns the centering symbol, does not distinguish between Rhombohedral
/// obverse and reverse.
std::string CenteringGroup::getSymbol() const { return m_symbol; }

/// Returns centering type enum value if centering symbol exists, throws
/// std::invalid_argument exception otherwise.
CenteringGroup::CenteringType CenteringGroupCreatorImpl::getCenteringType(
    const std::string &centeringSymbol) const {
  auto it = m_centeringSymbolMap.find(centeringSymbol);

  if (it == m_centeringSymbolMap.end()) {
    throw std::invalid_argument("Centering does not exist: " + centeringSymbol);
  }

  return it->second;
}

/// Returns a vector of symmetry operations for the given centering type or
/// throws std::invalid_argument if an invalid value is supplied.
std::vector<SymmetryOperation> CenteringGroupCreatorImpl::getSymmetryOperations(
    CenteringGroup::CenteringType centeringType) const {
  switch (centeringType) {
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
std::vector<SymmetryOperation> CenteringGroupCreatorImpl::getPrimitive() const {
  return SymmetryOperationFactory::Instance().createSymOps("x,y,z");
}

/// Returns symmetry operations for I-centering.
std::vector<SymmetryOperation>
CenteringGroupCreatorImpl::getBodyCentered() const {
  return SymmetryOperationFactory::Instance().createSymOps(
      "x,y,z; x+1/2,y+1/2,z+1/2");
}

/// Returns symmetry operations for A-centering.
std::vector<SymmetryOperation> CenteringGroupCreatorImpl::getACentered() const {
  return SymmetryOperationFactory::Instance().createSymOps(
      "x,y,z; x,y+1/2,z+1/2");
}

/// Returns symmetry operations for B-centering.
std::vector<SymmetryOperation> CenteringGroupCreatorImpl::getBCentered() const {
  return SymmetryOperationFactory::Instance().createSymOps(
      "x,y,z; x+1/2,y,z+1/2");
}

/// Returns symmetry operations for C-centering.
std::vector<SymmetryOperation> CenteringGroupCreatorImpl::getCCentered() const {
  return SymmetryOperationFactory::Instance().createSymOps(
      "x,y,z; x+1/2,y+1/2,z");
}

/// Returns symmetry operations for F-centering.
std::vector<SymmetryOperation> CenteringGroupCreatorImpl::getFCentered() const {
  return SymmetryOperationFactory::Instance().createSymOps(
      "x,y,z; x,y+1/2,z+1/2; x+1/2,y,z+1/2; x+1/2,y+1/2,z");
}

/// Returns symmetry operations for R-centering, obverse setting.
std::vector<SymmetryOperation>
CenteringGroupCreatorImpl::getRobvCentered() const {
  return SymmetryOperationFactory::Instance().createSymOps(
      "x,y,z; x+1/3,y+2/3,z+2/3; x+2/3,y+1/3,z+1/3");
}

/// Returns symmetry operations for R-centering, reverse setting.
std::vector<SymmetryOperation>
CenteringGroupCreatorImpl::getRrevCentered() const {
  return SymmetryOperationFactory::Instance().createSymOps(
      "x,y,z; x+1/3,y+2/3,z+1/3; x+2/3,y+1/3,z+2/3");
}

CenteringGroupCreatorImpl::CenteringGroupCreatorImpl()
    : m_centeringSymbolMap() {
  m_centeringSymbolMap.insert(std::make_pair("P", CenteringGroup::P));
  m_centeringSymbolMap.insert(std::make_pair("I", CenteringGroup::I));
  m_centeringSymbolMap.insert(std::make_pair("A", CenteringGroup::A));
  m_centeringSymbolMap.insert(std::make_pair("B", CenteringGroup::B));
  m_centeringSymbolMap.insert(std::make_pair("C", CenteringGroup::C));
  m_centeringSymbolMap.insert(std::make_pair("F", CenteringGroup::F));
  m_centeringSymbolMap.insert(std::make_pair("R", CenteringGroup::Robv));
  m_centeringSymbolMap.insert(std::make_pair("Robv", CenteringGroup::Robv));
  m_centeringSymbolMap.insert(std::make_pair("Rrev", CenteringGroup::Rrev));
}

} // namespace Geometry
} // namespace Mantid
