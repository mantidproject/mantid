#include "MantidGeometry/Crystal/SymmetryOperationSymbolParser.h"
#include "MantidKernel/Exception.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace Geometry {

/// Default constructor
SymmetryOperationSymbolParser::SymmetryOperationSymbolParser() {}

/**
 * Tries to parse the given symbol
 *
 * This method tries to parse a given symbol and returns the matrix/vector pair
 *resulting from
 * the parsing process. It takes a string representing a symmetry operation in
 *the format:
 *      x+a/b, -y-c/d, e/f-z
 * where x, y and z are the literals 'x', 'y' and 'z', while a-f are integers,
 *representing
 * rational numbers. The latter don't need to be present, a string "x,y,z" is
 *valid. Leading plus-signs
 * may be included if desired, so that "+x,+y,+z" is also valid.
 *
 * If there is a problem, a Kernel::Exception::ParseError exception is thrown.
 *
 * See also SymmetryOperationSymbolParser::getNormalizedIdentifier, which
 *performs the opposite operation.
 *
 * @param identifier :: Symbol representing a symmetry operation
 * @return Pair of Kernel::IntMatrix and V3R, representing the symmetry
 *operation.
 */
std::pair<Kernel::IntMatrix, V3R>
SymmetryOperationSymbolParser::parseIdentifier(const std::string &identifier) {
  std::vector<std::string> components;
  boost::split(components, identifier, boost::is_any_of(","));

  try {
    std::pair<Kernel::IntMatrix, V3R> matrixVector =
        parseComponents(components);

    return matrixVector;
  } catch (const std::runtime_error &e1) {
    throw Kernel::Exception::ParseError("Error in parsing symbol " +
                                            identifier + ":\n" +
                                            std::string(e1.what()),
                                        "", 0);
  } catch (const boost::bad_lexical_cast &e2) {
    throw Kernel::Exception::ParseError("Error in parsing symbol " +
                                            identifier + ":\n" +
                                            std::string(e2.what()),
                                        "", 0);
  }
}

