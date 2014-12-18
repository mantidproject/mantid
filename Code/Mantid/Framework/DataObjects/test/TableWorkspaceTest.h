#ifndef TESTTABLEWORKSPACE_
#define TESTTABLEWORKSPACE_

#include <vector> 
#include <algorithm> 
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/TableWorkspace.h" 
#include "MantidAPI/TableRow.h" 
#include "MantidAPI/ColumnFactory.h" 
#include "MantidAPI/WorkspaceProperty.h"

#include <limits>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace std;


template<class T> 
class TableColTestHelper:public TableColumn<T>
{
public:
    TableColTestHelper<T>(T value){
        std::vector<T> &dat = TableColumn<T>::data();
        dat.resize(1);
        dat[0]=value;
    }

};

class TableWorkspaceTest : public CxxTest::TestSuite
{
public:
  void testTCcast(){
      TableColTestHelper<float> Tcf(1.);
      float frez=(float)Tcf[0];
      TSM_ASSERT_DELTA("1 float not converted, type: "+std::string(typeid(frez).name()),1.,frez,1.e-5);

      TableColTestHelper<double> Tdr(1.);
      double drez=(double)Tcf[0];
      TSM_ASSERT_DELTA("2 double not converted, type: "+std::string(typeid(drez).name()),1.,drez,1.e-5);

      TableColTestHelper<int> Tci(1);
      int irez=(int)Tci[0];
      TSM_ASSERT_EQUALS("3 integer not converted, type: "+std::string(typeid(irez).name()),1,irez);

      TableColTestHelper<int64_t> Tcl(1);
      int64_t lrez=(int64_t)Tcl[0];
      TSM_ASSERT_EQUALS("4 int64_t not converted, type: "+std::string(typeid(lrez).name()),1,lrez);

      TableColTestHelper<long> Tcls(1);
      long  lsrez = (long)Tcls[0];
      TSM_ASSERT_EQUALS("5 long not converted, type: "+std::string(typeid(lsrez).name()),1,lsrez);

     TableColTestHelper<size_t> Tcst(1);
      size_t  strez = (size_t)Tcst[0];
      TSM_ASSERT_EQUALS("6 size_t not converted, type: "+std::string(typeid(strez).name()),1,lsrez);

//
      TableColTestHelper<float> Tcf2(-1.);
      frez=(float)Tcf2[0];
      TSM_ASSERT_DELTA("7 float not converted, type: "+std::string(typeid(float).name()),-1.,frez,1.e-5);

      TableColTestHelper<double> Tdr2(-1.);
      drez=(double)Tcf2[0];
      TSM_ASSERT_DELTA("8 double not converted, type: "+std::string(typeid(double).name()),-1.,drez,1.e-5);

      TableColTestHelper<int> Tci2(-1);
      irez=(int)Tci2[0];
      TSM_ASSERT_EQUALS("9 integer not converted, type: "+std::string(typeid(int).name()),-1,irez);

      TableColTestHelper<int64_t> Tcl2(-1);
      lrez=(int64_t)Tcl2[0];
      TSM_ASSERT_EQUALS("10 int64_t not converted, type: "+std::string(typeid(int64_t).name()),-1,lrez);

      TableColTestHelper<long> Tcls2(-1);
      lsrez = (long)Tcls2[0];
      TSM_ASSERT_EQUALS("11 long not converted, type: "+std::string(typeid(long).name()),-1,lsrez);

  }
  void testAll()
  {
    TableWorkspace tw(3);
    Column_sptr intCol = tw.addColumn("int","Number");
    Column_sptr strCol = tw.addColumn("str","Name");
    Column_sptr v3dCol = tw.addColumn("V3D","Position");

    TS_ASSERT_EQUALS(tw.rowCount(),3);
    TS_ASSERT_EQUALS(tw.columnCount(),3);

    TS_ASSERT_EQUALS(tw.getColumn("Number"),intCol);
    TS_ASSERT_EQUALS(tw.getColumn("Name"),strCol);
    TS_ASSERT_EQUALS(tw.getColumn("Position"),v3dCol);
    // Test trying to add existing column returns null pointer
    TS_ASSERT( ! tw.addColumn("int","Number") )

    tw.getRef<int>("Number",1) = 17;
    tw.cell<std::string>(2,1) = "STRiNG";

    ColumnVector<int> cNumb = tw.getVector("Number");
    TS_ASSERT_EQUALS(cNumb[1],17);

    ColumnVector<string> str = tw.getVector("Name");
    TS_ASSERT_EQUALS(str.size(),3);
    TS_ASSERT_EQUALS(str[2],"STRiNG");

    for(int i=0;i<static_cast<int>(cNumb.size());i++)
        cNumb[i] = i+1;

    tw.insertRow(2);
    cNumb[2] = 4;
    TS_ASSERT_EQUALS(tw.rowCount(),4);
    TS_ASSERT_EQUALS(cNumb[3],3);

    tw.setRowCount(10);
    TS_ASSERT_EQUALS(tw.rowCount(),10);
    TS_ASSERT_EQUALS(cNumb[3],3);

    tw.removeRow(3);
    TS_ASSERT_EQUALS(tw.rowCount(),9);
    TS_ASSERT_EQUALS(cNumb[3],0);

    tw.setRowCount(2);
    TS_ASSERT_EQUALS(tw.rowCount(),2);
    TS_ASSERT_EQUALS(cNumb[1],2);

    //str[0] = "First"; str[1] = "Second";
    //vector<string> names;
    //names.push_back("Number");
    //names.push_back("Name");

  }

