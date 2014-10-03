#include "MantidGeometry/Crystal/CyclicGroup.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include <boost/make_shared.hpp>

namespace Mantid
{
namespace Geometry
{

/// Construct cyclic group from one symmetry operation by applying it to itself until identity is obtained.
CyclicGroup::CyclicGroup(const SymmetryOperation &symmetryOperation) :
    Group(generateAllOperations(symmetryOperation))
{
}

Group_const_sptr CyclicGroup::create(const std::string &symmetryOperation)
{
    return boost::make_shared<const CyclicGroup>(SymmetryOperationFactory::Instance().createSymOp(symmetryOperation));
}

std::vector<SymmetryOperation> CyclicGroup::generateAllOperations(const SymmetryOperation &operation) const
{
    std::vector<SymmetryOperation> symOps(1, operation);

    for(size_t i = 1; i < operation.order(); ++i) {
        symOps.push_back(operation * symOps.back());
    }

    return symOps;
}



} // namespace Geometry
} // namespace Mantid
