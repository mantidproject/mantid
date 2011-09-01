#ifndef MANAGEDDATABLOCK2DTEST_H_
#define MANAGEDDATABLOCK2DTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/ManagedDataBlock2D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/cow_ptr.h"

using namespace Mantid::DataObjects;
using Mantid::MantidVec;
using Mantid::MantidVecPtr;

class ManagedDataBlock2DTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ManagedDataBlock2DTest *createSuite() { return new ManagedDataBlock2DTest(); }
  static void destroySuite( ManagedDataBlock2DTest *suite ) { delete suite; }

  ManagedDataBlock2DTest()
    : data(0,2,4,3, NULL, MantidVecPtr() )
  {
    for (int i = 0; i < 4; ++i)
    {
      data.getSpectrum(0)->dataX()[i] = i;
      data.getSpectrum(1)->dataX()[i] = i+4;
    }
    
    for (int i = 0; i < 3; ++i)
    {
      data.getSpectrum(0)->dataY()[i] = i*10;
      data.getSpectrum(0)->dataE()[i] = sqrt(data.getSpectrum(0)->dataY()[i]);
      data.getSpectrum(1)->dataY()[i] = i*100;
      data.getSpectrum(1)->dataE()[i] = sqrt(data.getSpectrum(1)->dataY()[i]);
    }
  }
  
  void testConstructor()
  {
    ManagedDataBlock2D aBlock(0,2,2,2, NULL, MantidVecPtr());
    TS_ASSERT_EQUALS( aBlock.minIndex(), 0 );
    TS_ASSERT( ! aBlock.hasChanges() );
    TSM_ASSERT("When initialized the block says it is loaded", aBlock.isLoaded() );
    TS_ASSERT_EQUALS( aBlock.getSpectrum(0)->dataX().size(), 2 );
    TS_ASSERT_EQUALS( aBlock.getSpectrum(0)->dataY().size(), 2 );
    TS_ASSERT_EQUALS( aBlock.getSpectrum(0)->dataE().size(), 2 );
    TS_ASSERT_EQUALS( aBlock.getSpectrum(1)->dataX().size(), 2 );
    TS_ASSERT_EQUALS( aBlock.getSpectrum(1)->dataY().size(), 2 );
    TS_ASSERT_EQUALS( aBlock.getSpectrum(1)->dataE().size(), 2 );
  }

  void test_releaseData()
  {
    ManagedDataBlock2D aBlock(0,2,2,2, NULL, MantidVecPtr());
    TSM_ASSERT("When initialized the block says it is loaded", aBlock.isLoaded() );
    // Spectra start loaded too
    ManagedHistogram1D * h0 = dynamic_cast<ManagedHistogram1D *>(aBlock.getSpectrum(0));
    ManagedHistogram1D * h1 = dynamic_cast<ManagedHistogram1D *>(aBlock.getSpectrum(1));
    TS_ASSERT(h0->isLoaded());
    TS_ASSERT(h1->isLoaded());

    aBlock.releaseData();

    // No longer loaded
    TS_ASSERT(!h0->isLoaded());
    TS_ASSERT(!h1->isLoaded());
  }
    
  void testSetX()
  {
    ManagedDataBlock2D aBlock(0,1,1,1, NULL, MantidVecPtr());
    double aNumber = 5.5;
    boost::shared_ptr<MantidVec > v( new MantidVec(1, aNumber) );
    TS_ASSERT_THROWS_NOTHING( aBlock.getSpectrum(0)->setX(v) );
    TS_ASSERT_EQUALS( aBlock.getSpectrum(0)->dataX()[0], aNumber );
    TS_ASSERT_THROWS( aBlock.getSpectrum(-1)->setX(v), std::range_error );
    TS_ASSERT_THROWS( aBlock.getSpectrum(1)->setX(v), std::range_error );
    TS_ASSERT( aBlock.hasChanges() );
  }
  
  void testSpectrumNo()
  {
    ManagedDataBlock2D aBlock(0,1,1,1, NULL, MantidVecPtr());
    TS_ASSERT_THROWS_NOTHING( aBlock.getSpectrum(0)->setSpectrumNo(1234) );
    // You don't need to save back to disk since that's in memory all the time
    TS_ASSERT( !aBlock.hasChanges() );
  }

  void testDetectorIDs()
  {
    ManagedDataBlock2D aBlock(0,1,1,1, NULL, MantidVecPtr());
    TS_ASSERT_THROWS_NOTHING( aBlock.getSpectrum(0)->addDetectorID(1234) );
    // You don't need to save back to disk since that's in memory all the time
    TS_ASSERT( !aBlock.hasChanges() );
  }

  void testSetData()
  {
    ManagedDataBlock2D aBlock(0,1,1,1, NULL, MantidVecPtr());
    double aNumber = 9.9;
    boost::shared_ptr<MantidVec > v( new MantidVec(1, aNumber) );
    double anotherNumber = 3.3;
    boost::shared_ptr<MantidVec > w( new MantidVec(1, anotherNumber) );
    TS_ASSERT_THROWS_NOTHING( aBlock.getSpectrum(0)->setData(v,v) );
    TS_ASSERT_EQUALS( aBlock.getSpectrum(0)->dataY()[0], aNumber )    ;
    TS_ASSERT_THROWS( aBlock.getSpectrum(-1)->setData(v,v), std::range_error );
    TS_ASSERT_THROWS( aBlock.getSpectrum(1)->setData(v,v), std::range_error );
    
    double yetAnotherNumber = 2.25;
    (*v)[0] = yetAnotherNumber;
    TS_ASSERT_THROWS_NOTHING( aBlock.getSpectrum(0)->setData(v,w) );
    TS_ASSERT_EQUALS( aBlock.getSpectrum(0)->dataY()[0], yetAnotherNumber );
    TS_ASSERT_EQUALS( aBlock.getSpectrum(0)->dataE()[0], anotherNumber );
    TS_ASSERT_THROWS( aBlock.getSpectrum(-1)->setData(v,w), std::range_error );
    TS_ASSERT_THROWS( aBlock.getSpectrum(1)->setData(v,w), std::range_error );
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

    // Empty block
    ManagedDataBlock2D readData(0,2,4,3, NULL, MantidVecPtr());

    // The spectra say "loaded" because they were initialized
    ManagedHistogram1D * h0 = dynamic_cast<ManagedHistogram1D *>(readData.getSpectrum(0));
    ManagedHistogram1D * h1 = dynamic_cast<ManagedHistogram1D *>(readData.getSpectrum(1));
    TS_ASSERT(h0->isLoaded());
    TS_ASSERT(h1->isLoaded());

    infile >> readData;
    // use const methods so that I can check the changes flag behaves as it should
    TS_ASSERT( ! readData.hasChanges() );
    dataXTester(readData);
    dataYTester(readData);
    dataETester(readData);
    TS_ASSERT( readData.hasChanges() );
    
    // The spectra are now marked as loaded
    TS_ASSERT(h0->isLoaded());
    TS_ASSERT(h1->isLoaded());


    remove("ManagedDataBlock2DTest.tmp");
  }

