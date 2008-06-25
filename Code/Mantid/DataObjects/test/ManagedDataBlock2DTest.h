#ifndef MANAGEDDATABLOCK2DTEST_H_
#define MANAGEDDATABLOCK2DTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/ManagedDataBlock2D.h"

using namespace Mantid::DataObjects;

class ManagedDataBlock2DTest : public CxxTest::TestSuite
{
public:
  ManagedDataBlock2DTest()
    : data(0,2,4,3)
  {
    std::vector<double> x(4);
    std::vector<double> xx(4);
    for (int i = 0; i < 4; ++i)
    {
      x[i] = i;
      xx[i] = i+4;
    }
    data.setX(0,x);
    data.setX(1,xx);
    
    std::vector<double> y(3);
    std::vector<double> e(3);
    std::vector<double> yy(3);
    std::vector<double> ee(3);
    for (int i = 0; i < 3; ++i)
    {
      y[i] = i*10;
      e[i] = sqrt(y[i]);
      yy[i] = i*100;
      ee[i] = sqrt(yy[i]);     
    }
    data.setData(0,y,e);
    data.setData(1,yy,ee);
  }
  
  void testConstructor()
  {
    ManagedDataBlock2D aBlock(0,2,2,2);
    TS_ASSERT_EQUALS( aBlock.minIndex(), 0 )
    TS_ASSERT( ! aBlock.hasChanges() )
    TS_ASSERT_EQUALS( aBlock.dataX(0).size(), 2 )
    TS_ASSERT_EQUALS( aBlock.dataY(0).size(), 2 )
    TS_ASSERT_EQUALS( aBlock.dataE(0).size(), 2 )
    TS_ASSERT_EQUALS( aBlock.dataX(1).size(), 2 )
    TS_ASSERT_EQUALS( aBlock.dataY(1).size(), 2 )
    TS_ASSERT_EQUALS( aBlock.dataE(1).size(), 2 )
  }
    
  void testSetX()
  {
    ManagedDataBlock2D aBlock(0,1,1,1);
    double aNumber = 5.5;
    std::vector<double> v(1, aNumber);
    TS_ASSERT_THROWS_NOTHING( aBlock.setX(0,v) )
    TS_ASSERT_EQUALS( aBlock.dataX(0)[0], aNumber )
    TS_ASSERT_THROWS( aBlock.setX(-1,v), std::range_error )
    TS_ASSERT_THROWS( aBlock.setX(1,v), std::range_error )
    TS_ASSERT( aBlock.hasChanges() )
  }
  
  void testSetData()
  {
    ManagedDataBlock2D aBlock(0,1,1,1);
    double aNumber = 9.9;
    std::vector<double> v(1, aNumber);
    double anotherNumber = 3.3;
    std::vector<double> w(1, anotherNumber);
    TS_ASSERT_THROWS_NOTHING( aBlock.setData(0,v) )
    TS_ASSERT_EQUALS( aBlock.dataY(0)[0], aNumber )    
    TS_ASSERT_THROWS( aBlock.setData(-1,v), std::range_error )
    TS_ASSERT_THROWS( aBlock.setData(1,v), std::range_error )
    
    double yetAnotherNumber = 2.25;
    v[0] = yetAnotherNumber;
    TS_ASSERT_THROWS_NOTHING( aBlock.setData(0,v,w) )
    TS_ASSERT_EQUALS( aBlock.dataY(0)[0], yetAnotherNumber )
    TS_ASSERT_EQUALS( aBlock.dataE(0)[0], anotherNumber )
    TS_ASSERT_THROWS( aBlock.setData(-1,v,w), std::range_error )
    TS_ASSERT_THROWS( aBlock.setData(1,v,w), std::range_error )
    TS_ASSERT( aBlock.hasChanges() )
  }
  
  void testDataX()
  {
    dataXTester(data);
  }
  
  void testDataY()
  {
    dataYTester(data);
  }
  
  void testDataE()
  {
    dataETester(data);
  }
  
  void testStreamOperators()
  {
    std::fstream outfile("ManagedDataBlock2DTest.tmp", std::ios::binary | std::ios::out);
    TS_ASSERT( outfile )
    outfile << data;
    outfile.close();
    
    std::fstream infile("ManagedDataBlock2DTest.tmp", std::ios::binary | std::ios::in);
    TS_ASSERT( infile )
    ManagedDataBlock2D readData(0,2,4,3);
    infile >> readData;
    // use const methods so that I can check the changes flag behaves as it should
    TS_ASSERT( ! readData.hasChanges() )
    dataXTester(readData);
    dataYTester(readData);
    dataETester(readData);
    TS_ASSERT( readData.hasChanges() )
    
    remove("ManagedDataBlock2DTest.tmp");
  }

private:
    ManagedDataBlock2D data;
    
