#ifndef MANTID_CURVEFITTING_PRODUCTQUADRATICEXPTEST_H_
#define MANTID_CURVEFITTING_PRODUCTQUADRATICEXPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/ProductQuadraticExp.h"

using Mantid::CurveFitting::ProductQuadraticExp;

class ProductQuadraticExpTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProductQuadraticExpTest *createSuite() { return new ProductQuadraticExpTest(); }
  static void destroySuite( ProductQuadraticExpTest *suite ) { delete suite; }


  void test_name()
  {
    ProductQuadraticExp func;
    TS_ASSERT_EQUALS("ProductQuadraticExp", func.name());
  }

  void test_catagory()
  {
    ProductQuadraticExp func;
    TS_ASSERT_EQUALS("Calibrate", func.category());
  }

  void test_set_parameters()
  {
    const double A0 = 1;
    const double A1 = 2;
    const double A2 = 3;
    const double Height = 4;
    const double Lifetime = 0.1;
    
    ProductQuadraticExp func;
    func.setParameter("A0", A0);
    func.setParameter("A1", A1);
    func.setParameter("A2", A2);
    func.setParameter("Height", Height);
    func.setParameter("Lifetime", Lifetime);

    TS_ASSERT_EQUALS(A0, func.getParameter("A0"));
    TS_ASSERT_EQUALS(A1, func.getParameter("A1"));
    TS_ASSERT_EQUALS(A2, func.getParameter("A2"));
    TS_ASSERT_EQUALS(Height, func.getParameter("Height"));
    TS_ASSERT_EQUALS(Lifetime, func.getParameter("Lifetime"));
  }



};


#endif /* MANTID_CURVEFITTING_PRODUCTQUADRATICEXPTEST_H_ */