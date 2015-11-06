#ifndef MANTID_GEOMETRY_MATRIXVECTORPAIRPARSER_H_
#define MANTID_GEOMETRY_MATRIXVECTORPAIRPARSER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/MatrixVectorPair.h"
#include "MantidGeometry/Crystal/V3R.h"

#include "MantidKernel/Exception.h"

#include <boost/spirit/include/qi.hpp>

namespace Mantid {
namespace Geometry {

typedef boost::fusion::vector<int, boost::optional<int>> ParsedRationalNumber;

/** MatrixVectorPairParser

  MatrixVectorPairParser can parse matrix/vector pairs in
  Jones faithful notation of the form:

    a/b-x, cy, -z

  The resulting matrix/vector pair can be retrieved through a
  templated method that lets the caller decide on the numeric
  type stored in the matrix. The vector is currently always V3R.

  To reuse the internally constructed boost::spirit-based parser,
  simply instantiate MatrixVectorPairParser once and use the
  parseMatrix

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
class MANTID_GEOMETRY_DLL MatrixVectorPairParser {
public:
  MatrixVectorPairParser();
  ~MatrixVectorPairParser() {}

  /// Tries to parse the given string. Throws a ParseError-exception if there is
  /// unparsable string left at the end.
  template <typename T>
  MatrixVectorPair<T, V3R> parse(const std::string &matrixVectorString) {
    namespace qi = boost::spirit::qi;

    std::string::const_iterator strIterator = matrixVectorString.begin();
    std::string::const_iterator strEnd = matrixVectorString.end();

    m_builder.reset();
    qi::phrase_parse(strIterator, strEnd, m_parser, qi::space);

    if (std::distance(strIterator, strEnd) > 0) {
      throw Kernel::Exception::ParseError(
          "Additional characters at end of string: '" +
              std::string(strIterator, strEnd) + "'.",
          "", 0);
    }

    return m_builder.getMatrixVectorPair<T>();
  }

private:
  /// Helper class to build up matrix/vector pair in parser.
  class MatrixVectorPairBuilder {
  public:
    MatrixVectorPairBuilder();

    /// Construct and return the actual matrix/vector pair, the rational matrix
    /// components are castd to T.
    template <typename T> MatrixVectorPair<T, V3R> getMatrixVectorPair() const {
      if (m_currentRow < 2) {
        throw Kernel::Exception::ParseError(
            "Less than three rows were processed by MatrixVectorPairBuilder.",
            "", 0);
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

    /// Returns the parsed vector.
    const V3R &getVector() const { return m_vector; }

    void setCurrentFactor(ParsedRationalNumber rationalNumberComponents);
    void setCurrentDirection(std::string vector);

    /// Make the current sign negative.
    void setCurrentSignNegative() { m_currentSign = -1; }

    /// Make the current sign positive.
    void setCurrentSignPositive() { m_currentSign = 1; }

    void addCurrentStateToResult();

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

  private:
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

    std::map<std::string, V3R> m_directions;

    std::vector<V3R> m_matrixRows;
    V3R m_vector;

    RationalNumber m_currentFactor;
    V3R m_currentDirection;
    int m_currentSign;

    size_t m_currentRow;
  };

  void initializeParser();

  MatrixVectorPairBuilder m_builder;
  boost::spirit::qi::rule<std::string::const_iterator,
                          boost::spirit::qi::space_type> m_parser;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_MATRIXVECTORPAIRPARSER_H_ */