  void testRow()
  {
    TableWorkspace tw(2);
    tw.addColumn("int","Number");
    tw.addColumn("double","Ratio");
    tw.addColumn("str","Name");
    tw.addColumn("bool","OK");

    TableRow row = tw.getFirstRow();

    TS_ASSERT_EQUALS(row.row(),0);

    row << 18 << 3.14 << "FIRST";

    TS_ASSERT_EQUALS(tw.Int(0,0),18);
    TS_ASSERT_EQUALS(tw.Double(0,1),3.14);
    TS_ASSERT_EQUALS(tw.String(0,2),"FIRST");

    if (row.next())
    {
        row << 36 << 6.28 << "SECOND";
    }

    int i;
    double r;
    std::string str;
    row.row(1);
    row>>i>>r>>str;

    TS_ASSERT_EQUALS(i,36);
    TS_ASSERT_EQUALS(r,6.28);
    TS_ASSERT_EQUALS(str,"SECOND");

    for(int i=0;i<5;i++)
    {
        TableRow row = tw.appendRow();
        size_t j = row.row();
        std::ostringstream ostr;
        ostr<<"Number "<<j;
        row << int(18*j) << 3.14*double(j) << ostr.str() << (j%2 == 0);
    }

    TS_ASSERT_EQUALS(tw.rowCount(),7);

    TableRow row1 = tw.getRow(2);
    TS_ASSERT_EQUALS(row1.row(),2);

    do
    {
        TS_ASSERT_EQUALS(row1.Int(0),row1.row()*18);
        TS_ASSERT_EQUALS(row1.Double(1),static_cast<double>(row1.row())*3.14);
        std::istringstream istr(row1.String(2));
        std::string str;
        int j;
        istr>>str>>j;
        TS_ASSERT_EQUALS(str,"Number");
        TS_ASSERT_EQUALS(j,row1.row());
        row1.Bool(3) = !tw.Bool(row1.row(),3);
        TS_ASSERT_EQUALS(tw.Bool(row1.row(),3), static_cast<Mantid::API::Boolean>(row1.row()%2 != 0));
    }while(row1.next());

  }

  void testOutOfRange()
  {
      TableWorkspace tw(2);
      tw.addColumn("str","Name");
      tw.addColumn("int","Number");
      TS_ASSERT_THROWS(tw.String(0,1),std::runtime_error);
      TS_ASSERT_THROWS(tw.Int(0,3),std::range_error);
      TS_ASSERT_THROWS(tw.Int(3,1),std::range_error);

      {
        TableRow row = tw.appendRow();
        TS_ASSERT_THROWS(row << "One" << 1 << 2,std::range_error);
      }

      {
        std::string str;
        int i;
        double d;
        TableRow row = tw.getFirstRow();
        TS_ASSERT_THROWS(row >> str >> i >> d,std::range_error);
      }

      {
        TableRow row = tw.getFirstRow();
        TS_ASSERT_THROWS(row.row(3),std::range_error);
      }
  }

