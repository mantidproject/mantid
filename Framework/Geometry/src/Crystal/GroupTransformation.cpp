#include "MantidGeometry/Crystal/GroupTransformation.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidGeometry/Crystal/MatrixVectorPairParser.h"

#include <functional>

namespace Mantid {
namespace Geometry {

using namespace Kernel;

/// Constructor using SymmetryOperation.
GroupTransformation::GroupTransformation(
    const MatrixVectorPair<int, V3R> &operation)
    : m_symOp(operation) {}

/// Constructor using SymmetruOperation string, uses SymmetryOperationFactory.
GroupTransformation::GroupTransformation(const std::string &operationString) {
  MatrixVectorPairParser parser;
  m_symOp = parser.parse<int>(operationString);
}

/// Transforms the supplied group and returns the result.
Group GroupTransformation::operator()(const Group &other) const {
  std::vector<SymmetryOperation> groupOperations =
      other.getSymmetryOperations();

  std::vector<SymmetryOperation> transformedOperations;
  transformedOperations.reserve(groupOperations.size());

  using std::placeholders::_1;

  std::transform(groupOperations.begin(), groupOperations.end(),
                 std::back_inserter(transformedOperations),
                 std::bind(&GroupTransformation::transformOperation, this, _1));

  return Group(transformedOperations);
}

/// Returns the inverse transformation.
GroupTransformation GroupTransformation::getInverse() const {
  return GroupTransformation(m_symOp.getInverse());
}

/**
 * Transforms the operation using the internally stored SymmetryOperation.
 *
 * This method returns the transformed symmetry operation, using the
 * following relation:
 *
 *  S' = O^-1 * S * O
 *
 * Where O is the internally stored symmetry operation.
 *
 * @param operation :: SymmetryOperation to transform.
 * @return Transformed symmetry operation.
 */
SymmetryOperation GroupTransformation::transformOperation(
    const SymmetryOperation &operation) const {
  MatrixVectorPair<int, V3R> op =
      m_symOp.getInverse() *
      MatrixVectorPair<int, V3R>(operation.matrix(), operation.vector()) *
      m_symOp;
  return SymmetryOperation(op.getMatrix(), op.getVector());
}

} // namespace Geometry
} // namespace Mantid
