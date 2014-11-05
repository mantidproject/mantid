#ifndef MANTID_DATAOBJECTS_TABLECOLUMNTEST_H_
#define MANTID_DATAOBJECTS_TABLECOLUMNTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/TableColumn.h"
#include "MantidDataObjects/TableWorkspace.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <boost/make_shared.hpp>
#include <iostream>

using namespace Mantid::DataObjects;

class TableColumnTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TableColumnTest *createSuite() { return new TableColumnTest(); }
  static void destroySuite( TableColumnTest *suite ) { delete suite; }

  void test_sortIndex()
  {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("int","col");
    TableColumn<int>& column = static_cast<TableColumn<int>&>(*ws.getColumn("col"));
    std::cerr << "Col size=" << column.size() << std::endl;
    auto &data = column.data();
    data[0] = 5;
    data[1] = 7;
    data[2] = 3;
    data[3] = 12;
    data[4] = 1;
    data[5] = 6;
    data[6] = 3;
    data[7] = 2;
    data[8] = 0;
    data[9] = 12;

    auto indexVec = makeIndexVector( column.size() );
    std::vector<std::pair<size_t,size_t>> eqRanges;
    column.sortIndex( 0, column.size(), indexVec, eqRanges );

    for(size_t i = 0; i < n; ++i)
    {
      std::cerr << i << ' ' << indexVec[i] << ' ' << data[indexVec[i]] << std::endl;
    }

    TS_ASSERT_EQUALS( data[0], 5.0 );
    TS_ASSERT_EQUALS( data[1], 7.0 );
    TS_ASSERT_EQUALS( data[2], 3.0 );
    TS_ASSERT_EQUALS( data[3], 12.0 );
    TS_ASSERT_EQUALS( data[4], 1.0 );
    TS_ASSERT_EQUALS( data[5], 6.0 );
    TS_ASSERT_EQUALS( data[6], 3.0 );
    TS_ASSERT_EQUALS( data[7], 2.0 );
    TS_ASSERT_EQUALS( data[8], 0.0 );
    TS_ASSERT_EQUALS( data[9], 12.0 );

    TS_ASSERT_EQUALS( data[indexVec[0]], 0 );
    TS_ASSERT_EQUALS( data[indexVec[1]], 1 );
    TS_ASSERT_EQUALS( data[indexVec[2]], 2 );
    TS_ASSERT_EQUALS( data[indexVec[3]], 3 );
    TS_ASSERT_EQUALS( data[indexVec[4]], 3 );
    TS_ASSERT_EQUALS( data[indexVec[5]], 5 );
    TS_ASSERT_EQUALS( data[indexVec[6]], 6 );
    TS_ASSERT_EQUALS( data[indexVec[7]], 7 );
    TS_ASSERT_EQUALS( data[indexVec[8]], 12 );
    TS_ASSERT_EQUALS( data[indexVec[9]], 12 );

    TS_ASSERT_EQUALS( eqRanges.size(), 2 );
  }

private:

  std::vector<size_t> makeIndexVector(size_t n)
  {
    std::vector<size_t> vec(n);
    for(auto i = vec.begin()+1; i != vec.end(); ++i)
    {
      *i = *(i-1) + 1;
    }
    return vec;
  }

};


#endif /* MANTID_DATAOBJECTS_TABLECOLUMNTEST_H_ */

