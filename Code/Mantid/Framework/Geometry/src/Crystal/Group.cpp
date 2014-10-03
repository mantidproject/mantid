#include "MantidGeometry/Crystal/Group.h"
#include <boost/make_shared.hpp>

namespace Mantid
{
namespace Geometry
{

int Group::m_numOps = 0;

/// Construct a group group from a given set of operations. This is
Group::Group() :
    m_allOperations(),
    m_operationSet()
{
    std::vector<SymmetryOperation> operation(1);
    setSymmetryOperations(operation);
}

Group::Group(const std::vector<SymmetryOperation> &symmetryOperations) :
    m_allOperations(),
    m_operationSet()
{
    setSymmetryOperations(symmetryOperations);
}

Group::Group(const Group &other) :
    m_allOperations(other.m_allOperations),
    m_operationSet(other.m_operationSet)
{

}

Group &Group::operator =(const Group &other)
{
    m_allOperations = other.m_allOperations;
    m_operationSet = other.m_operationSet;

    return *this;
}

size_t Group::order() const
{
    return m_allOperations.size();
}

std::vector<SymmetryOperation> Group::getSymmetryOperations() const
{
    return m_allOperations;
}

Group Group::operator *(const Group &other) const
{
    std::vector<SymmetryOperation> result;
    result.reserve(order() * other.order());

    for(auto selfOp = m_allOperations.begin(); selfOp != m_allOperations.end(); ++selfOp) {
        for(auto otherOp = other.m_allOperations.begin(); otherOp != other.m_allOperations.end(); ++otherOp) {
            result.push_back((*selfOp) * (*otherOp));
            ++m_numOps;
        }
    }

    return Group(result);
}

std::vector<Kernel::V3D> Group::operator *(const Kernel::V3D &vector) const
{
    std::set<Kernel::V3D> result;

    for(auto op = m_allOperations.begin(); op != m_allOperations.end(); ++op) {
        result.insert(Geometry::getWrappedVector((*op) * vector));
    }

    return std::vector<Kernel::V3D>(result.begin(), result.end());
}

bool Group::operator ==(const Group &other) const
{
    return m_operationSet == other.m_operationSet;
}

bool Group::operator !=(const Group &other) const
{
    return !(this->operator ==(other));
}

void Group::setSymmetryOperations(const std::vector<SymmetryOperation> &symmetryOperations)
{
    if(symmetryOperations.size() < 1) {
        throw std::invalid_argument("Group needs at least one element.");
    }

    m_operationSet = std::set<SymmetryOperation>(symmetryOperations.begin(), symmetryOperations.end());
    m_allOperations = std::vector<SymmetryOperation>(m_operationSet.begin(), m_operationSet.end());
}

Group_const_sptr operator *(const Group_const_sptr &lhs, const Group_const_sptr &rhs)
{
    return boost::make_shared<const Group>((*lhs) * (*rhs));
}

std::vector<Kernel::V3D> operator *(const Group_const_sptr &lhs, const Kernel::V3D &rhs)
{
    return (*lhs) * rhs;
}


bool operator ==(const Group_const_sptr &lhs, const Group_const_sptr &rhs)
{
    return (*lhs) == (*rhs);
}

bool operator !=(const Group_const_sptr &lhs, const Group_const_sptr &rhs)
{
    return !(operator ==(lhs, rhs));
}


} // namespace Geometry
} // namespace Mantid
