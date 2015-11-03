#ifndef MANTID_GEOMETRY_MATRIXVECTORPAIRTEST_H_
#define MANTID_GEOMETRY_MATRIXVECTORPAIRTEST_H_

#include <cxxtest/TestSuite.h>
#include <boost/spirit/include/qi.hpp>
#include <MantidGeometry/Crystal/V3R.h>

#include "MantidGeometry/Crystal/MatrixVectorPair.h"
#include "MantidKernel/Matrix.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace qi = boost::spirit::qi;

void hallo() { std::cout << "Yes" << std::endl; }

class MatrixVectorPairTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MatrixVectorPairTest *createSuite() {
    return new MatrixVectorPairTest();
  }
  static void destroySuite(MatrixVectorPairTest *suite) { delete suite; }

  class RowBuilder {
  public:
    RowBuilder() : m_rowVector(), m_vectorComponent(), m_dirMap() {
      m_dirMap.insert(std::make_pair("x", V3R(1, 0, 0)));
      m_dirMap.insert(std::make_pair("y", V3R(0, 1, 0)));
      m_dirMap.insert(std::make_pair("z", V3R(0, 0, 1)));
    }

    RationalNumber getNumber(int numerator,
                             boost::optional<int> denominator) const {
      if (denominator.is_initialized()) {
        return RationalNumber(numerator, denominator.value());
      } else {
        return RationalNumber(numerator);
      }
    }

    void
    addComponent(boost::fusion::vector<int, boost::optional<int>,
                                       boost::optional<std::string>> arg_) {
      RationalNumber n =
          getNumber(boost::fusion::at_c<0>(arg_), boost::fusion::at_c<1>(arg_));
      boost::optional<std::string> l = boost::fusion::at_c<2>(arg_);

      if (l.is_initialized()) {
        addToRowVector(n, l.value());
      } else {
        addToVectorComponent(n);
      }
    }

    void addToRowVector(const RationalNumber &rn, const std::string &c) {
      m_rowVector += (m_dirMap[c] * rn);
    }

    void addToVectorComponent(const RationalNumber &rn) {
      m_vectorComponent += rn;
    }

    const V3R &getRowVector() const { return m_rowVector; }

    const RationalNumber &getVectorComponent() const {
      return m_vectorComponent;
    }

  private:
    V3R m_rowVector;
    RationalNumber m_vectorComponent;

    std::map<std::string, V3R> m_dirMap;
  };

  void test_Something() {
    using qi::int_;
    using qi::uint_;
    using qi::phrase_parse;
    using qi::space;
    using qi::lit;

    Matrix<double> m(3, 3, false);

    RowBuilder matrixRowBuilder;

    using std::placeholders::_1;

    auto component =
        (-lit('+') >> (int_ >> -('/' >> int_)) >>
         -(qi::string("x") | qi::string("y") | qi::string("z")))[std::bind(
            &RowBuilder::addComponent, &matrixRowBuilder, _1)];

    auto bla = component >> *(component);
    auto parser =
        (bla >> ',' >> bla >> ',' >>
         bla);

    std::string n1 = "+2/3-3/4+1/2+4x-2/3y,x,z";

    bool r1 = phrase_parse(n1.begin(), n1.end(), parser, space);

    m.setRow(0, matrixRowBuilder.getRowVector());

    std::cout << std::endl << V3D(matrixRowBuilder.getRowVector()) << std::endl;
    std::cout << matrixRowBuilder.getVectorComponent() << std::endl;
    std::cout << m << std::endl;
  }
};

#endif /* MANTID_GEOMETRY_MATRIXVECTORPAIRTEST_H_ */
