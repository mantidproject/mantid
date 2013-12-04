#ifndef MANTID_DATAOBJECTS_VECTORCOLUMNTEST_H_
#define MANTID_DATAOBJECTS_VECTORCOLUMNTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/VectorColumn.h"

using Mantid::DataObjects::VectorColumn;

template<class Type>
class VectorColumnTestHelper : public VectorColumn<Type>
{
public:
  using VectorColumn<Type>::resize;
  using VectorColumn<Type>::insert;
  using VectorColumn<Type>::remove;
};

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

  void test_read()
  {
    VectorColumnTestHelper<int> col;

    col.resize(5);

    // Simple case
    TS_ASSERT_THROWS_NOTHING( col.read(0,"1,2,3") );
    std::vector<int> v1;
    v1.push_back(1); v1.push_back(2); v1.push_back(3);
    TS_ASSERT_EQUALS( col.cell< std::vector<int> >(0), v1 );

    // Check if trimming works
    TS_ASSERT_THROWS_NOTHING( col.read(1,"  4, 5,  6") );
    std::vector<int> v2;
    v2.push_back(4); v2.push_back(5); v2.push_back(6);
    TS_ASSERT_EQUALS( col.cell< std::vector<int> >(1), v2 );

    // Single element
    TS_ASSERT_THROWS_NOTHING( col.read(2,"7") );
    std::vector<int> v3;
    v3.push_back(7);
    TS_ASSERT_EQUALS( col.cell< std::vector<int> >(2), v3 );

    // Empty string
    TS_ASSERT_THROWS_NOTHING( col.read(3,"") );
    std::vector<int> v4;
    TS_ASSERT_EQUALS( col.cell< std::vector<int> >(3), v4 );
    TS_ASSERT_EQUALS( col.cell< std::vector<int> >(4), v4 );

    // Non-convertable characters
    TS_ASSERT_THROWS( col.read(4,"1,2,a,3"), std::invalid_argument);
  }

};


#endif /* MANTID_DATAOBJECTS_VECTORCOLUMNTEST_H_ */
