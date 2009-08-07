#ifndef COMPRESSEDWORKSPACE2D_H_
#define COMPRESSEDWORKSPACE2D_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/CompressedWorkspace2D.h"

using namespace std;
using namespace Mantid::DataObjects;

class CompressedWorkspace2DTest : public CxxTest::TestSuite
{
public:
  CompressedWorkspace2DTest()
  {
    smallWorkspace.setTitle("smallWorkspace");
    smallWorkspace.initialize(2,4,3);
    for (int i = 0; i < 4; ++i)
    {
      smallWorkspace.dataX(0)[i] = i;
      smallWorkspace.dataX(1)[i] = i+4;
    }
    
    for (int i = 0; i < 3; ++i)
    {
      smallWorkspace.dataY(0)[i] = i*10;
      smallWorkspace.dataE(0)[i] = sqrt(smallWorkspace.dataY(0)[i]);
      smallWorkspace.dataY(1)[i] = i*100;
      smallWorkspace.dataE(1)[i] = sqrt(smallWorkspace.dataY(1)[i]);     
    }
    
    bigWorkspace.setTitle("bigWorkspace");
    int nVec = 1250;
    int vecLength = 25;
    bigWorkspace.initialize(nVec, vecLength, vecLength);
    for (int i=0; i< nVec; i++)
    {
      boost::shared_ptr<std::vector<double> > x1(new std::vector<double>(vecLength,1+i) );
      boost::shared_ptr<std::vector<double> > y1(new std::vector<double>(vecLength,5+i) );
      boost::shared_ptr<std::vector<double> > e1(new std::vector<double>(vecLength,4+i) );
      bigWorkspace.setX(i,x1);     
      bigWorkspace.setData(i,y1,e1);
    }
  }
  
  void testInit()
  {
    CompressedWorkspace2D ws;
    ws.setTitle("testInit");
    TS_ASSERT_THROWS_NOTHING( ws.initialize(5,5,5) )
    TS_ASSERT_EQUALS( ws.getNumberHistograms(), 5 )
    TS_ASSERT_EQUALS( ws.blocksize(), 5 )
    TS_ASSERT_EQUALS( ws.size(), 25 )

    for (int i = 0; i < 5; ++i)
    {
      TS_ASSERT_EQUALS( ws.dataX(i).size(), 5 )
      TS_ASSERT_EQUALS( ws.dataY(i).size(), 5 )
      TS_ASSERT_EQUALS( ws.dataE(i).size(), 5 )
    }

  }

  void testCast()
  {
    CompressedWorkspace2D *ws = new CompressedWorkspace2D;
    TS_ASSERT( dynamic_cast<Workspace2D*>(ws) )
    TS_ASSERT( dynamic_cast<Mantid::API::Workspace*>(ws) )
  }

  void testId()
  {
    TS_ASSERT( ! smallWorkspace.id().compare("CompressedWorkspace2D") )
    TS_ASSERT_EQUALS( smallWorkspace.id(), "CompressedWorkspace2D" )
  }

  void testgetNumberHistograms()
  {
    TS_ASSERT_EQUALS( smallWorkspace.getNumberHistograms(), 2 )
    TS_ASSERT_EQUALS( bigWorkspace.getNumberHistograms(), 1250 )
    
    Workspace2D &ws = dynamic_cast<Workspace2D&>(smallWorkspace);
    TS_ASSERT_EQUALS( ws.getNumberHistograms(), 2);
  }

  void testSetX()
  {
    CompressedWorkspace2D ws;
    ws.setTitle("testSetX");
    ws.initialize(1,1,1);
    double aNumber = 5.5;
    boost::shared_ptr<std::vector<double> > v(new std::vector<double>(1, aNumber));
    TS_ASSERT_THROWS_NOTHING( ws.setX(0,v) )
    TS_ASSERT_EQUALS( ws.dataX(0)[0], aNumber )
    TS_ASSERT_THROWS( ws.setX(-1,v), std::range_error )
    TS_ASSERT_THROWS( ws.setX(1,v), std::range_error )
    
    double anotherNumber = 9.99;
    boost::shared_ptr<std::vector<double> > vec(new std::vector<double>(25, anotherNumber));
    TS_ASSERT_THROWS_NOTHING( bigWorkspace.setX(10, vec) )
    TS_ASSERT_EQUALS( bigWorkspace.dataX(10)[7], anotherNumber )
    TS_ASSERT_EQUALS( bigWorkspace.dataX(10)[22], anotherNumber )
  }

  void testSetData()
  {
    CompressedWorkspace2D ws;
    ws.setTitle("testSetData");
    ws.initialize(1,1,1);
    double aNumber = 9.9;
    boost::shared_ptr<std::vector<double> > v(new std::vector<double>(1, aNumber));
    double anotherNumber = 3.3;
    boost::shared_ptr<std::vector<double> > w(new std::vector<double>(1, anotherNumber));
    TS_ASSERT_THROWS_NOTHING( ws.setData(0,v,v) )
    TS_ASSERT_EQUALS( ws.dataY(0)[0], aNumber )    
    TS_ASSERT_THROWS( ws.setData(-1,v,v), std::range_error )
    TS_ASSERT_THROWS( ws.setData(1,v,v), std::range_error )
    
    double yetAnotherNumber = 2.25;
    (*v)[0] = yetAnotherNumber;
    TS_ASSERT_THROWS_NOTHING( ws.setData(0,v,w) )
    TS_ASSERT_EQUALS( ws.dataY(0)[0], yetAnotherNumber )
    TS_ASSERT_EQUALS( ws.dataE(0)[0], anotherNumber )
    TS_ASSERT_THROWS( ws.setData(-1,v,w), std::range_error )
    TS_ASSERT_THROWS( ws.setData(1,v,w), std::range_error )
    
    double oneMoreNumber = 8478.6728;
    boost::shared_ptr<std::vector<double> > vec(new std::vector<double>(25, oneMoreNumber));
    TS_ASSERT_THROWS_NOTHING( bigWorkspace.setData(49, vec, vec) )
    TS_ASSERT_EQUALS( bigWorkspace.dataY(49)[0], oneMoreNumber )
    TS_ASSERT_EQUALS( bigWorkspace.dataE(49)[9], oneMoreNumber )
  }

