#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
namespace Mantid
{
namespace Geometry
{

boost::regex SymmetryOperation::m_tokenRegex = boost::regex("[+\\-]?((x|y|z)|(\\d/\\d))", boost::regex::icase);
boost::regex SymmetryOperation::m_matrixRowRegex = boost::regex("^[+\\-]?(x|y|z)", boost::regex::icase);
boost::regex SymmetryOperation::m_vectorComponentRegex = boost::regex("^[+\\-]?(\\d/\\d)", boost::regex::icase);

/**
 * The constructor of SymmetryOperation
 *
 * Since the SymmetryOperation base-class is not intended to be used directly, the
 * only constructor is protected. This way, it can be called by the inheriting classes
 * with the correct parameters, but no "anonymous" symmetry operation is possible.
 *
 * Three parameters are required. The "order", as described in the header file specifies
 * how often the operation has to be applied to an object in sequence until it is
 * identical with itself. The matrix-parameter is the transformation matrix that
 * represents the operation and identifier is a string that follows a certain convention,
 * also described in the header file. It must match the following regular expression:
 *
 *      ^-?((1)|((2|3|4|6|m) \\[(-?\\d{1}){3}\\]h?))$
 *
 * @param order :: Order of the symmetry operation.
 * @param matrix :: Integer matrix with dimensions 3x3, defines symmetry operation.
 * @param identifier :: Identifier string for symmetry operation.
 */
SymmetryOperation::SymmetryOperation(size_t order, Kernel::IntMatrix matrix, std::string identifier) :
    m_order(order),
    m_matrix(matrix),
    m_vector(),
    m_identifier(identifier)
{
}

SymmetryOperation::SymmetryOperation() :
    m_order(1),
    m_matrix(Kernel::IntMatrix(3, 3, true)),
    m_vector(),
    m_identifier()
{

}

SymmetryOperation::SymmetryOperation(const Kernel::IntMatrix &matrix, const V3R &vector) :
    m_order(0),
    m_matrix(matrix),
    m_vector(vector),
    m_identifier()
{

}

void SymmetryOperation::initialize()
{
    m_order = getOrderFromComponents(m_matrix, m_vector);
    m_identifier = getIdentifierFromComponents(m_matrix, m_vector);
}

/**
 * Returns the order of the symmetry operation
 *
 * @return Order of the symmetry operation
 */
size_t SymmetryOperation::order() const
{
    return m_order;
}

/**
 * Returns the string-identifier for this symmetry operation
 *
 * @return Identifier of the symmetry operation
 */
std::string SymmetryOperation::identifier() const
{
    return m_identifier;
}

bool SymmetryOperation::isIdentity() const
{
    return m_matrix == Kernel::IntMatrix(3, 3, true) && m_vector == 0;
}

SymmetryOperation_const_sptr SymmetryOperation::operator *(const SymmetryOperation_const_sptr &operand) const
{
    return boost::make_shared<const SymmetryOperation>(m_matrix * operand->m_matrix, getWrappedVector((m_matrix * operand->m_vector) + m_vector) );
}

/**
 * Takes a flat int-array and assigns its 9 elements to the internal matrix.
 *
 * @param array :: int-array containing the transformation matrix.
 */
void SymmetryOperation::setMatrixFromArray(int array[])
{
    for(size_t row = 0; row < 3; ++row) {
        for(size_t col = 0; col < 3; ++col) {
            m_matrix[row][col] = array[row * 3 + col];
        }
    }
}

V3R SymmetryOperation::getWrappedVector(const V3R &vector) const
{
    V3R wrappedVector(vector);
    for(size_t i = 0; i < 3; ++i) {
        if(wrappedVector[i] < 0) {
            wrappedVector[i] += 1;
        } else if(wrappedVector[i] >= 1) {
            wrappedVector[i] -= 1;
        }
    }

    return wrappedVector;
}

size_t SymmetryOperation::getOrderFromComponents(const Kernel::IntMatrix &matrix, const V3R &vector) const
{
    SymmetryOperation_const_sptr symOp = boost::make_shared<const SymmetryOperation>(matrix, vector);

    size_t i = 1;

    SymmetryOperation_const_sptr buffer = symOp;
    while(!buffer->isIdentity()) {
        buffer = (*symOp) * buffer;
        ++i;
    }

    return i;
}

std::string SymmetryOperation::getIdentifierFromComponents(const Kernel::IntMatrix &matrix, const V3R &vector) const
{
    if(matrix.numCols() != 3 || matrix.numRows() != 3) {
        return "";
    }

    std::vector<std::string> symbols;
    symbols.push_back("x");
    symbols.push_back("y");
    symbols.push_back("z");

    std::vector<std::string> components;

    for(size_t r = 0; r < 3; ++r) {
        std::ostringstream currentComponent;

        if(vector[r] != 0) {
            currentComponent << vector[r];
        }

        for(size_t c = 0; c < 3; ++c) {
            if(matrix[r][c] != 0) {
                if(matrix[r][c] < 0) {
                    currentComponent << "-";
                } else {
                    if(vector[r] != 0) {
                        currentComponent << "+";
                    }
                }

                currentComponent << symbols[c];
            }
        }

        components.push_back(currentComponent.str());
    }

    return boost::join(components, ",");
}

std::pair<Kernel::IntMatrix, V3R> SymmetryOperation::parseIdentifier(const std::string &identifier) const
{
    std::vector<std::string> components;
    boost::split(components, identifier, boost::is_any_of(","));

    return parseComponents(components);
}

std::pair<Kernel::IntMatrix, V3R> SymmetryOperation::parseComponents(const std::vector<std::string> &components) const
{
    if(components.size() != 3) {
        throw std::runtime_error("Failed to parse identifier [" + boost::join(components, ", ") + "]: Wrong number of components.");
    }

    Kernel::IntMatrix matrix(3, 3);
    V3R vector;

    for(size_t i = 0; i < 3; ++i) {
        std::pair<std::vector<int>, RationalNumber> currentComponent = parseComponent(getCleanComponentString(components[i]));

        matrix.setRow(i, currentComponent.first);
        vector[i] = currentComponent.second;
    }

    return std::make_pair(matrix, vector);
}

std::string SymmetryOperation::getCleanComponentString(const std::string &componentString) const
{
    return boost::algorithm::erase_all_copy(componentString, " ");
}

std::pair<std::vector<int>, RationalNumber> SymmetryOperation::parseComponent(const std::string &component) const
{
    std::vector<int> matrixRow(3, 0);
    RationalNumber vectorComponent;

    size_t totalMatchedLength = 0;

    boost::sregex_iterator iter(component.begin(), component.end(), m_tokenRegex);
    boost::sregex_iterator end;
    for(; iter != end; ++iter) {
        std::string currentString = iter->str();
        totalMatchedLength += currentString.size();

        if(boost::regex_match(currentString, m_matrixRowRegex)) {
            processMatrixRowToken(currentString, matrixRow);
        } else if(boost::regex_match(currentString, m_vectorComponentRegex)) {
            processVectorComponentToken(currentString, vectorComponent);
        } else {
            throw std::runtime_error("Failed to parse input: " + component);
        }
    }

    if(totalMatchedLength < component.size()) {
        throw std::runtime_error("Failed to parse component string " + component + ": Could not parse entire string.");
    }

    if(!isValidMatrixRow(matrixRow)) {
        throw std::runtime_error("Failed to parse component string " + component + ": Matrix row is invalid (all 0 or an abs(element) > 1).");
    }

    return std::make_pair(matrixRow, vectorComponent);
}

void SymmetryOperation::processMatrixRowToken(const std::string &matrixToken, std::vector<int> &matrixRow) const
{
    switch(matrixToken.size()) {
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

void SymmetryOperation::addToVector(std::vector<int> &vector, const std::vector<int> &add) const
{
    if(vector.size() != add.size()) {
        throw std::runtime_error("Vectors do not have matching sizes, can not add.");
    }

    for(size_t i = 0; i < vector.size(); ++i) {
        vector[i] += add[i];
    }
}

std::vector<int> SymmetryOperation::getVectorForSymbol(const char symbol, const char sign) const
{
    int factor = getFactorForSign(sign);

    std::vector<int> symbolVector(3, 0);

    switch(symbol) {
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
        throw std::runtime_error("Failed to parse matrix row token " + std::string(&symbol) + " with sign " + std::string(&sign));
    }

    return symbolVector;
}

int SymmetryOperation::getFactorForSign(const char sign) const
{
    switch(sign) {
    case '+':
        return 1;
    case '-':
        return -1;
    default:
        throw std::runtime_error("Failed to parse sign " + std::string(&sign));
    }
}

void SymmetryOperation::processVectorComponentToken(const std::string &rationalNumberToken, RationalNumber &vectorComponent) const
{
    std::vector<std::string> components;
    boost::split(components, rationalNumberToken, boost::is_any_of("/"));

    switch(components.size()) {
    case 1:
        vectorComponent += boost::lexical_cast<int>(components.front());
        break;
    case 2:
        if(!(components.front()).empty() && !(components.back()).empty()) {
            vectorComponent += RationalNumber(
                                    boost::lexical_cast<int>(components.front()),
                                    boost::lexical_cast<int>(components.back())
                               );
            break;
        }
    default:
        throw std::runtime_error("Failed to parse vector token " + rationalNumberToken);
    }
}

bool SymmetryOperation::isValidMatrixRow(const std::vector<int> &matrixRow) const
{
    int nulls = 0;

    for(auto it = matrixRow.begin(); it != matrixRow.end(); ++it) {
        if(abs(*it) > 1) {
            return false;
        } else if(*it == 0) {
            ++nulls;
        }
    }

    return nulls > 0 && nulls < 3;
}

/// Identity
SymOpIdentity::SymOpIdentity() :
    SymmetryOperation(1, Kernel::IntMatrix(3, 3, true), "1")
{

}

DECLARE_SYMMETRY_OPERATION(SymOpIdentity)

/// Inversion
SymOpInversion::SymOpInversion() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3, true), "-1")
{
    m_matrix *= -1;
}

DECLARE_SYMMETRY_OPERATION(SymOpInversion)


/* 2-fold rotation axes */
/// 2-fold rotation around x-axis
SymOpRotationTwoFoldX::SymOpRotationTwoFoldX() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [100]")
{
    int rotTwoFoldX[] = {1,  0,  0,
                         0, -1,  0,
                         0,  0, -1};

    setMatrixFromArray(rotTwoFoldX);
}

DECLARE_SYMMETRY_OPERATION(SymOpRotationTwoFoldX)

/// 2-fold rotation around y-axis
SymOpRotationTwoFoldY::SymOpRotationTwoFoldY() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [010]")
{
    int rotTwoFoldY[] = {-1,  0,  0,
                          0,  1,  0,
                          0,  0, -1};

    setMatrixFromArray(rotTwoFoldY);
}

DECLARE_SYMMETRY_OPERATION(SymOpRotationTwoFoldY)

/// 2-fold rotation around z-axis
SymOpRotationTwoFoldZ::SymOpRotationTwoFoldZ() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [001]")
{
    int rotTwoFoldZ[] = {-1,  0,  0,
                          0, -1,  0,
                          0,  0,  1};

    setMatrixFromArray(rotTwoFoldZ);
}

DECLARE_SYMMETRY_OPERATION(SymOpRotationTwoFoldZ)

/// 2-fold rotation around x-axis, hexagonal coordinate system
SymOpRotationTwoFoldXHexagonal::SymOpRotationTwoFoldXHexagonal() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [100]h")
{
    int rotTwoFoldXHexagonal[] = {1, -1,  0,
                                  0, -1,  0,
                                  0,  0, -1};

    setMatrixFromArray(rotTwoFoldXHexagonal);
}

DECLARE_SYMMETRY_OPERATION(SymOpRotationTwoFoldXHexagonal)

/// 2-fold rotation around [210]-axis, hexagonal coordinate system
SymOpRotationTwoFold210Hexagonal::SymOpRotationTwoFold210Hexagonal() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "2 [210]h")
{
    int rotTwoFold210Hexagonal[] = { 1,  0,  0,
                                     1, -1,  0,
                                     0,  0, -1};

    setMatrixFromArray(rotTwoFold210Hexagonal);
}

DECLARE_SYMMETRY_OPERATION(SymOpRotationTwoFold210Hexagonal)

/* 4-fold rotation axes */
/// 4-fold rotation around z-axis
SymOpRotationFourFoldZ::SymOpRotationFourFoldZ() :
    SymmetryOperation(4, Kernel::IntMatrix(3, 3), "4 [001]")
{
    int rotFourFoldZ[] = { 0, -1,  0,
                           1,  0,  0,
                           0,  0,  1};

    setMatrixFromArray(rotFourFoldZ);
}

DECLARE_SYMMETRY_OPERATION(SymOpRotationFourFoldZ)

/* 3-fold rotation axes */
/// 3-fold rotation around z-axis, hexagonal coordinate system
SymOpRotationThreeFoldZHexagonal::SymOpRotationThreeFoldZHexagonal() :
    SymmetryOperation(3, Kernel::IntMatrix(3, 3), "3 [001]h")
{
    int rotThreeFoldZHexagonal[] = { 0, -1,  0,
                                     1, -1,  0,
                                     0,  0,  1};

    setMatrixFromArray(rotThreeFoldZHexagonal);
}

DECLARE_SYMMETRY_OPERATION(SymOpRotationThreeFoldZHexagonal)

/// 3-fold rotation around [111]-axis
SymOpRotationThreeFold111::SymOpRotationThreeFold111() :
    SymmetryOperation(3, Kernel::IntMatrix(3, 3), "3 [111]")
{
    int rotThreeFold111[] = { 0,  0,  1,
                              1,  0,  0,
                              0,  1,  0};

    setMatrixFromArray(rotThreeFold111);
}

DECLARE_SYMMETRY_OPERATION(SymOpRotationThreeFold111)

/* 6-fold rotation axes */
/// 6-fold rotation around z-axis, hexagonal coordinate system
SymOpRotationSixFoldZHexagonal::SymOpRotationSixFoldZHexagonal() :
    SymmetryOperation(6, Kernel::IntMatrix(3, 3), "6 [001]h")
{
    int rotSixFoldZHexagonal[] = { 1, -1,  0,
                                   1,  0,  0,
                                   0,  0,  1};

    setMatrixFromArray(rotSixFoldZHexagonal);
}

DECLARE_SYMMETRY_OPERATION(SymOpRotationSixFoldZHexagonal)

/* Mirror planes */
/// Mirror plane perpendicular to y-axis
SymOpMirrorPlaneY::SymOpMirrorPlaneY() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "m [010]")
{
    int mirrorPlaneY[] = {1,  0,  0,
                          0, -1,  0,
                          0,  0,  1};

    setMatrixFromArray(mirrorPlaneY);
}

DECLARE_SYMMETRY_OPERATION(SymOpMirrorPlaneY)

/// Mirror plane perpendicular to z-axis
SymOpMirrorPlaneZ::SymOpMirrorPlaneZ() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "m [001]")
{
    int mirrorPlaneZ[] = {1,  0,  0,
                          0,  1,  0,
                          0,  0, -1};

    setMatrixFromArray(mirrorPlaneZ);
}

DECLARE_SYMMETRY_OPERATION(SymOpMirrorPlaneZ)

/// Mirror plane perpendicular to [210]-axis
SymOpMirrorPlane210Hexagonal::SymOpMirrorPlane210Hexagonal() :
    SymmetryOperation(2, Kernel::IntMatrix(3, 3), "m [210]h")
{
    int mirrorPlane210Hexagonal[] = {-1,  0,  0,
                                     -1,  1,  0,
                                      0,  0,  1};

    setMatrixFromArray(mirrorPlane210Hexagonal);
}

DECLARE_SYMMETRY_OPERATION(SymOpMirrorPlane210Hexagonal)

} // namespace Geometry
} // namespace Mantid
