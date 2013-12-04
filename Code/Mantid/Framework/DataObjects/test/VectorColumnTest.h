#ifndef MANTID_DATAOBJECTS_VECTORCOLUMNTEST_H_
#define MANTID_DATAOBJECTS_VECTORCOLUMNTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/VectorColumn.h"

using Mantid::DataObjects::VectorColumn;

class VectorColumnTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static VectorColumnTest *createSuite() { return new VectorColumnTest(); }
  static void destroySuite( VectorColumnTest *suite ) { delete suite; }

  void test_construction()
  {
    VectorColumn<int> col;
    TS_ASSERT_EQUALS( col.type(), "vector_int"); 
  }


};


#endif /* MANTID_DATAOBJECTS_VECTORCOLUMNTEST_H_ */
