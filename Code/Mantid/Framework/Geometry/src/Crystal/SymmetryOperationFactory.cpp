#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidKernel/LibraryManager.h"

namespace Mantid
{
namespace Geometry
{

/// Creates a SymmetryOperation object from its identifier.
SymmetryOperation_const_sptr SymmetryOperationFactoryImpl::createSymOp(const std::string &identifier) const
{
    return create(identifier);
}

/// Private default constructor.
SymmetryOperationFactoryImpl::SymmetryOperationFactoryImpl() : Kernel::DynamicFactory<const SymmetryOperation>()
{
    Kernel::LibraryManager::Instance();
}

} // namespace Geometry
} // namespace Mantid
