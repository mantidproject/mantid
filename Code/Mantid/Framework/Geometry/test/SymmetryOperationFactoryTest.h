#ifndef MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidKernel/Matrix.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

/* A fake symmetry operation for testing the factory
 * without interfering with other tests.
 */
class TestSymmetryOperation : public SymmetryOperation
{
public:
    TestSymmetryOperation() : SymmetryOperation(2, IntMatrix(3, 3, true), "fake")
    {}
    ~TestSymmetryOperation() { }
};

class SymmetryOperationFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SymmetryOperationFactoryTest *createSuite() { return new SymmetryOperationFactoryTest(); }
  static void destroySuite( SymmetryOperationFactoryTest *suite ) { delete suite; }

  SymmetryOperationFactoryTest()
  {
      SymmetryOperationFactory::Instance().subscribeSymOp<TestSymmetryOperation>();
  }

  ~SymmetryOperationFactoryTest()
  {
      SymmetryOperationFactory::Instance().unsubscribeSymOp("fake");
  }


  void testCreateSymOp()
  {
      TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().createSymOp("fake"));
      TS_ASSERT_THROWS(SymmetryOperationFactory::Instance().createSymOp("fake2"), Mantid::Kernel::Exception::NotFoundError);
  }

  void testUnsubscribe()
  {
      TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().createSymOp("fake"));

      SymmetryOperationFactory::Instance().unsubscribeSymOp("fake");
      TS_ASSERT_THROWS(SymmetryOperationFactory::Instance().createSymOp("fake"), Mantid::Kernel::Exception::NotFoundError);

      SymmetryOperationFactory::Instance().subscribeSymOp<TestSymmetryOperation>();
      TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().createSymOp("fake"));
  }

};


#endif /* MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_ */
