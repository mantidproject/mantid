#ifndef MANTID_GEOMETRY_MATRIXVECTORPAIRPARSER_H_
#define MANTID_GEOMETRY_MATRIXVECTORPAIRPARSER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/MatrixVectorPair.h"
#include "MantidGeometry/Crystal/V3R.h"

#include "MantidKernel/Exception.h"

#include <boost/spirit/include/qi.hpp>

namespace Mantid {
namespace Geometry {

typedef boost::fusion::vector<int, boost::optional<int> > ParsedRationalNumber;

using boost::spirit::qi::grammar;
using boost::spirit::qi::rule;

/** MatrixVectorPairParser

  MatrixVectorPairParser can parse matrix/vector pairs in
  Jones faithful notation of the form:

    a/b-x, cy, -z

  The resulting matrix/vector pair can be retrieved through a
  templated method that lets the caller decide on the numeric
  type stored in the matrix. The vector is currently always V3R.

  To reuse the internally constructed boost::spirit-based parser,
  simply instantiate MatrixVectorPairParser once and use the
  parse-method.

      @author Michael Wedel, ESS
      @date 02/11/2015

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class MANTID_GEOMETRY_DLL MatrixVectorPairParser
    : public grammar<std::string::const_iterator,
                     boost::spirit::qi::space_type> {
public:
  MatrixVectorPairParser()
      : MatrixVectorPairParser::base_type(m_parser), m_directions(),
        m_matrixRows(3), m_vector(), m_currentFactor(1),
        m_currentDirection(0, 0, 0), m_currentSign(1), m_currentRow(0) {
    m_directions.insert(std::make_pair("x", V3R(1, 0, 0)));
    m_directions.insert(std::make_pair("y", V3R(0, 1, 0)));
    m_directions.insert(std::make_pair("z", V3R(0, 0, 1)));

    namespace qi = boost::spirit::qi;

    using std::placeholders::_1;
    using qi::int_;
    using qi::uint_;
    using qi::lit;

    // Switch sign in the builder
    m_sign =
        lit('+')
            [std::bind(&MatrixVectorPairParser::setCurrentSignPositive, this)] |
        lit('-')
            [std::bind(&MatrixVectorPairParser::setCurrentSignNegative, this)];

    // This matches a rational number (also things like -1/-2 -> 1/2)
    m_rational = (int_ >> -('/' >> int_))
        [std::bind(&MatrixVectorPairParser::setCurrentFactor, this, _1)];

    // Matches x, y or z.
    m_direction = (qi::string("x") | qi::string("y") | qi::string("z"))
        [std::bind(&MatrixVectorPairParser::setCurrentDirection, this, _1)];

    /* A "component", which is either a rational number possibly followed by a
     * vector
     * definition. Examples:
     *    2/3, -2/3, -2/-3, -4, 2, -2x, 3y, 4/5z, ...
     * Or a vector possibly preceded by a sign:
     *    z, -y, +x
     */
    m_component = (m_rational >> -m_direction) | (-m_sign >> m_direction);

    // One row of the matrix/vector pair can have many such "components".
    m_componentSeries =
        m_component[std::bind(&MatrixVectorPairParser::addCurrentStateToResult,
                              this)] >>
        *(m_component[std::bind(
             &MatrixVectorPairParser::addCurrentStateToResult, this)]);

    // The entire matrix/vector pair is defined by three comma separated of
    // those
    // component strings.
    m_parser =
        (m_componentSeries >>
         lit(',')[std::bind(&MatrixVectorPairParser::advanceRow, this)] >>
         m_componentSeries >>
         lit(',')[std::bind(&MatrixVectorPairParser::advanceRow, this)] >>
         m_componentSeries);
  }

