#ifndef MANAGEDDATABLOCK2DTEST_H_
#define MANAGEDDATABLOCK2DTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/ManagedDataBlock2D.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::DataObjects;
using Mantid::MantidVec;

class ManagedDataBlock2DTest : public CxxTest::TestSuite
{
public:
  ManagedDataBlock2DTest()
    : data(0,2,4,3)
  {
    for (int i = 0; i < 4; ++i)
    {
      data.dataX(0)[i] = i;
      data.dataX(1)[i] = i+4;
    }
    
    for (int i = 0; i < 3; ++i)
    {
      data.dataY(0)[i] = i*10;
      data.dataE(0)[i] = sqrt(data.dataY(0)[i]);
      data.dataY(1)[i] = i*100;
      data.dataE(1)[i] = sqrt(data.dataY(1)[i]);     
    }
  }
  
  void testConstructor()
  {
    ManagedDataBlock2D aBlock(0,2,2,2);
    TS_ASSERT_EQUALS( aBlock.minIndex(), 0 );
    TS_ASSERT( ! aBlock.hasChanges() );
    TS_ASSERT_EQUALS( aBlock.dataX(0).size(), 2 );
    TS_ASSERT_EQUALS( aBlock.dataY(0).size(), 2 );
    TS_ASSERT_EQUALS( aBlock.dataE(0).size(), 2 );
    TS_ASSERT_EQUALS( aBlock.dataX(1).size(), 2 );
    TS_ASSERT_EQUALS( aBlock.dataY(1).size(), 2 );
    TS_ASSERT_EQUALS( aBlock.dataE(1).size(), 2 );
  }
    
  void testSetX()
  {
    ManagedDataBlock2D aBlock(0,1,1,1);
    double aNumber = 5.5;
    boost::shared_ptr<MantidVec > v( new MantidVec(1, aNumber) );
    TS_ASSERT_THROWS_NOTHING( aBlock.setX(0,v) );
    TS_ASSERT_EQUALS( aBlock.dataX(0)[0], aNumber );
    TS_ASSERT_THROWS( aBlock.setX(-1,v), std::range_error );
    TS_ASSERT_THROWS( aBlock.setX(1,v), std::range_error );
    TS_ASSERT( aBlock.hasChanges() );
  }
  
  void testSetData()
  {
    ManagedDataBlock2D aBlock(0,1,1,1);
    double aNumber = 9.9;
    boost::shared_ptr<MantidVec > v( new MantidVec(1, aNumber) );
    double anotherNumber = 3.3;
    boost::shared_ptr<MantidVec > w( new MantidVec(1, anotherNumber) );
    TS_ASSERT_THROWS_NOTHING( aBlock.setData(0,v,v) );
    TS_ASSERT_EQUALS( aBlock.dataY(0)[0], aNumber )    ;
    TS_ASSERT_THROWS( aBlock.setData(-1,v,v), std::range_error );
    TS_ASSERT_THROWS( aBlock.setData(1,v,v), std::range_error );
    
    double yetAnotherNumber = 2.25;
    (*v)[0] = yetAnotherNumber;
    TS_ASSERT_THROWS_NOTHING( aBlock.setData(0,v,w) );
    TS_ASSERT_EQUALS( aBlock.dataY(0)[0], yetAnotherNumber );
    TS_ASSERT_EQUALS( aBlock.dataE(0)[0], anotherNumber );
    TS_ASSERT_THROWS( aBlock.setData(-1,v,w), std::range_error );
    TS_ASSERT_THROWS( aBlock.setData(1,v,w), std::range_error );
    TS_ASSERT( aBlock.hasChanges() );
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
    TS_ASSERT( outfile );
    outfile << data;
    outfile.close();
    
    std::fstream infile("ManagedDataBlock2DTest.tmp", std::ios::binary | std::ios::in);
    TS_ASSERT( infile );
    ManagedDataBlock2D readData(0,2,4,3);
    infile >> readData;
    // use const methods so that I can check the changes flag behaves as it should
    TS_ASSERT( ! readData.hasChanges() );
    dataXTester(readData);
    dataYTester(readData);
    dataETester(readData);
    TS_ASSERT( readData.hasChanges() );
    
    remove("ManagedDataBlock2DTest.tmp");
  }

private:
    ManagedDataBlock2D data;
    
