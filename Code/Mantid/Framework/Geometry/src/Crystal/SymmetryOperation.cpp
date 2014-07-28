#include "MantidGeometry/Crystal/SymmetryOperation.h"

namespace Mantid
{
namespace Geometry
{

// SymmertryOperation - base class
SymmetryOperation::SymmetryOperation(size_t order, Kernel::IntMatrix matrix) :
    m_order(order),
    m_matrix(matrix)
{
}

size_t SymmetryOperation::order() const
{
    return m_order;
}

// Identity
SymOpIdentity::SymOpIdentity() :
    SymmetryOperation(1, Kernel::IntMatrix(3, 3, true))
{

}

SymOpInversion::SymOpInversion() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3, true))
{
    m_matrix *= -1;
}

} // namespace Geometry
} // namespace Mantid