  void testBoolean()
  {
  try
  {
      TableWorkspace tw(10);
      tw.addColumn("int","Number");
      tw.addColumn("bool","OK");

      TableRow row = tw.getFirstRow();
      do
      {
          size_t i = row.row();
          row << i << (i % 2 == 0);
      }
      while(row.next());

      TableColumn_ptr<bool> bc = tw.getColumn("OK");

      //std::vector<bool>& bv = bc->data();  // doesn't work
      //std::vector<Boolean>& bv = bc->data();  // works
      //bool& br = bc->data()[1]; // doesn't work
      //bool b = bc->data()[1]; // works
      bc->data()[1] = true;

  }
  catch(std::exception& e)
  {
      std::cerr<<"Error: "<<e.what()<<'\n';
  }
  }
  void testFindMethod()
  {
      TableWorkspace tw;
      tw.addColumn("str","Name");
      tw.addColumn("str","Format");
      tw.addColumn("str","Format Version");
      tw.addColumn("str","Format Type");
      tw.addColumn("str","Create Time");
     
       for (int i=1;i<10;++i)
      {
         std::stringstream s;
         TableRow t = tw.appendRow();
          s<<i;
          std::string name="Name";
          name+=s.str();
         
          t<<name;
          std::string format="Format";
          format+=s.str();
          t<<format;
          std::string formatver="Format Version"; 
          formatver+=s.str();
          t<<formatver;
          std::string formattype="Format Type";
          formattype+=s.str();
          t<<formattype;
          std::string creationtime="Creation Time";
          creationtime+=s.str();
          t<<creationtime;

      }
      std::string searchstr="Name3";
      size_t row=0;
      const size_t col=0;
      tw.find(searchstr,row,col);
      TS_ASSERT_EQUALS(row,2);

      const int formatcol=2;
      searchstr="Format Version8";
      tw.find(searchstr,row,formatcol);
      TS_ASSERT_EQUALS(row,7);

  }

  void testClone()
  {
    TableWorkspace tw(1);
      tw.addColumn("str","X");
      tw.addColumn("str","Y");
      tw.addColumn("str","Z");
   
    tw.getColumn(0)->cell<std::string>(0) = "a";
    tw.getColumn(1)->cell<std::string>(0) = "b";
    tw.getColumn(2)->cell<std::string>(0) = "c";

    boost::scoped_ptr<TableWorkspace> cloned(tw.clone());

    //Check clone is same as original.
    TS_ASSERT_EQUALS(tw.columnCount(), cloned->columnCount());
    TS_ASSERT_EQUALS(tw.rowCount(), cloned->rowCount());
    TS_ASSERT_EQUALS("a", cloned->getColumn(0)->cell<std::string>(0));
    TS_ASSERT_EQUALS("b", cloned->getColumn(1)->cell<std::string>(0));
    TS_ASSERT_EQUALS("c", cloned->getColumn(2)->cell<std::string>(0));

  }