    void dataXTester(ManagedDataBlock2D &dataToTest)
    {
      MantidVec x;
      TS_ASSERT_THROWS( dataToTest.dataX(-1), std::range_error );
      TS_ASSERT_THROWS_NOTHING( x = dataToTest.dataX(0) );
      MantidVec xx;
      TS_ASSERT_THROWS_NOTHING( xx = dataToTest.dataX(1) );
      TS_ASSERT_THROWS( dataToTest.dataX(2), std::range_error );
      TS_ASSERT_EQUALS( x.size(), 4 );
      TS_ASSERT_EQUALS( xx.size(), 4 );
      for (unsigned int i = 0; i < x.size(); ++i)
      {
        TS_ASSERT_EQUALS( x[i], i );
        TS_ASSERT_EQUALS( xx[i], i+4 );
      }
      
      // test const version
      const ManagedDataBlock2D &constRefToData = dataToTest;
      TS_ASSERT_THROWS( const MantidVec v = constRefToData.dataX(-1), std::range_error );
      const MantidVec xc = constRefToData.dataX(0);
      const MantidVec xxc = constRefToData.dataX(1);
      TS_ASSERT_THROWS( const MantidVec v = constRefToData.dataX(2), std::range_error );
      TS_ASSERT_EQUALS( xc.size(), 4 );
      TS_ASSERT_EQUALS( xxc.size(), 4 );
      for (unsigned int i = 0; i < xc.size(); ++i)
      {
        TS_ASSERT_EQUALS( xc[i], i );
        TS_ASSERT_EQUALS( xxc[i], i+4 );
      }
    }
    
    void dataYTester(ManagedDataBlock2D &dataToTest)
    {
      MantidVec y;
      TS_ASSERT_THROWS( dataToTest.dataY(-1), std::range_error );
      TS_ASSERT_THROWS_NOTHING( y = dataToTest.dataY(0) );
      MantidVec yy;
      TS_ASSERT_THROWS_NOTHING( yy = dataToTest.dataY(1) );
      TS_ASSERT_THROWS( dataToTest.dataY(2), std::range_error );
      TS_ASSERT_EQUALS( y.size(), 3 );
      TS_ASSERT_EQUALS( yy.size(), 3 );
      for (unsigned int i = 0; i < y.size(); ++i)
      {
        TS_ASSERT_EQUALS( y[i], i*10 );
        TS_ASSERT_EQUALS( yy[i], i*100 );
      }    

      // test const version
      const ManagedDataBlock2D &constRefToData = dataToTest;
      TS_ASSERT_THROWS( const MantidVec v = constRefToData.dataY(-1), std::range_error );
      const MantidVec yc = constRefToData.dataY(0);
      const MantidVec yyc = constRefToData.dataY(1);
      TS_ASSERT_THROWS( const MantidVec v = constRefToData.dataY(2), std::range_error );
      TS_ASSERT_EQUALS( yc.size(), 3 );
      TS_ASSERT_EQUALS( yyc.size(), 3 );
      for (unsigned int i = 0; i < yc.size(); ++i)
      {
        TS_ASSERT_EQUALS( yc[i], i*10 );
        TS_ASSERT_EQUALS( yyc[i], i*100 );
      }    
    }
    
    void dataETester(ManagedDataBlock2D &dataToTest)
    {
      MantidVec e;
      TS_ASSERT_THROWS( dataToTest.dataE(-1), std::range_error );
      TS_ASSERT_THROWS_NOTHING( e = dataToTest.dataE(0) );
      MantidVec ee;
      TS_ASSERT_THROWS_NOTHING( ee = dataToTest.dataE(1) );
      TS_ASSERT_THROWS( dataToTest.dataE(2), std::range_error );
      TS_ASSERT_EQUALS( e.size(), 3 );
      TS_ASSERT_EQUALS( ee.size(), 3 );
      for (unsigned int i = 0; i < e.size(); ++i)
      {
        TS_ASSERT_EQUALS( e[i], sqrt(i*10.0) );
        TS_ASSERT_EQUALS( ee[i], sqrt(i*100.0) );
      }    
      
      // test const version
      const ManagedDataBlock2D &constRefToData = dataToTest;
      TS_ASSERT_THROWS( const MantidVec v = constRefToData.dataE(-1), std::range_error );
      const MantidVec ec = constRefToData.dataE(0);
      const MantidVec eec = constRefToData.dataE(1);
      TS_ASSERT_THROWS( const MantidVec v = constRefToData.dataE(2), std::range_error );
      TS_ASSERT_EQUALS( ec.size(), 3 );
      TS_ASSERT_EQUALS( eec.size(), 3 );
      for (unsigned int i = 0; i < ec.size(); ++i)
      {
        TS_ASSERT_EQUALS( ec[i], sqrt(i*10.0) );
        TS_ASSERT_EQUALS( eec[i], sqrt(i*100.0) );
      }    
    }
};

#endif /*MANAGEDDATABLOCK2DTEST_H_*/
