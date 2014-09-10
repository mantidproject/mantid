#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidKernel/LibraryManager.h"

namespace Mantid
{
namespace Geometry
{

/// Creates a SymmetryOperation object from its Hermann-Mauguin symbol.
SymmetryOperation_const_sptr SymmetryOperationFactoryImpl::createSymOp(const std::string &identifier) const
{
    return create(identifier);
}

/// Returns the Hermann-Mauguin symbols of all registered point groups.
const std::list<std::string> &SymmetryOperationFactoryImpl::getAllSymOpIdentifiers() const
{
    return m_availableSymOps;
}

/// Private default constructor.
SymmetryOperationFactoryImpl::SymmetryOperationFactoryImpl() : Kernel::DynamicFactory<const SymmetryOperation>(),
    m_availableSymOps()
{
    Kernel::LibraryManager::Instance();
}

/// Adds a point group to a map that stores pairs of Hermann-Mauguin symbol and crystal system.
void SymmetryOperationFactoryImpl::addToAvailable(const std::string &identifier)
{
    m_availableSymOps.insert(m_availableSymOps.end(), identifier);
}

/// Removes point group from internal crystal system map.
void SymmetryOperationFactoryImpl::removeFromAvailable(const std::string &identifier)
{
    m_availableSymOps.remove(identifier);
}

} // namespace Geometry
} // namespace Mantid
