#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/Exception.h"

#include <boost/make_shared.hpp>

namespace Mantid
{
namespace Geometry
{

/// Creates a SymmetryOperation object from its identifier.
SymmetryOperation SymmetryOperationFactoryImpl::createSymOp(const std::string &identifier)
{
    if(!isSubscribed(identifier)) {
        subscribeSymOp(identifier);
    }

    return SymmetryOperation(m_prototypes[identifier]);
}

/// Subscribes a symmetry operation into the factory
void SymmetryOperationFactoryImpl::subscribeSymOp(const std::string &identifier)
{
    SymmetryOperation prototype(identifier);

    subscribe(identifier, prototype);
}

/// Unsubscribes a symmetry operation from the factory
void SymmetryOperationFactoryImpl::unsubscribeSymOp(const std::string &identifier)
{
    if(isSubscribed(identifier)) {
        m_prototypes.erase(identifier);
    }
}

/// Returns true if identifier already has a prototype in the factory.
bool SymmetryOperationFactoryImpl::isSubscribed(const std::string &identifier) const
{
    return m_prototypes.find(identifier) != m_prototypes.end();
}

/// Returns all symbols in the factory.
std::vector<std::string> SymmetryOperationFactoryImpl::subscribedSymbols() const
{
    std::vector<std::string> symbols;
    symbols.reserve(m_prototypes.size());

    for(auto it = m_prototypes.begin(); it != m_prototypes.end(); ++it) {
        symbols.push_back(it->first);
    }

    return symbols;
}

/// Subscribes symmetry operation into factory, using the supplied alias as key.
void SymmetryOperationFactoryImpl::subscribe(const std::string &alias, const SymmetryOperation &prototype)
{
    if(!isSubscribed(alias)) {
        m_prototypes.insert(std::make_pair(alias, prototype));
    }
}

/// Private default constructor.
SymmetryOperationFactoryImpl::SymmetryOperationFactoryImpl() :
    m_prototypes()
{
    Kernel::LibraryManager::Instance();
}

} // namespace Geometry
} // namespace Mantid