    void dataXTester(ManagedDataBlock2D &dataToTest)
    {
      std::vector<double> x;
      TS_ASSERT_THROWS( dataToTest.dataX(-1), std::range_error )
      TS_ASSERT_THROWS_NOTHING( x = dataToTest.dataX(0) )
      std::vector<double> xx;
      TS_ASSERT_THROWS_NOTHING( xx = dataToTest.dataX(1) )
      TS_ASSERT_THROWS( dataToTest.dataX(2), std::range_error )
      TS_ASSERT_EQUALS( x.size(), 4 )
      TS_ASSERT_EQUALS( xx.size(), 4 )
      for (unsigned int i = 0; i < x.size(); ++i)
      {
        TS_ASSERT_EQUALS( x[i], i )
        TS_ASSERT_EQUALS( xx[i], i+4 )
      }
      
      // test const version
      const ManagedDataBlock2D &constRefToData = dataToTest;
      TS_ASSERT_THROWS( const std::vector<double> v = constRefToData.dataX(-1), std::range_error )
      const std::vector<double> xc = constRefToData.dataX(0);
      const std::vector<double> xxc = constRefToData.dataX(1);
      TS_ASSERT_THROWS( const std::vector<double> v = constRefToData.dataX(2), std::range_error )
      TS_ASSERT_EQUALS( xc.size(), 4 )
      TS_ASSERT_EQUALS( xxc.size(), 4 )
      for (unsigned int i = 0; i < xc.size(); ++i)
      {
        TS_ASSERT_EQUALS( xc[i], i )
        TS_ASSERT_EQUALS( xxc[i], i+4 )
      }
    }
    
    void dataYTester(ManagedDataBlock2D &dataToTest)
    {
      std::vector<double> y;
      TS_ASSERT_THROWS( dataToTest.dataY(-1), std::range_error )
      TS_ASSERT_THROWS_NOTHING( y = dataToTest.dataY(0) )
      std::vector<double> yy;
      TS_ASSERT_THROWS_NOTHING( yy = dataToTest.dataY(1) )
      TS_ASSERT_THROWS( dataToTest.dataY(2), std::range_error )
      TS_ASSERT_EQUALS( y.size(), 3 )
      TS_ASSERT_EQUALS( yy.size(), 3 )
      for (unsigned int i = 0; i < y.size(); ++i)
      {
        TS_ASSERT_EQUALS( y[i], i*10 )
        TS_ASSERT_EQUALS( yy[i], i*100 )
      }    

      // test const version
      const ManagedDataBlock2D &constRefToData = dataToTest;
      TS_ASSERT_THROWS( const std::vector<double> v = constRefToData.dataY(-1), std::range_error )
      const std::vector<double> yc = constRefToData.dataY(0);
      const std::vector<double> yyc = constRefToData.dataY(1);
      TS_ASSERT_THROWS( const std::vector<double> v = constRefToData.dataY(2), std::range_error )
      TS_ASSERT_EQUALS( yc.size(), 3 )
      TS_ASSERT_EQUALS( yyc.size(), 3 )
      for (unsigned int i = 0; i < yc.size(); ++i)
      {
        TS_ASSERT_EQUALS( yc[i], i*10 )
        TS_ASSERT_EQUALS( yyc[i], i*100 )
      }    
    }
    
    void dataETester(ManagedDataBlock2D &dataToTest)
    {
      std::vector<double> e;
      TS_ASSERT_THROWS( dataToTest.dataE(-1), std::range_error )
      TS_ASSERT_THROWS_NOTHING( e = dataToTest.dataE(0) )
      std::vector<double> ee;
      TS_ASSERT_THROWS_NOTHING( ee = dataToTest.dataE(1) )
      TS_ASSERT_THROWS( dataToTest.dataE(2), std::range_error )
      TS_ASSERT_EQUALS( e.size(), 3 )
      TS_ASSERT_EQUALS( ee.size(), 3 )
      for (unsigned int i = 0; i < e.size(); ++i)
      {
        TS_ASSERT_EQUALS( e[i], sqrt(i*10.0) )
        TS_ASSERT_EQUALS( ee[i], sqrt(i*100.0) )
      }    
      
      // test const version
      const ManagedDataBlock2D &constRefToData = dataToTest;
      TS_ASSERT_THROWS( const std::vector<double> v = constRefToData.dataE(-1), std::range_error )
      const std::vector<double> ec = constRefToData.dataE(0);
      const std::vector<double> eec = constRefToData.dataE(1);
      TS_ASSERT_THROWS( const std::vector<double> v = constRefToData.dataE(2), std::range_error )
      TS_ASSERT_EQUALS( ec.size(), 3 )
      TS_ASSERT_EQUALS( eec.size(), 3 )
      for (unsigned int i = 0; i < ec.size(); ++i)
      {
        TS_ASSERT_EQUALS( ec[i], sqrt(i*10.0) )
        TS_ASSERT_EQUALS( eec[i], sqrt(i*100.0) )
      }    
    }
};

#endif /*MANAGEDDATABLOCK2DTEST_H_*/