private:
    ManagedDataBlock2D data;
    
    void dataXTester(ManagedDataBlock2D &dataToTest)
    {
      MantidVec x;
      TS_ASSERT_THROWS( dataToTest.getSpectrum(-1)->dataX(), std::range_error );
      TS_ASSERT_THROWS_NOTHING( x = dataToTest.getSpectrum(0)->dataX() );
      MantidVec xx;
      TS_ASSERT_THROWS_NOTHING( xx = dataToTest.getSpectrum(1)->dataX() );
      TS_ASSERT_THROWS( dataToTest.getSpectrum(2)->dataX(), std::range_error );
      TS_ASSERT_EQUALS( x.size(), 4 );
      TS_ASSERT_EQUALS( xx.size(), 4 );
      for (unsigned int i = 0; i < x.size(); ++i)
      {
        TS_ASSERT_EQUALS( x[i], i );
        TS_ASSERT_EQUALS( xx[i], i+4 );
      }
      
      // test const version
      const ManagedDataBlock2D &constRefToData = dataToTest;
      TS_ASSERT_THROWS( const MantidVec v = constRefToData.getSpectrum(-1)->dataX(), std::range_error );
      const MantidVec xc = constRefToData.getSpectrum(0)->dataX();
      const MantidVec xxc = constRefToData.getSpectrum(1)->dataX();
      TS_ASSERT_THROWS( const MantidVec v = constRefToData.getSpectrum(2)->dataX(), std::range_error );
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
      TS_ASSERT_THROWS( dataToTest.getSpectrum(-1)->dataY(), std::range_error );
      TS_ASSERT_THROWS_NOTHING( y = dataToTest.getSpectrum(0)->dataY() );
      MantidVec yy;
      TS_ASSERT_THROWS_NOTHING( yy = dataToTest.getSpectrum(1)->dataY() );
      TS_ASSERT_THROWS( dataToTest.getSpectrum(2)->dataY(), std::range_error );
      TS_ASSERT_EQUALS( y.size(), 3 );
      TS_ASSERT_EQUALS( yy.size(), 3 );
      for (unsigned int i = 0; i < y.size(); ++i)
      {
        TS_ASSERT_EQUALS( y[i], i*10 );
        TS_ASSERT_EQUALS( yy[i], i*100 );
      }    

      // test const version
      const ManagedDataBlock2D &constRefToData = dataToTest;
      TS_ASSERT_THROWS( const MantidVec v = constRefToData.getSpectrum(-1)->dataY(), std::range_error );
      const MantidVec yc = constRefToData.getSpectrum(0)->dataY();
      const MantidVec yyc = constRefToData.getSpectrum(1)->dataY();
      TS_ASSERT_THROWS( const MantidVec v = constRefToData.getSpectrum(2)->dataY(), std::range_error );
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
      TS_ASSERT_THROWS( dataToTest.getSpectrum(-1)->dataE(), std::range_error );
      TS_ASSERT_THROWS_NOTHING( e = dataToTest.getSpectrum(0)->dataE() );
      MantidVec ee;
      TS_ASSERT_THROWS_NOTHING( ee = dataToTest.getSpectrum(1)->dataE() );
      TS_ASSERT_THROWS( dataToTest.getSpectrum(2)->dataE(), std::range_error );
      TS_ASSERT_EQUALS( e.size(), 3 );
      TS_ASSERT_EQUALS( ee.size(), 3 );
      for (unsigned int i = 0; i < e.size(); ++i)
      {
        TS_ASSERT_EQUALS( e[i], sqrt(i*10.0) );
        TS_ASSERT_EQUALS( ee[i], sqrt(i*100.0) );
      }    
      
      // test const version
      const ManagedDataBlock2D &constRefToData = dataToTest;
      TS_ASSERT_THROWS( const MantidVec v = constRefToData.getSpectrum(-1)->dataE(), std::range_error );
      const MantidVec ec = constRefToData.getSpectrum(0)->dataE();
      const MantidVec eec = constRefToData.getSpectrum(1)->dataE();
      TS_ASSERT_THROWS( const MantidVec v = constRefToData.getSpectrum(2)->dataE(), std::range_error );
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
