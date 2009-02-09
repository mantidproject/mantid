#ifndef TESTTABLEWORKSPACE_
#define TESTTABLEWORKSPACE_

#include <vector> 
#include <algorithm> 
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/TableWorkspace.h" 
#include "MantidDataObjects/TableRow.h" 
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

    /*boost::tuples::tuple<int,string,Class> tup1;
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
    TS_ASSERT_EQUALS(tup1.get<2>().d,22)*/

  }

  void testRow()
  {
    TableWorkspace tw(2);
    tw.createColumn("int","Number");
    tw.createColumn("double","Ratio");
    tw.createColumn("str","Name");
    tw.createColumn("bool","OK");

    TableRow row = tw.getFirstRow();

    TS_ASSERT_EQUALS(row.row(),0)

    row << 18 << 3.14 << "FIRST";

    TS_ASSERT_EQUALS(tw.Int(0,0),18)
    TS_ASSERT_EQUALS(tw.Double(0,1),3.14)
    TS_ASSERT_EQUALS(tw.String(0,2),"FIRST")

    if (row.next())
    {
        row << 36 << 6.28 << "SECOND";
    }

    int i;
    double r;
    std::string str;
    row.row(1);
    row>>i>>r>>str;

    TS_ASSERT_EQUALS(i,36)
    TS_ASSERT_EQUALS(r,6.28)
    TS_ASSERT_EQUALS(str,"SECOND")

    for(int i=0;i<5;i++)
    {
        TableRow row = tw.appendRow();
        int j = row.row();
        std::ostringstream ostr;
        ostr<<"Number "<<j;
        row << 18*j << 3.14*j << ostr.str() << (j%2 == 0);
    }

    TS_ASSERT_EQUALS(tw.rowCount(),7)

    TableRow row1 = tw.getRow(2);
    TS_ASSERT_EQUALS(row1.row(),2)

    do
    {
        TS_ASSERT_EQUALS(row1.Int(0),row1.row()*18)
        TS_ASSERT_EQUALS(row1.Double(1),row1.row()*3.14)
        std::istringstream istr(row1.String(2));
        std::string str;
        int j;
        istr>>str>>j;
        TS_ASSERT_EQUALS(str,"Number")
        TS_ASSERT_EQUALS(j,row1.row())
        row1.Bool(3) = !tw.Bool(row1.row(),3);
        TS_ASSERT_EQUALS(tw.Bool(row1.row(),3), row1.row()%2 != 0)
    }while(row1.next());

  }

  void testBoolean()
  {
  try
  {
      TableWorkspace tw(10);
      tw.createColumn("int","Number");
      tw.createColumn("bool","OK");

      TableRow row = tw.getFirstRow();
      do
      {
          int i = row.row();
          row << i << (i % 2 == 0);
      }
      while(row.next());

      TableColumn_ptr<bool> bc = tw.getColumn("OK");

      // std::vector<bool>& bv = tw.getStdVector<bool>("OK"); // doesn't work
      //std::vector<bool>& bv = bc->data();  // doesn't work
      std::vector<Boolean>& bv = bc->data();  // works
      //bool& br = bc->data()[1]; // doesn't work
      bool b = bc->data()[1]; // works
      bc->data()[1] = true;

      for(int i=0;i<tw.rowCount();i++)
      {
//          std::cerr<<bc->data()[i]<<'\n';
      }
  }
  catch(std::exception& e)
  {
      std::cerr<<"Error: "<<e.what()<<'\n';
  }
  }



};
#endif /*TESTTABLEWORKSPACE_*/