/// Returns a Jones faithful representation of the symmetry operation
/// characterized by the supplied matrix/column pair.
std::string SymmetryOperationSymbolParser::getNormalizedIdentifier(
    const std::pair<Kernel::IntMatrix, V3R> &data) {
  return getNormalizedIdentifier(data.first, data.second);
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

  std::vector<std::string> symbols;
  symbols.push_back("x");
  symbols.push_back("y");
  symbols.push_back("z");

  std::vector<std::string> components;

  for (size_t r = 0; r < 3; ++r) {
    std::ostringstream currentComponent;

    for (size_t c = 0; c < 3; ++c) {
      if (matrix[r][c] != 0) {
        if (matrix[r][c] < 0) {
          currentComponent << "-";
        } else {
          if (currentComponent.str().size() > 0) {
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
      currentComponent << vector[r];
    }

    components.push_back(currentComponent.str());
  }

  return boost::join(components, ",");
}

/// Tries to parse the three components of the symbol, throws std::runtime_error
/// if number of components is not three.
std::pair<Kernel::IntMatrix, V3R>
SymmetryOperationSymbolParser::parseComponents(
    const std::vector<std::string> &components) {
  if (components.size() != 3) {
    throw std::runtime_error("Failed to parse identifier [" +
                             boost::join(components, ", ") +
                             "]: Wrong number of components.");
  }

  Kernel::IntMatrix matrix(3, 3);
  V3R vector;

  // Each part of the symbol contains one row of the resulting matrix and the
  // magnitude of the translation vector.
  for (size_t i = 0; i < 3; ++i) {
    std::pair<std::vector<int>, RationalNumber> currentComponent =
        parseComponent(getCleanComponentString(components[i]));

    matrix.setRow(i, currentComponent.first);
    vector[i] = currentComponent.second;
  }

  return std::make_pair(matrix, vector);
}

/// Strips all spaces from a string, also in the middle.
std::string SymmetryOperationSymbolParser::getCleanComponentString(
    const std::string &componentString) {
  return boost::algorithm::erase_all_copy(componentString, " ");
}

/// Tries to parse a single component of the total symbol, throws
/// std::runtime_error if the string can not be parsed.
std::pair<std::vector<int>, RationalNumber>
SymmetryOperationSymbolParser::parseComponent(const std::string &component) {
  std::vector<int> matrixRow(3, 0);
  RationalNumber vectorComponent;

  size_t totalMatchedLength = 0;

  // Regular expressions for different token types
  boost::regex tokenRegex("[+\\-]?((x|y|z)|(\\d/\\d))", boost::regex::icase);
  boost::regex matrixRowRegex("^[+\\-]?(x|y|z)", boost::regex::icase);
  boost::regex vectorComponentRegex("^[+\\-]?(\\d/\\d)", boost::regex::icase);

  // Check how many tokens this string is composed of and iterate through them
  boost::sregex_iterator iter(component.begin(), component.end(), tokenRegex);
  boost::sregex_iterator end;
  for (; iter != end; ++iter) {
    std::string currentString = iter->str();
    totalMatchedLength += currentString.size();

    // Try to handle the current token as either a matrix row (x, y, z) or a
    // vector component (a/b)
    if (boost::regex_match(currentString, matrixRowRegex)) {
      processMatrixRowToken(currentString, matrixRow);
    } else if (boost::regex_match(currentString, vectorComponentRegex)) {
      processVectorComponentToken(currentString, vectorComponent);
    } else {
      throw std::runtime_error("Failed to parse input: " + component);
    }
  }

  // If the combined length of the matched sub strings is less than the total
  // string length, there was some garbage inbetween.
  if (totalMatchedLength < component.size()) {
    throw std::runtime_error("Failed to parse component string " + component +
                             ": Could not parse entire string.");
  }

  // The matrix may be invalid, this happens when something like x+x+y+z is
  // specified.
  if (!isValidMatrixRow(matrixRow)) {
    throw std::runtime_error(
        "Failed to parse component string " + component +
        ": Matrix row is invalid (all 0 or an abs(element) > 1).");
  }

  return std::make_pair(matrixRow, vectorComponent);
}

/// Try to generate a matrix row from the token and add it to the supplied
/// vector, throws std::runtime_error if it fails to parse the input.
void SymmetryOperationSymbolParser::processMatrixRowToken(
    const std::string &matrixToken, std::vector<int> &matrixRow) {
  switch (matrixToken.size()) {
  case 1:
    addToVector(matrixRow, getVectorForSymbol(matrixToken[0]));
    break;
  case 2:
    addToVector(matrixRow, getVectorForSymbol(matrixToken[1], matrixToken[0]));
    break;
  default:
    throw std::runtime_error("Failed to parse matrix row token " + matrixToken);
  }
}

/// Add a vector to another vector (element wise), throws std::runtime_error if
/// sizes don't match.
void SymmetryOperationSymbolParser::addToVector(std::vector<int> &vector,
                                                const std::vector<int> &add) {
  if (vector.size() != add.size()) {
    throw std::runtime_error(
        "Vectors do not have matching sizes, can not add.");
  }

  for (size_t i = 0; i < vector.size(); ++i) {
    vector[i] += add[i];
  }
}

/// Returns the vector corresponding to the given symbol (x: (1, 0, 0); y: (0,
/// 1, 0); z: (0, 0, 1)) and adds + or -.
std::vector<int>
SymmetryOperationSymbolParser::getVectorForSymbol(const char symbol,
                                                  const char sign) {
  int factor = getFactorForSign(sign);

  std::vector<int> symbolVector(3, 0);

  switch (symbol) {
  case 'x':
    symbolVector[0] = factor * 1;
    break;
  case 'y':
    symbolVector[1] = factor * 1;
    break;
  case 'z':
    symbolVector[2] = factor * 1;
    break;
  default:
    throw std::runtime_error("Failed to parse matrix row token " +
                             std::string(1, symbol) + " with sign " +
                             std::string(1, sign));
  }

  return symbolVector;
}

/// Returns a multiplication factor for the given sign ('-': -1, '+': 1).
int SymmetryOperationSymbolParser::getFactorForSign(const char sign) {
  switch (sign) {
  case '+':
    return 1;
  case '-':
    return -1;
  default:
    throw std::runtime_error("Failed to parse sign " + std::string(1, sign));
  }
}

/// Tries to create a RationalNumber from the input and adds it to the supplied
/// RationalNumber.
void SymmetryOperationSymbolParser::processVectorComponentToken(
    const std::string &rationalNumberToken, RationalNumber &vectorComponent) {
  std::vector<std::string> components;
  boost::split(components, rationalNumberToken, boost::is_any_of("/"));

  switch (components.size()) {
  case 1:
    vectorComponent += boost::lexical_cast<int>(components.front());
    break;
  case 2:
    if (!(components.front()).empty() && !(components.back()).empty()) {
      vectorComponent +=
          RationalNumber(boost::lexical_cast<int>(components.front()),
                         boost::lexical_cast<int>(components.back()));
      break;
    }
  default:
    throw std::runtime_error("Failed to parse vector token " +
                             rationalNumberToken);
  }
}

/// Checks if there are either 1 or 2 zeros in a given matrix row and all
/// non-zero elements are 1 or -1.
bool SymmetryOperationSymbolParser::isValidMatrixRow(
    const std::vector<int> &matrixRow) {
  int nulls = 0;

  for (auto it = matrixRow.begin(); it != matrixRow.end(); ++it) {
    if (abs(*it) > 1) {
      return false;
    } else if (*it == 0) {
      ++nulls;
    }
  }

  return nulls > 0 && nulls < 3;
}

} // namespace Geometry
} // namespace Mantid