  void testSize()
  {
    TS_ASSERT_EQUALS( smallWorkspace.size(), 6 )
    TS_ASSERT_EQUALS( bigWorkspace.size(), 31250 )
  }

  void testBlocksize()
  {
    TS_ASSERT_EQUALS( smallWorkspace.blocksize(), 3 )
    TS_ASSERT_EQUALS( bigWorkspace.blocksize(), 25 )    
  }

  void testDataX()
  {
    std::vector<double> x;
    TS_ASSERT_THROWS( smallWorkspace.dataX(-1), std::range_error )
    TS_ASSERT_THROWS_NOTHING( x = smallWorkspace.dataX(0) )
    std::vector<double> xx;
    TS_ASSERT_THROWS_NOTHING( xx = smallWorkspace.dataX(1) )
    TS_ASSERT_THROWS( smallWorkspace.dataX(2), std::range_error )
    TS_ASSERT_EQUALS( x.size(), 4 )
    TS_ASSERT_EQUALS( xx.size(), 4 )
    for (unsigned int i = 0; i < x.size(); ++i)
    {
      TS_ASSERT_EQUALS( x[i], i )
      TS_ASSERT_EQUALS( xx[i], i+4 )
    }
    
    // test const version
    const CompressedWorkspace2D &constRefToData = smallWorkspace;
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
    
    TS_ASSERT_EQUALS( bigWorkspace.dataX(101)[5], 102 )
    TS_ASSERT_EQUALS( bigWorkspace.dataX(201)[24], 202 )
    TS_ASSERT_THROWS_NOTHING( bigWorkspace.dataX(39)[10] = 2.22 )
    TS_ASSERT_EQUALS( bigWorkspace.dataX(39)[10], 2.22 )
  }

  void testDataY()
  {
    std::vector<double> y;
    TS_ASSERT_THROWS( smallWorkspace.dataY(-1), std::range_error )
    TS_ASSERT_THROWS_NOTHING( y = smallWorkspace.dataY(0) )
    std::vector<double> yy;
    TS_ASSERT_THROWS_NOTHING( yy = smallWorkspace.dataY(1) )
    TS_ASSERT_THROWS( smallWorkspace.dataY(2), std::range_error )
    TS_ASSERT_EQUALS( y.size(), 3 )
    TS_ASSERT_EQUALS( yy.size(), 3 )
    for (unsigned int i = 0; i < y.size(); ++i)
    {
      TS_ASSERT_EQUALS( y[i], i*10 )
      TS_ASSERT_EQUALS( yy[i], i*100 )
    }    

    // test const version
    const CompressedWorkspace2D &constRefToData = smallWorkspace;
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

    TS_ASSERT_EQUALS( bigWorkspace.dataY(178)[8], 183 )
    TS_ASSERT_EQUALS( bigWorkspace.dataY(64)[11], 69 )
    TS_ASSERT_THROWS_NOTHING( bigWorkspace.dataY(123)[8] = 3.33 )
    TS_ASSERT_EQUALS( bigWorkspace.dataY(123)[8], 3.33 )
  }

  void testDataE()
  {
    std::vector<double> e;
    TS_ASSERT_THROWS( smallWorkspace.dataE(-1), std::range_error )
    TS_ASSERT_THROWS_NOTHING( e = smallWorkspace.dataE(0) )
    std::vector<double> ee;
    TS_ASSERT_THROWS_NOTHING( ee = smallWorkspace.dataE(1) )
    TS_ASSERT_THROWS( smallWorkspace.dataE(2), std::range_error )
    TS_ASSERT_EQUALS( e.size(), 3 )
    TS_ASSERT_EQUALS( ee.size(), 3 )
    for (unsigned int i = 0; i < e.size(); ++i)
    {
      TS_ASSERT_EQUALS( e[i], sqrt(i*10.0) )
      TS_ASSERT_EQUALS( ee[i], sqrt(i*100.0) )
    }    
    
    // test const version
    const CompressedWorkspace2D &constRefToData = smallWorkspace;
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

    TS_ASSERT_EQUALS( bigWorkspace.dataE(0)[23], 4 )
    TS_ASSERT_EQUALS( bigWorkspace.dataE(249)[2], 253 )
    TS_ASSERT_THROWS_NOTHING( bigWorkspace.dataE(11)[11] = 4.44 )
    TS_ASSERT_EQUALS( bigWorkspace.dataE(11)[11], 4.44 )
  }

  void testDestructor()
  {
  }

private:
  CompressedWorkspace2D smallWorkspace;
  CompressedWorkspace2D bigWorkspace;
};

#endif /*COMPRESSEDWORKSPACE2D_H_*/
