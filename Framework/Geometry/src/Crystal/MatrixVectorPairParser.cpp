#include "MantidGeometry/Crystal/MatrixVectorPairParser.h"
#include <functional>

namespace Mantid {
namespace Geometry {

MatrixVectorPairParser::MatrixVectorPairBuilder::MatrixVectorPairBuilder()
    : m_directions(), m_matrixRows(3), m_vector(), m_currentFactor(1),
      m_currentDirection(0, 0, 0), m_currentSign(1), m_currentRow(0) {
  m_directions.insert(std::make_pair("x", V3R(1, 0, 0)));
  m_directions.insert(std::make_pair("y", V3R(0, 1, 0)));
  m_directions.insert(std::make_pair("z", V3R(0, 0, 1)));
}

/// Set the current factor, which is a rational number. Depending on whether a
/// direction definition follows, it's processed differently later on.
void MatrixVectorPairParser::MatrixVectorPairBuilder::setCurrentFactor(
    ParsedRationalNumber rationalNumberComponents) {
  int numerator = boost::fusion::at_c<0>(rationalNumberComponents);
  boost::optional<int> denominator =
      boost::fusion::at_c<1>(rationalNumberComponents);

  if (denominator.is_initialized()) {
    m_currentFactor = RationalNumber(numerator, denominator.value());
  } else {
    m_currentFactor = RationalNumber(numerator);
  }
}

/// Set the direction vector to one of the stored values that correspond to x, y
/// or z.
void MatrixVectorPairParser::MatrixVectorPairBuilder::setCurrentDirection(
    std::string vector) {
  m_currentDirection = m_directions[vector];
}

/**
 * Adds currently stored state to the parse result
 *
 * This function takes the current factor, sign and direction that were assigned
 * using the respective functions and adds to the stored intermediate result.
 *
 * If the current direction is (0,0,0) then the currently processed component is
 * only a rational number that should go into the vector component of the row
 * that's currently being assembled. Otherwise the direction vector should be
 * multiplied by the rational number and added to the matrix row.
 *
 * After that, the current state is reset so that the next part of the current
 * component can be processed.
 */
void MatrixVectorPairParser::MatrixVectorPairBuilder::
    addCurrentStateToResult() {
  if (m_currentRow > m_matrixRows.size()) {
    throw Kernel::Exception::ParseError(
        "MatrixVectorPair can not have more than 3 rows.", "", 0);
  }

  m_currentFactor *= m_currentSign;

  if (m_currentDirection != V3R(0, 0, 0)) {
    m_matrixRows[m_currentRow] += (m_currentDirection * m_currentFactor);
  } else {
    m_vector[m_currentRow] += m_currentFactor;
  }

  resetState();
}

/// Constructor, initializes the parser.
MatrixVectorPairParser::MatrixVectorPairParser() : m_builder(), m_parser() {
  initializeParser();
}

/// Initialize the boost::spirit-based parser.
void MatrixVectorPairParser::initializeParser() {
  namespace qi = boost::spirit::qi;

  using std::placeholders::_1;
  using qi::int_;
  using qi::uint_;
  using qi::lit;

  // Switch sign in the builder
  auto sign =
      lit('+')[std::bind(&MatrixVectorPairParser::MatrixVectorPairBuilder::
                             setCurrentSignPositive,
                         &m_builder)] |
      lit('-')[std::bind(&MatrixVectorPairParser::MatrixVectorPairBuilder::
                             setCurrentSignNegative,
                         &m_builder)];

  // This matches a rational number (also things like -1/-2 -> 1/2)
  auto rational = (int_ >> -('/' >> int_))[std::bind(
      &MatrixVectorPairParser::MatrixVectorPairBuilder::setCurrentFactor,
      &m_builder, _1)];

  // Matches x, y or z.
  auto vector = (qi::string("x") | qi::string("y") | qi::string("z"))[std::bind(
      &MatrixVectorPairParser::MatrixVectorPairBuilder::setCurrentDirection,
      &m_builder, _1)];

  /* A "component", which is either a rational number possibly followed by a
   * vector
   * definition. Examples:
   *    2/3, -2/3, -2/-3, -4, 2, -2x, 3y, 4/5z, ...
   * Or a vector possibly preceded by a sign:
   *    z, -y, +x
   */
  auto component = (rational >> -vector) | (-sign >> vector);

  // One row of the matrix/vector pair can have many such "components".
  auto componentSeries =
      component[std::bind(&MatrixVectorPairParser::MatrixVectorPairBuilder::
                              addCurrentStateToResult,
                          &m_builder)] >>
      *(component[std::bind(&MatrixVectorPairParser::MatrixVectorPairBuilder::
                                addCurrentStateToResult,
                            &m_builder)]);

  // The entire matrix/vector pair is defined by three comma separated of those
  // component strings.
  m_parser = (componentSeries >>
              lit(',')[std::bind(
                  &MatrixVectorPairParser::MatrixVectorPairBuilder::advanceRow,
                  &m_builder)] >>
              componentSeries >>
              lit(',')[std::bind(
                  &MatrixVectorPairParser::MatrixVectorPairBuilder::advanceRow,
                  &m_builder)] >>
              componentSeries);
}

} // namespace Geometry
} // namespace Mantid