  /// Tries to parse the given string. Throws a ParseError-exception if there is
  /// unparsable string left at the end.
  template <typename T>
  MatrixVectorPair<T, V3R> parse(const std::string &matrixVectorString) {
    reset();

    namespace qi = boost::spirit::qi;

    std::string::const_iterator strIterator = matrixVectorString.begin();
    std::string::const_iterator strEnd = matrixVectorString.end();

    qi::phrase_parse(strIterator, strEnd, *this, qi::space);

    if (std::distance(strIterator, strEnd) > 0) {
      throw Kernel::Exception::ParseError(
          "Additional characters at end of string: '" +
              std::string(strIterator, strEnd) + "'.",
          "", 0);
    }

    return getMatrixVectorPair<T>();
  }

private:
  /// Construct and return the actual matrix/vector pair, the rational matrix
  /// components are castd to T.
  template <typename T> MatrixVectorPair<T, V3R> getMatrixVectorPair() const {
    if (m_currentRow < 2) {
      throw Kernel::Exception::ParseError(
          "Less than three rows were processed by MatrixVectorPairBuilder.", "",
          0);
    }

    std::vector<T> dbl;
    for (auto rb : m_matrixRows) {
      dbl.push_back(boost::rational_cast<T>((rb).x()));
      dbl.push_back(boost::rational_cast<T>((rb).y()));
      dbl.push_back(boost::rational_cast<T>((rb).z()));
    }
    Kernel::Matrix<T> mat(dbl);

    return MatrixVectorPair<T, V3R>(mat, m_vector);
  }

  /// Set the current factor, which is a rational number. Depending on whether a
  /// direction definition follows, it's processed differently later on.
  void setCurrentFactor(ParsedRationalNumber rationalNumberComponents) {
    int numerator = boost::fusion::at_c<0>(rationalNumberComponents);
    boost::optional<int> denominator =
        boost::fusion::at_c<1>(rationalNumberComponents);

    if (denominator.is_initialized()) {
      m_currentFactor = RationalNumber(numerator, denominator.get());
    } else {
      m_currentFactor = RationalNumber(numerator);
    }
  }

  /// Set the direction vector to one of the stored values that correspond to x,
  /// y
  /// or z.
  void setCurrentDirection(std::string vector) {
    m_currentDirection = m_directions[vector];
  }

  /// Make the current sign negative.
  void setCurrentSignNegative() { m_currentSign = -1; }

  /// Make the current sign positive.
  void setCurrentSignPositive() { m_currentSign = 1; }

  /**
   * Adds currently stored state to the parse result
   *
   * This function takes the current factor, sign and direction that were
   *assigned
   * using the respective functions and adds to the stored intermediate result.
   *
   * If the current direction is (0,0,0) then the currently processed component
   *is
   * only a rational number that should go into the vector component of the row
   * that's currently being assembled. Otherwise the direction vector should be
   * multiplied by the rational number and added to the matrix row.
   *
   * After that, the current state is reset so that the next part of the current
   * component can be processed.
   */
  void addCurrentStateToResult() {
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

  /// Advance to the next row of the matrix/vector pair.
  void advanceRow() {
    ++m_currentRow;
    resetState();
  }

  /// Completely reset the builder, including stored preliminary results.
  void reset() {
    resetState();
    resetAccumulatedResults();
  }

  /// Reset the current state, i.e. the state representing the current row.
  void resetState() {
    m_currentFactor = RationalNumber(1);
    m_currentDirection = V3R(0, 0, 0);
    m_currentSign = 1;
  }

  /// Reset all accumulated results.
  void resetAccumulatedResults() {
    m_matrixRows = std::vector<V3R>(3);
    m_vector = V3R(0, 0, 0);
    m_currentRow = 0;
  }

  rule<std::string::const_iterator, boost::spirit::qi::space_type> m_sign,
      m_rational, m_direction, m_component, m_componentSeries, m_parser;

  std::map<std::string, V3R> m_directions;

  std::vector<V3R> m_matrixRows;
  V3R m_vector;

  RationalNumber m_currentFactor;
  V3R m_currentDirection;
  int m_currentSign;

  size_t m_currentRow;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_MATRIXVECTORPAIRPARSER_H_ */