  void test_toDouble()
  {
    TableWorkspace tw(1);
      tw.addColumn("int","X");
      tw.addColumn("float","Y");
      tw.addColumn("double","Z");
      tw.addColumn("bool","F");
      tw.addColumn("bool","T");
      tw.addColumn("str","S");

      TableRow row = tw.getFirstRow();
      row << int(12) << float(25.1) << double(123.456) << false << true << std::string("hello");

      double d = 100.0;
      TS_ASSERT_THROWS_NOTHING(d = tw.getColumn("X")->toDouble(0));
      TS_ASSERT_EQUALS( d, 12.0 );
      TS_ASSERT_THROWS_NOTHING(d = tw.getColumn("Y")->toDouble(0));
      TS_ASSERT_DELTA( d, 25.1 , 1e-6); // accuracy of float
      TS_ASSERT_THROWS_NOTHING(d = tw.getColumn("Z")->toDouble(0));
      TS_ASSERT_EQUALS( d, 123.456 );
      TS_ASSERT_THROWS_NOTHING(d = tw.getColumn("F")->toDouble(0));
      TS_ASSERT_EQUALS( d, 0.0 );
      TS_ASSERT_THROWS_NOTHING(d = tw.getColumn("T")->toDouble(0));
      TS_ASSERT_EQUALS( d, 1.0 );
      TS_ASSERT_THROWS(d = tw.getColumn("S")->toDouble(0),std::runtime_error);

  }
  void testGetVectorSetVectorValues()
  {

    TableWorkspace tw(3);
    tw.addColumn("size_t","SizeT");
    tw.addColumn("double","Double");
    tw.addColumn("str","String");


    auto &SizeTData = tw.getColVector<size_t>("SizeT");
    TS_ASSERT_THROWS(tw.getColVector<int>("Double"),std::runtime_error);
    std::vector<double> &DoublData = tw.getColVector<double>("Double");
    std::vector<std::string> &StrData = tw.getColVector<std::string>("String");

    SizeTData[0] = 10;
    SizeTData[1] = 20;
    SizeTData[2] = 30;
    DoublData[0] = 100.;
    DoublData[1] = 200.;
    DoublData[2] = 300.;

    StrData[0] = "1";
    StrData[1] = "2";
    StrData[2] = "3";

    auto SizeTDataI = tw.getColVector<size_t>(0);
    TS_ASSERT_THROWS(tw.getColVector<int>(1),std::runtime_error);
    auto DoublDataI = tw.getColVector<double>(1);
    auto StrDataI = tw.getColVector<std::string>(2);

    for(size_t i=0;i<3;i++)
    {
      TS_ASSERT_EQUALS(SizeTData[i],SizeTDataI[i]);
      TS_ASSERT_EQUALS(DoublData[i],DoublDataI[i]);
      TS_ASSERT_EQUALS(StrData[i],StrDataI[i]);
    }

  }

  void testGetColDataArray()
  {
     TableWorkspace tw(3);
     tw.addColumn("float","MyFloatData");

     double *pdData =tw.getColDataArray<double>("MyFloatData");
     TS_ASSERT(!pdData);
     float *pfData =tw.getColDataArray<float>("NonExistingColumn");
     TS_ASSERT(!pfData);


     float *pData = tw.getColDataArray<float>("MyFloatData");
     TS_ASSERT(pData);
     for(int i=0;i<3;i++)
       *(pData+i) = float(i+10);

      std::vector<float> &MyFloats = tw.getColVector<float>("MyFloatData");
      for(int i=0;i<3;i++)
      {
        TS_ASSERT_DELTA(*(pData+i),MyFloats[i],1.e-6);
      }
  }

 /* void xestGetColDataArrayBool()
  {
  Does not work and probably should not 
     TableWorkspace tw(3);
     tw.addColumn("bool","MyBoolData");

     bool *pbData =tw.getColDataArray<bool>("MyBoolData");
     TS_ASSERT(pbData);


     for(int i=0;i<3;i++)
       *(pbData+i) = true;

      std::vector<bool> &MyBools = tw.getColVector<bool>("MyBoolData");
      for(int i=0;i<3;i++)
      {
        TS_ASSERT_EQUALS(*(pbData+i),MyBools[i]);
      }
  }*/

  void testAddProperty()
  {
    TableWorkspace tw(3);
    TS_ASSERT_THROWS_NOTHING(tw.logs()->addProperty("SomeInt",int(10)));
    TS_ASSERT_EQUALS(10,tw.getLogs()->getPropertyValueAsType<int>("SomeInt"));

    TS_ASSERT_THROWS_NOTHING(tw.logs()->addProperty<double>("SomeDouble",100));
    TS_ASSERT_DELTA(100,tw.getLogs()->getPropertyValueAsType<double>("SomeDouble"),1.e-7);
  }

  void test_known_to_property_for_unmangling()
  {
    Mantid::API::WorkspaceProperty<TableWorkspace> property("DummyProperty", "DummyWorkspace", Mantid::Kernel::Direction::Input);
    TS_ASSERT_EQUALS("TableWorkspace", Mantid::Kernel::getUnmangledTypeName(*property.type_info()));
  }

