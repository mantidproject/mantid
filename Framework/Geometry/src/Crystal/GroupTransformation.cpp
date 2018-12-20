#include "MantidGeometry/Crystal/GroupTransformation.h"
#include "MantidGeometry/Crystal/MatrixVectorPairParser.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

#include <functional>

namespace Mantid {
namespace Geometry {

using namespace Kernel;

/// Constructor using MatrixVectorPair.
GroupTransformation::GroupTransformation(
    const MatrixVectorPair<double, V3R> &operation)
    : m_matrixVectorPair(operation) {
  setInverseFromPair();
}

/// Constructor that parses a string to construct a MatrixVectorPair.
GroupTransformation::GroupTransformation(const std::string &operationString)
    : GroupTransformation(parseMatrixVectorPair<double>(operationString)) {}

/// Transforms the supplied group and returns the result.
Group GroupTransformation::operator()(const Group &other) const {
  std::vector<SymmetryOperation> groupOperations =
      other.getSymmetryOperations();

  std::vector<SymmetryOperation> transformedOperations;
  transformedOperations.reserve(groupOperations.size());

  using std::placeholders::_1;

  std::transform(groupOperations.cbegin(), groupOperations.cend(),
                 std::back_inserter(transformedOperations),
                 std::bind(&GroupTransformation::transformOperation, this, _1));

  return Group(transformedOperations);
}

/// Returns the inverse transformation.
GroupTransformation GroupTransformation::getInverse() const {
  return GroupTransformation(m_matrixVectorPair.getInverse());
}

/**
 * Transforms the operation using the internally stored SymmetryOperation.
 *
 * This method returns the transformed symmetry operation, using the
 * following relation:
 *
 *  S' = O^-1 * S * O
 *
 * Where O is the internally stored operation.
 *
 * @param operation :: SymmetryOperation to transform.
 * @return Transformed symmetry operation.
 */
SymmetryOperation GroupTransformation::transformOperation(
    const SymmetryOperation &operation) const {
  MatrixVectorPair<double, V3R> op =
      m_inversePair *
      MatrixVectorPair<double, V3R>(convertMatrix<double>(operation.matrix()),
                                    operation.vector()) *
      m_matrixVectorPair;

  return SymmetryOperation(op.getMatrix(), getWrappedVector(op.getVector()));
}

void GroupTransformation::setInverseFromPair() {
  m_inversePair = m_matrixVectorPair.getInverse();
}

} // namespace Geometry
} // namespace Mantid
