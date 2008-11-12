#ifndef TESTTABLEWORKSPACE_
#define TESTTABLEWORKSPACE_

#include <vector> 
#include <algorithm> 
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/TableWorkspace.h" 
#include "MantidDataObjects/ColumnFactory.h" 

class Class
{
public:
    int d;
    Class():d(0){} 
private:
    Class(const Class&);
};

DECLARE_TABLEPOINTERCOLUMN(Class,Class);

using namespace Mantid::DataObjects;
using namespace std;

class TableWorkspaceTest : public CxxTest::TestSuite
{
public:
  void testAll()
  {
    TableWorkspace tw(3);
    tw.createColumn("int","Number");
    tw.createColumn("str","Name");
    tw.createColumn("V3D","Position");
    tw.createColumn("Class","class");

    TS_ASSERT_EQUALS(tw.rowCount(),3)
    TS_ASSERT_EQUALS(tw.columnCount(),4)

    vector<int>& stdNumb = tw.getStdVector<int>("Number");
    TS_ASSERT_EQUALS(stdNumb.size(),3)
    stdNumb[1] = 17;
    ColumnVector<int> cNumb = tw.getVector("Number");
    TS_ASSERT_EQUALS(cNumb[1],17)

    ColumnVector<string> str = tw.getVector("Name");
    TS_ASSERT_EQUALS(str.size(),3)

    ColumnPointerVector<Class> cl = tw.getVector("class");
    TS_ASSERT_EQUALS(cl.size(),3)

    for(int i=0;i<cNumb.size();i++)
        cNumb[i] = i+1;

    tw.insertRow(2);
    cNumb[2] = 4;
    TS_ASSERT_EQUALS(tw.rowCount(),4)
    TS_ASSERT_EQUALS(cNumb[3],3)

    tw.setRowCount(10);
    TS_ASSERT_EQUALS(tw.rowCount(),10)
    TS_ASSERT_EQUALS(cNumb[3],3)

    tw.removeRow(3);
    TS_ASSERT_EQUALS(tw.rowCount(),9)
    TS_ASSERT_EQUALS(cNumb[3],0)

    tw.setRowCount(2);
    TS_ASSERT_EQUALS(tw.rowCount(),2)
    TS_ASSERT_EQUALS(cNumb[1],2)

    str[0] = "First"; str[1] = "Second";
    cl[0].d = 11; cl[1].d = 22;

    vector<string> names;
    names.push_back("Number");
    names.push_back("Name");
    names.push_back("class");

    boost::tuples::tuple<int,string,Class> tup1;
    tw.set_Tuple(1,tup1,names);

    boost::tuples::tuple<int*,string*,Class*> tup2 =
        tw.make_TupleRef< boost::tuples::tuple<int*,string*,Class*> >(1,names);

    boost::tuples::tuple<int*,string*,Class*> tup3;
    tw.set_TupleRef(1,tup3,names);

    *tup2.get<0>() = 200; 
    *tup2.get<1>() = "End";
    tup2.get<2>()->d = 222;

    TS_ASSERT_EQUALS(*tup3.get<0>(),200)
    TS_ASSERT_EQUALS(*tup3.get<1>(),"End")
    TS_ASSERT_EQUALS(tup3.get<2>()->d,222)

    TS_ASSERT_EQUALS(tup1.get<0>(),2)
    TS_ASSERT_EQUALS(tup1.get<1>(),"Second")
    TS_ASSERT_EQUALS(tup1.get<2>().d,22)

  }



};
#endif /*TESTTABLEWORKSPACE_*/