  void test_sort()
  {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("int","col1");
    ws.addColumn("str","col2");
    ws.addColumn("double","col3");
    auto &data1 = static_cast<TableColumn<int>&>(*ws.getColumn("col1")).data();
    auto &data2 = static_cast<TableColumn<std::string>&>(*ws.getColumn("col2")).data();
    auto &data3 = static_cast<TableColumn<double>&>(*ws.getColumn("col3")).data();

    data1[0] = 3;  data2[0] = "three (3)";  data3[0] = 0.0;
    data1[1] = 1;  data2[1] = "one (3)";  data3[1] = 1.0;
    data1[2] = 1;  data2[2] = "one (2)";  data3[2] = 2.0;
    data1[3] = 2;  data2[3] = "two (1)";  data3[3] = 3.0;
    data1[4] = 3;  data2[4] = "three (2)";  data3[4] = 4.0;
    data1[5] = 3;  data2[5] = "three (2)";  data3[5] = 5.0;
    data1[6] = 2;  data2[6] = "two (2)";  data3[6] = 6.0;
    data1[7] = 1;  data2[7] = "one (1)";  data3[7] = 7.0;
    data1[8] = 2;  data2[8] = "two (1)";  data3[8] = 8.0;
    data1[9] = 2;  data2[9] = "two (2)";  data3[9] = 9.0;

    std::vector<std::pair<std::string, bool>> criteria(3);
    criteria[0] = std::make_pair("col1",true);
    criteria[1] = std::make_pair("col2",true);
    criteria[2] = std::make_pair("col3",false);

    ws.sort( criteria );

    TS_ASSERT_EQUALS( data1[0], 1 );
    TS_ASSERT_EQUALS( data1[1], 1 );
    TS_ASSERT_EQUALS( data1[2], 1 );
    TS_ASSERT_EQUALS( data1[3], 2 );
    TS_ASSERT_EQUALS( data1[4], 2 );
    TS_ASSERT_EQUALS( data1[5], 2 );
    TS_ASSERT_EQUALS( data1[6], 2 );
    TS_ASSERT_EQUALS( data1[7], 3 );
    TS_ASSERT_EQUALS( data1[8], 3 );
    TS_ASSERT_EQUALS( data1[9], 3 );

    TS_ASSERT_EQUALS( data2[0], "one (1)" );
    TS_ASSERT_EQUALS( data2[1], "one (2)" );
    TS_ASSERT_EQUALS( data2[2], "one (3)" );
    TS_ASSERT_EQUALS( data2[3], "two (1)" );
    TS_ASSERT_EQUALS( data2[4], "two (1)" );
    TS_ASSERT_EQUALS( data2[5], "two (2)" );
    TS_ASSERT_EQUALS( data2[6], "two (2)" );
    TS_ASSERT_EQUALS( data2[7], "three (2)" );
    TS_ASSERT_EQUALS( data2[8], "three (2)" );
    TS_ASSERT_EQUALS( data2[9], "three (3)" );

    TS_ASSERT_EQUALS( data3[0], 7);
    TS_ASSERT_EQUALS( data3[1], 2);
    TS_ASSERT_EQUALS( data3[2], 1);
    TS_ASSERT_EQUALS( data3[3], 8);
    TS_ASSERT_EQUALS( data3[4], 3);
    TS_ASSERT_EQUALS( data3[5], 9);
    TS_ASSERT_EQUALS( data3[6], 6);
    TS_ASSERT_EQUALS( data3[7], 5);
    TS_ASSERT_EQUALS( data3[8], 4);
    TS_ASSERT_EQUALS( data3[9], 0);


  }

