// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/MatrixVectorPair.h"
#include "MantidGeometry/Crystal/V3R.h"
#include "MantidGeometry/DllConfig.h"

#include "MantidKernel/Exception.h"
#include <boost/spirit/include/qi.hpp>
#include <functional>
#include <map>

namespace Mantid {
namespace Geometry {

// TODO moving from boost::optional to std::optoinal in this file is non-trivial
using ParsedRationalNumber = boost::fusion::vector<int, boost::optional<int>>;

class MatrixVectorPairBuilder {
public:
  MatrixVectorPairBuilder() {
    // Fill direction map
    m_directions.emplace("x", V3R(1, 0, 0));
    m_directions.emplace("y", V3R(0, 1, 0));
    m_directions.emplace("z", V3R(0, 0, 1));

    // Initialize all other members correctly and consistently
    reset();
  }

  /// Construct and return the actual matrix/vector pair, the rational matrix
  /// components are castd to T.
  template <typename T> MatrixVectorPair<T, V3R> getMatrixVectorPair() const {
    if (m_currentRow < 2) {
      throw std::runtime_error("Less than three rows were processed by MatrixVectorPairBuilder.");
    }

    std::vector<T> typedMatrixElements;
    for (const auto &rb : m_matrixRows) {
      typedMatrixElements.emplace_back(boost::rational_cast<T>((rb).x()));
      typedMatrixElements.emplace_back(boost::rational_cast<T>((rb).y()));
      typedMatrixElements.emplace_back(boost::rational_cast<T>((rb).z()));
    }
    Kernel::Matrix<T> mat(typedMatrixElements);

    return MatrixVectorPair<T, V3R>(mat, m_vector);
  }

  /// Set the current factor, which is a rational number. Depending on whether a
  /// direction definition follows, it's processed differently later on.
  void setCurrentFactor(const ParsedRationalNumber &rationalNumberComponents) {
    int numerator = boost::fusion::at_c<0>(rationalNumberComponents);
    boost::optional<int> denominator = boost::fusion::at_c<1>(rationalNumberComponents);

    if (denominator.is_initialized()) {
      if (denominator.get() == 0) {
        throw std::runtime_error("Zero denominator is not allowed in MatrixVectorPair-strings.");
      }

      m_currentFactor = RationalNumber(numerator, denominator.get());
    } else {
      m_currentFactor = RationalNumber(numerator);
    }
  }

  /// Set the direction vector to one of the stored values that correspond to
  /// x, y or z.
  void setCurrentDirection(const std::string &vector) { m_currentDirection = m_directions[vector]; }

  /// Make the current sign negative.
  void setCurrentSignNegative() { m_currentSign = -1; }

  /// Make the current sign positive.
  void setCurrentSignPositive() { m_currentSign = 1; }

  /**
   * Adds currently stored state to the parse result
   *
   * This function takes the current factor, sign and direction that were
   * assigned using the respective functions and adds to the stored
   * intermediate result.
   *
   * If the current direction is (0,0,0) then the currently processed
   * component is only a rational number that should go into the vector
   * component of the row that's currently being assembled. Otherwise
   * the direction vector should be multiplied by the rational number
   * and added to the matrix row.
   *
   * After that, the current state is reset so that the next part of the current
   * component can be processed.
   */
  void addCurrentStateToResult() {
    if (m_currentRow > m_matrixRows.size()) {
      throw std::runtime_error("MatrixVectorPair can not have more than 3 rows.");
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

private:
  std::map<std::string, V3R> m_directions;

  std::vector<V3R> m_matrixRows;
  V3R m_vector;

  RationalNumber m_currentFactor;
  V3R m_currentDirection;
  int m_currentSign;

  size_t m_currentRow;
};

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
*/
template <typename Iterator> class MatrixVectorPairParser : public grammar<Iterator, boost::spirit::qi::space_type> {
public:
  MatrixVectorPairParser(MatrixVectorPairBuilder &builder) : MatrixVectorPairParser::base_type(m_parser) {

    using boost::spirit::unused_type;

    namespace qi = boost::spirit::qi;

    using qi::int_;
    using qi::lit;
    using qi::uint_;

    /* MSVC currently has some problems using std::bind in connection
     * with boost::spirit in some circumstances, but they can be circumvented
     * using lambda-expressions as a sort of call proxy. For consistency,
     * all semantic parse actions are formulated like that.
     */
    auto positiveSignAction = [&builder](unused_type, unused_type, unused_type) { builder.setCurrentSignPositive(); };

    auto negativeSignAction = [&builder](unused_type, unused_type, unused_type) { builder.setCurrentSignNegative(); };

    auto currentFactorAction = [&builder](const ParsedRationalNumber &rationalNumberComponents, unused_type,
                                          unused_type) { builder.setCurrentFactor(rationalNumberComponents); };

    auto currentDirectionAction = [&builder](const std::string &s, unused_type, unused_type) {
      builder.setCurrentDirection(s);
    };

    auto addCurrentStateToResultAction = [&builder](unused_type, unused_type, unused_type) {
      builder.addCurrentStateToResult();
    };

    auto advanceRowAction = [&builder](unused_type, unused_type, unused_type) { builder.advanceRow(); };

    // Switch sign in the builder
    m_sign = lit('+')[positiveSignAction] | lit('-')[negativeSignAction];

    // This matches a rational number (also things like -1/-2 -> 1/2)
    m_rational = (int_ >> -('/' >> int_))[currentFactorAction];

    // Matches x, y or z.
    m_direction = (qi::string("x") | qi::string("y") | qi::string("z"))[currentDirectionAction];

    /* A "component", which is either a rational number possibly followed by a
     * vector definition. Examples:
     *    2/3, -2/3, -2/-3, -4, 2, -2x, 3y, 4/5z, ...
     * Or a vector possibly preceded by a sign:
     *    z, -y, +x
     */
    m_component = (m_rational >> -m_direction) | (-m_sign >> m_direction);

    // One row of the matrix/vector pair can have many such "components".
    m_componentSeries = m_component[addCurrentStateToResultAction] >> *(m_component[addCurrentStateToResultAction]);

    // The entire matrix/vector pair is defined by three comma separated of
    // those component strings.
    m_parser = (m_componentSeries >> lit(',')[advanceRowAction] >> m_componentSeries >> lit(',')[advanceRowAction] >>
                m_componentSeries);
  }

  rule<Iterator, boost::spirit::qi::space_type> m_sign, m_rational, m_direction, m_component, m_componentSeries,
      m_parser;
};

/// Tries to parse the given string. Throws a ParseError-exception if there is
/// unparsable string left at the end.
template <typename T> MatrixVectorPair<T, V3R> parseMatrixVectorPair(const std::string &matrixVectorString) {

  namespace qi = boost::spirit::qi;

  auto strIterator = matrixVectorString.cbegin();
  auto strEnd = matrixVectorString.cend();

  MatrixVectorPairBuilder builder;
  MatrixVectorPairParser<std::string::const_iterator> parser(builder);

  try {
    qi::phrase_parse(strIterator, strEnd, parser, qi::space);

    if (std::distance(strIterator, strEnd) > 0) {
      throw std::runtime_error("Additional characters at end of string: '" + std::string(strIterator, strEnd) + "'.");
    }
  } catch (std::runtime_error &builderError) {
    throw Kernel::Exception::ParseError("Parse error: " + std::string(builderError.what()), matrixVectorString, 0);
  }

  return builder.getMatrixVectorPair<T>();
}

} // namespace Geometry
} // namespace Mantid
