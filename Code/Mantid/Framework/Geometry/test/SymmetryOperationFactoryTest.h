#ifndef MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Exception.h"

#include <boost/lexical_cast.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;


class SymmetryOperationFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SymmetryOperationFactoryTest *createSuite() { return new SymmetryOperationFactoryTest(); }
  static void destroySuite( SymmetryOperationFactoryTest *suite ) { delete suite; }

  SymmetryOperationFactoryTest()
  {
      SymmetryOperationFactory::Instance().subscribeSymOp("x,y,z");
  }

  ~SymmetryOperationFactoryTest()
  {
      SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z");
  }


  void testCreateSymOp()
  {
      TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
      TS_ASSERT_THROWS(SymmetryOperationFactory::Instance().createSymOp("fake2"), Mantid::Kernel::Exception::ParseError);
  }

  void testUnsubscribe()
  {
      TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));

      TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z"));
      TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"), false);
      TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));

      TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().subscribeSymOp("x,y,z"));
  }
};


#endif /* MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_ */