  void test_sort_1()
  {
    const size_t n = 10;
    TableWorkspace ws(n);
    ws.addColumn("int","col1");
    ws.addColumn("str","col2");
    ws.addColumn("double","col3");
    auto &data1 = static_cast<TableColumn<int>&>(*ws.getColumn("col1")).data();
    auto &data2 = static_cast<TableColumn<std::string>&>(*ws.getColumn("col2")).data();
    auto &data3 = static_cast<TableColumn<double>&>(*ws.getColumn("col3")).data();

    data1[0] = 3;  data2[0] = "three (3)";  data3[0] = 0.0;
    data1[1] = 1;  data2[1] = "one (3)";  data3[1] = 1.0;
    data1[2] = 1;  data2[2] = "one (2)";  data3[2] = 2.0;
    data1[3] = 2;  data2[3] = "two (1)";  data3[3] = 3.0;
    data1[4] = 3;  data2[4] = "three (2)";  data3[4] = 4.0;
    data1[5] = 3;  data2[5] = "three (2)";  data3[5] = 5.0;
    data1[6] = 2;  data2[6] = "two (2)";  data3[6] = 6.0;
    data1[7] = 1;  data2[7] = "one (1)";  data3[7] = 7.0;
    data1[8] = 2;  data2[8] = "two (1)";  data3[8] = 8.0;
    data1[9] = 2;  data2[9] = "two (2)";  data3[9] = 9.0;

    std::vector<std::pair<std::string, bool>> criteria(3);
    criteria[0] = std::make_pair("col1",true);
    criteria[1] = std::make_pair("col2",false);
    criteria[2] = std::make_pair("col3",true);

    ws.sort( criteria );

    TS_ASSERT_EQUALS( data1[0], 1 );
    TS_ASSERT_EQUALS( data1[1], 1 );
    TS_ASSERT_EQUALS( data1[2], 1 );
    TS_ASSERT_EQUALS( data1[3], 2 );
    TS_ASSERT_EQUALS( data1[4], 2 );
    TS_ASSERT_EQUALS( data1[5], 2 );
    TS_ASSERT_EQUALS( data1[6], 2 );
    TS_ASSERT_EQUALS( data1[7], 3 );
    TS_ASSERT_EQUALS( data1[8], 3 );
    TS_ASSERT_EQUALS( data1[9], 3 );

    TS_ASSERT_EQUALS( data2[0], "one (3)" );
    TS_ASSERT_EQUALS( data2[1], "one (2)" );
    TS_ASSERT_EQUALS( data2[2], "one (1)" );
    TS_ASSERT_EQUALS( data2[3], "two (2)" );
    TS_ASSERT_EQUALS( data2[4], "two (2)" );
    TS_ASSERT_EQUALS( data2[5], "two (1)" );
    TS_ASSERT_EQUALS( data2[6], "two (1)" );
    TS_ASSERT_EQUALS( data2[7], "three (3)" );
    TS_ASSERT_EQUALS( data2[8], "three (2)" );
    TS_ASSERT_EQUALS( data2[9], "three (2)" );

    TS_ASSERT_EQUALS( data3[0], 1);
    TS_ASSERT_EQUALS( data3[1], 2);
    TS_ASSERT_EQUALS( data3[2], 7);
    TS_ASSERT_EQUALS( data3[3], 6);
    TS_ASSERT_EQUALS( data3[4], 9);
    TS_ASSERT_EQUALS( data3[5], 3);
    TS_ASSERT_EQUALS( data3[6], 8);
    TS_ASSERT_EQUALS( data3[7], 0);
    TS_ASSERT_EQUALS( data3[8], 4);
    TS_ASSERT_EQUALS( data3[9], 5);

  }

  void test_sort_empty()
  {
    TableWorkspace ws;
    ws.addColumn("int","col1");
    ws.addColumn("str","col2");
    ws.addColumn("double","col3");

    TS_ASSERT_EQUALS( ws.rowCount(), 0 );

    std::vector<std::pair<std::string, bool>> criteria(3);
    criteria[0] = std::make_pair("col1",true);
    criteria[1] = std::make_pair("col2",false);
    criteria[2] = std::make_pair("col3",true);

    TS_ASSERT_THROWS_NOTHING( ws.sort(criteria) );

  }

  void test_sort_almost_empty()
  {
    TableWorkspace ws(1);
    ws.addColumn("int","col1");
    ws.addColumn("str","col2");
    ws.addColumn("double","col3");
    auto &data1 = static_cast<TableColumn<int>&>(*ws.getColumn("col1")).data();
    auto &data2 = static_cast<TableColumn<std::string>&>(*ws.getColumn("col2")).data();
    auto &data3 = static_cast<TableColumn<double>&>(*ws.getColumn("col3")).data();
    data1[0] = 3;  data2[0] = "hello";  data3[0] = 5.0;

    std::vector<std::pair<std::string, bool>> criteria(3);
    criteria[0] = std::make_pair("col1",true);
    criteria[1] = std::make_pair("col2",false);
    criteria[2] = std::make_pair("col3",true);

    TS_ASSERT_THROWS_NOTHING( ws.sort(criteria) );

    TS_ASSERT_EQUALS( data1[0], 3 );
    TS_ASSERT_EQUALS( data2[0], "hello" );
    TS_ASSERT_EQUALS( data3[0], 5 );

  }

};


#endif /*TESTTABLEWORKSPACE_*/
