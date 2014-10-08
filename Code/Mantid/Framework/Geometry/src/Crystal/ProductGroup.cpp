#include "MantidGeometry/Crystal/ProductGroup.h"

#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidGeometry/Crystal/CyclicGroup.h"

namespace Mantid
{
namespace Geometry
{

/// String constructor with semicolon-separated symmetry operations
ProductGroup::ProductGroup(const std::string &generators) :
    Group(*(getGeneratedGroup(generators)))
{
}

/// Constructor which directly takes a list of factor groups to form the product
ProductGroup::ProductGroup(const std::vector<Group_const_sptr> &factorGroups) :
    Group(*(getProductGroup(factorGroups)))
{
}

/// Generates symmetry operations from the string, creates a CyclicGroup from each operation and multiplies them to form a factor group.
Group_const_sptr ProductGroup::getGeneratedGroup(const std::string &generators) const
{
    std::vector<SymmetryOperation> operations = SymmetryOperationFactory::Instance().createSymOps(generators);
    std::vector<Group_const_sptr> factorGroups = getFactorGroups(operations);

    return getProductGroup(factorGroups);
}

/// Returns a vector of cyclic groups for the given vector of symmetry operations
std::vector<Group_const_sptr> ProductGroup::getFactorGroups(const std::vector<SymmetryOperation> &symmetryOperations) const
{
    std::vector<Group_const_sptr> groups;

    for(auto it = symmetryOperations.begin(); it != symmetryOperations.end(); ++it) {
        groups.push_back(GroupFactory::create<CyclicGroup>((*it).identifier()));
    }

    return groups;
}

/// Multiplies all supplied groups and returns the result
Group_const_sptr ProductGroup::getProductGroup(const std::vector<Group_const_sptr> &factorGroups) const
{
    Group_const_sptr productGroup = boost::make_shared<const Group>(*(factorGroups.front()));

    for(size_t i = 1; i < factorGroups.size(); ++i) {
        productGroup = productGroup * factorGroups[i];
    }

    return productGroup;
}


} // namespace Geometry
} // namespace Mantid
