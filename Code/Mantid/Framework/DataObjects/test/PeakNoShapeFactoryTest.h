#ifndef MANTID_DATAOBJECTS_PEAKNOSHAPEFACTORYTEST_H_
#define MANTID_DATAOBJECTS_PEAKNOSHAPEFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/PeakNoShapeFactory.h"
#include "MantidDataObjects/NoShape.h"

using namespace Mantid::DataObjects;

class PeakNoShapeFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakNoShapeFactoryTest *createSuite() { return new PeakNoShapeFactoryTest(); }
  static void destroySuite( PeakNoShapeFactoryTest *suite ) { delete suite; }


  void test_create()
  {
      PeakNoShapeFactory factory;
      PeakShape* product = factory.create("-**-");
      TS_ASSERT(dynamic_cast<NoShape*>(product));
      TS_ASSERT_EQUALS("none", product->shapeName());
      delete product;
  }


};


#endif /* MANTID_DATAOBJECTS_PEAKNOSHAPEFACTORYTEST_H_ */
