#include "MantidGeometry/Crystal/SymmetryOperationSymbolParser.h"

#include "MantidGeometry/Crystal/MatrixVectorPairParser.h"
#include "MantidKernel/Exception.h"

#include <boost/algorithm/string.hpp>
#include <strstream>

namespace Mantid {
namespace Geometry {

/// Verify that the matrix does not contain elements with abs(element) > 1 and
/// has an acceptable number of non-zero elements.
void SymmetryOperationSymbolParser::verifyMatrix(
    const Kernel::IntMatrix &matrix) {
  for (size_t i = 0; i < matrix.numRows(); ++i) {
    if (!isValidMatrixRow(matrix[i], matrix.numCols())) {
      std::ostringstream error;
      error << "Matrix row " << i << " is invalid. Elements: [" << matrix[i][0]
            << ", " << matrix[i][1] << ", " << matrix[i][2] << "]";
      throw Kernel::Exception::ParseError(error.str(), "", 0);
    }
  }
}

/**
 * Tries to parse the given symbol
 *
 * This method tries to parse a given symbol and returns the matrix/vector pair
 * resulting from the parsing process. It takes a string representing a
 * symmetry operation in the format:
 *      x+a/b, -y-c/d, e/f-z
 * where x, y and z are the literals 'x', 'y' and 'z', while a-f are integers,
 * representing rational numbers. The latter don't need to be present,
 * a string "x,y,z" is valid. Leading plus-signs may be included if desired,
 * so that "+x,+y,+z" is also valid.
 *
 * If there is a problem, a Kernel::Exception::ParseError exception is thrown.
 *
 * See also SymmetryOperationSymbolParser::getNormalizedIdentifier, which
 * performs the opposite operation.
 *
 * @param identifier :: Symbol representing a symmetry operation
 * @return Kernel::IntMatrix and V3R, representing the symmetry operation.
 */
MatrixVectorPair<int, V3R>
SymmetryOperationSymbolParser::parseIdentifier(const std::string &identifier) {
  MatrixVectorPair<int, V3R> pair = parseMatrixVectorPair<int>(identifier);

  verifyMatrix(pair.getMatrix());

  return pair;
}

/// Returns a Jones faithful representation of the symmetry operation
/// characterized by the supplied matrix/column pair.
std::string SymmetryOperationSymbolParser::getNormalizedIdentifier(
    const MatrixVectorPair<int, V3R> &data) {
  return getNormalizedIdentifier(data.getMatrix(), data.getVector());
}

/**
 * Returns the Jones faithful representation of a symmetry operation
 *
 * This method generates a Jones faithful string for the given matrix and
 *vector.
 * The string is generated bases on some rules:
 *
 *  - No spaces:
 *          'x + 1/2' -> 'x+1/2'
 *  - Matrix components occur before vector components:
 *          '1/2+x' -> 'x+1/2'
 *  - No leading '+' signs:
 *          '+x' -> 'x'
 *  - If more than one matrix element is present, they are ordered x, y, z:
 *          'y-x' -> '-x+y'
 *
 * If the matrix is not 3x3, an std::runtime_error exception is thrown.
 *
 * @param matrix
 * @param vector
 * @return
 */
std::string SymmetryOperationSymbolParser::getNormalizedIdentifier(
    const Kernel::IntMatrix &matrix, const V3R &vector) {
  if (matrix.numCols() != 3 || matrix.numRows() != 3) {
    throw std::runtime_error("Matrix is not a 3x3 matrix.");
  }

  std::vector<std::string> symbols{"x", "y", "z"};
  std::vector<std::string> components;

  for (size_t r = 0; r < 3; ++r) {
    std::ostringstream currentComponent;

    for (size_t c = 0; c < 3; ++c) {
      if (matrix[r][c] != 0) {
        if (matrix[r][c] < 0) {
          currentComponent << "-";
        } else {
          if (!currentComponent.str().empty()) {
            currentComponent << "+";
          }
        }

        currentComponent << symbols[c];
      }
    }

    if (vector[r] != 0) {
      if (vector[r] > 0) {
        currentComponent << "+";
      }

      if (vector[r].denominator() != 1) {
        currentComponent << vector[r];
      } else {
        currentComponent << vector[r].numerator();
      }
    }

    components.push_back(currentComponent.str());
  }

  return boost::join(components, ",");
}

/// Checks if there are either 1 or 2 zeros in a given matrix row and all
/// non-zero elements are 1 or -1.
bool SymmetryOperationSymbolParser::isValidMatrixRow(const int *element,
                                                     size_t columnNumber) {
  int nulls = 0;
  for (size_t i = 0; i < columnNumber; ++i) {
    if (abs(element[i]) > 1) {
      return false;
    } else if (element[i] == 0) {
      ++nulls;
    }
  }

  return nulls > 0 && nulls < 3;
}

} // namespace Geometry
} // namespace Mantid
