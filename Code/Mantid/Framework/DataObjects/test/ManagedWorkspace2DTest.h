#ifndef MANAGEDWORKSPACE2DTEST_H_
#define MANAGEDWORKSPACE2DTEST_H_

#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/ManagedWorkspace2D.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument/OneToOneSpectraDetectorMap.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Memory.h"
#include <cxxtest/TestSuite.h>
#include <Poco/File.h>
#include <boost/lexical_cast.hpp>

using Mantid::MantidVec;
using std::size_t;
using Mantid::DataObjects::ManagedWorkspace2D;

class ManagedWorkspace2DTest : public CxxTest::TestSuite
{
public:
  static ManagedWorkspace2DTest *createSuite() { return new ManagedWorkspace2DTest(); }
  static void destroySuite( ManagedWorkspace2DTest *suite ) { delete suite; }

  ManagedWorkspace2DTest()
  {
    smallWorkspace.setTitle("ManagedWorkspace2DTest_smallWorkspace");
    smallWorkspace.initialize(2,4,3);
    for (size_t i = 0; i < 4; ++i)
    {
      smallWorkspace.dataX(0)[i] = static_cast<double>(i);
      smallWorkspace.dataX(1)[i] = static_cast<double>(i+4);
    }
    
    for (size_t i = 0; i < 3; ++i)
    {
      smallWorkspace.dataY(0)[i] = static_cast<double>(i*10);
      smallWorkspace.dataE(0)[i] = sqrt(smallWorkspace.dataY(0)[i]);
      smallWorkspace.dataY(1)[i] = static_cast<double>(i*100);
      smallWorkspace.dataE(1)[i] = sqrt(smallWorkspace.dataY(1)[i]);     
    }
    
    bigWorkspace.setTitle("ManagedWorkspace2DTest_bigWorkspace");
    size_t nVec = 1250;
    size_t vecLength = 25;
    bigWorkspace.initialize(nVec, vecLength, vecLength);
    // As of 20/7/2011, revision [13332], this call is required for the testSpectrumAndDetectorNumbers test to pass
    bigWorkspace.replaceSpectraMap(new Mantid::Geometry::OneToOneSpectraDetectorMap(1,(int)nVec));
    for (size_t i=0; i< nVec; i++)
    {
      boost::shared_ptr<MantidVec > x1(new MantidVec(vecLength,1+i) );
      boost::shared_ptr<MantidVec > y1(new MantidVec(vecLength,5+i) );
      boost::shared_ptr<MantidVec > e1(new MantidVec(vecLength,4+i) );
      bigWorkspace.setX(i,x1);     
      bigWorkspace.setData(i,y1,e1);
      // As of 20/7/2011, revision [13332], these calls have no (lasting) effect.
      // When they do, the testSpectrumAndDetectorNumbers test will start to fail
      bigWorkspace.getSpectrum(i)->setSpectrumNo((int)i);
      bigWorkspace.getSpectrum(i)->setDetectorID((int)i*100);
    }
  }
  
  void testInit()
  {
    Mantid::DataObjects::ManagedWorkspace2D ws;
    ws.setTitle("testInit");
    TS_ASSERT_THROWS_NOTHING( ws.initialize(5,5,5) );;
    TS_ASSERT_EQUALS( ws.getNumberHistograms(), 5 );;
    TS_ASSERT_EQUALS( ws.blocksize(), 5 );;
    TS_ASSERT_EQUALS( ws.size(), 25 );;

    for (size_t i = 0; i < 5; ++i)
    {
      TS_ASSERT_EQUALS( ws.dataX(i).size(), 5 );;
      TS_ASSERT_EQUALS( ws.dataY(i).size(), 5 );;
      TS_ASSERT_EQUALS( ws.dataE(i).size(), 5 );;
    }

    // Test all is as it should be with the temporary file
    std::string filename = ws.get_filename() + "0";
    std::fstream file(filename.c_str(), std::ios::in | std::ios::binary);
    TSM_ASSERT(filename, file);;
    
    double temp;
    file.read((char *) &temp, sizeof(double));
    TS_ASSERT( file.fail() );
    file.close();
  }

  void testCast()
  {
    Mantid::DataObjects::ManagedWorkspace2D *ws = new Mantid::DataObjects::ManagedWorkspace2D;
    TS_ASSERT( dynamic_cast<Mantid::DataObjects::Workspace2D*>(ws) );
    TS_ASSERT( dynamic_cast<Mantid::API::Workspace*>(ws) );
    delete ws;
  }

  void testId()
  {
    TS_ASSERT( ! smallWorkspace.id().compare("ManagedWorkspace2D") );
  }

  void testgetNumberHistograms()
  {
    TS_ASSERT_EQUALS( smallWorkspace.getNumberHistograms(), 2 );
    TS_ASSERT_EQUALS( bigWorkspace.getNumberHistograms(), 1250 );
    
    Mantid::DataObjects::Workspace2D &ws = dynamic_cast<Mantid::DataObjects::Workspace2D&>(smallWorkspace);
    TS_ASSERT_EQUALS( ws.getNumberHistograms(), 2);;
  }

  void testSetX()
  {
    Mantid::DataObjects::ManagedWorkspace2D ws;
    ws.setTitle("testSetX");
    ws.initialize(1,1,1);
    double aNumber = 5.5;
    boost::shared_ptr<MantidVec > v(new MantidVec(1, aNumber));
    TS_ASSERT_THROWS_NOTHING( ws.setX(0,v) );
    TS_ASSERT_EQUALS( ws.dataX(0)[0], aNumber );
    TS_ASSERT_THROWS( ws.setX(-1,v), std::range_error );
    TS_ASSERT_THROWS( ws.setX(1,v), std::range_error );
    
    double anotherNumber = 9.99;
    boost::shared_ptr<MantidVec > vec(new MantidVec(25, anotherNumber));
    TS_ASSERT_THROWS_NOTHING( bigWorkspace.setX(10, vec) );
    TS_ASSERT_EQUALS( bigWorkspace.dataX(10)[7], anotherNumber );
    TS_ASSERT_EQUALS( bigWorkspace.dataX(10)[22], anotherNumber );
  }

  void testSetData()
  {
    Mantid::DataObjects::ManagedWorkspace2D ws;
    ws.setTitle("testSetData");
    ws.initialize(1,1,1);
    double aNumber = 9.9;
    boost::shared_ptr<MantidVec > v(new MantidVec(1, aNumber));
    double anotherNumber = 3.3;
    boost::shared_ptr<MantidVec > w(new MantidVec(1, anotherNumber));
    TS_ASSERT_THROWS_NOTHING( ws.setData(0,v,v) );
    TS_ASSERT_EQUALS( ws.dataY(0)[0], aNumber )    ;
    TS_ASSERT_THROWS( ws.setData(-1,v,v), std::range_error );
    TS_ASSERT_THROWS( ws.setData(1,v,v), std::range_error );
    
    double yetAnotherNumber = 2.25;
    (*v)[0] = yetAnotherNumber;
    TS_ASSERT_THROWS_NOTHING( ws.setData(0,v,w) );
    TS_ASSERT_EQUALS( ws.dataY(0)[0], yetAnotherNumber );
    TS_ASSERT_EQUALS( ws.dataE(0)[0], anotherNumber );
    TS_ASSERT_THROWS( ws.setData(-1,v,w), std::range_error );
    TS_ASSERT_THROWS( ws.setData(1,v,w), std::range_error );
    
    double oneMoreNumber = 8478.6728;
    boost::shared_ptr<MantidVec > vec(new MantidVec(25, oneMoreNumber));
    TS_ASSERT_THROWS_NOTHING( bigWorkspace.setData(49, vec, vec) );
    TS_ASSERT_EQUALS( bigWorkspace.dataY(49)[0], oneMoreNumber );
    TS_ASSERT_EQUALS( bigWorkspace.dataE(49)[9], oneMoreNumber );
  }

  void testSize()
  {
    TS_ASSERT_EQUALS( smallWorkspace.size(), 6 );
    TS_ASSERT_EQUALS( bigWorkspace.size(), 31250 );
  }

  void testBlocksize()
  {
    TS_ASSERT_EQUALS( smallWorkspace.blocksize(), 3 );
    TS_ASSERT_EQUALS( bigWorkspace.blocksize(), 25 )    ;
  }

  void testDataX()
  {
    MantidVec x;
    TS_ASSERT_THROWS( smallWorkspace.dataX(-1), std::range_error );
    TS_ASSERT_THROWS_NOTHING( x = smallWorkspace.dataX(0) );
    MantidVec xx;
    TS_ASSERT_THROWS_NOTHING( xx = smallWorkspace.dataX(1) );
    TS_ASSERT_THROWS( smallWorkspace.dataX(2), std::range_error );
    TS_ASSERT_EQUALS( x.size(), 4 );
    TS_ASSERT_EQUALS( xx.size(), 4 );
    for (size_t i = 0; i < x.size(); ++i)
    {
      TS_ASSERT_EQUALS( x[i], i );
      TS_ASSERT_EQUALS( xx[i], i+4 );
    }
    
    // test const version
    const Mantid::DataObjects::ManagedWorkspace2D &constRefToData = smallWorkspace;
    TS_ASSERT_THROWS( const MantidVec v = constRefToData.dataX(-1), std::range_error );
    const MantidVec xc = constRefToData.dataX(0);
    const MantidVec xxc = constRefToData.dataX(1);
    TS_ASSERT_THROWS( const MantidVec v = constRefToData.dataX(2), std::range_error );
    TS_ASSERT_EQUALS( xc.size(), 4 );
    TS_ASSERT_EQUALS( xxc.size(), 4 );
    for (size_t i = 0; i < xc.size(); ++i)
    {
      TS_ASSERT_EQUALS( xc[i], i );
      TS_ASSERT_EQUALS( xxc[i], i+4 );
    }
    
    TS_ASSERT_EQUALS( bigWorkspace.dataX(101)[5], 102 );
    TS_ASSERT_EQUALS( bigWorkspace.dataX(201)[24], 202 );
    TS_ASSERT_THROWS_NOTHING( bigWorkspace.dataX(39)[10] = 2.22 );
    TS_ASSERT_EQUALS( bigWorkspace.dataX(39)[10], 2.22 );
  }

  void testDataDx()
  {
    TS_ASSERT_EQUALS( smallWorkspace.dataDx(0).size(), 4 );
    TS_ASSERT_EQUALS( smallWorkspace.readDx(1)[3], 0.0 );

    TS_ASSERT_THROWS_NOTHING( smallWorkspace.dataDx(1)[3] = 9.9 );
    TS_ASSERT_EQUALS( smallWorkspace.readDx(1)[3], 9.9 );
  }

  void testDataY()
  {
    MantidVec y;
    TS_ASSERT_THROWS( smallWorkspace.dataY(-1), std::range_error );
    TS_ASSERT_THROWS_NOTHING( y = smallWorkspace.dataY(0) );
    MantidVec yy;
    TS_ASSERT_THROWS_NOTHING( yy = smallWorkspace.dataY(1) );
    TS_ASSERT_THROWS( smallWorkspace.dataY(2), std::range_error );
    TS_ASSERT_EQUALS( y.size(), 3 );
    TS_ASSERT_EQUALS( yy.size(), 3 );
    for (size_t i = 0; i < y.size(); ++i)
    {
      TS_ASSERT_EQUALS( y[i], i*10 );
      TS_ASSERT_EQUALS( yy[i], i*100 );
    }    

    // test const version
    const Mantid::DataObjects::ManagedWorkspace2D &constRefToData = smallWorkspace;
    TS_ASSERT_THROWS( const MantidVec v = constRefToData.dataY(-1), std::range_error );
    const MantidVec yc = constRefToData.dataY(0);
    const MantidVec yyc = constRefToData.dataY(1);
    TS_ASSERT_THROWS( const MantidVec v = constRefToData.dataY(2), std::range_error );
    TS_ASSERT_EQUALS( yc.size(), 3 );
    TS_ASSERT_EQUALS( yyc.size(), 3 );
    for (size_t i = 0; i < yc.size(); ++i)
    {
      TS_ASSERT_EQUALS( yc[i], i*10 );
      TS_ASSERT_EQUALS( yyc[i], i*100 );
    }    

    TS_ASSERT_EQUALS( bigWorkspace.dataY(178)[8], 183 );
    TS_ASSERT_EQUALS( bigWorkspace.dataY(64)[11], 69 );
    TS_ASSERT_THROWS_NOTHING( bigWorkspace.dataY(123)[8] = 3.33 );
    TS_ASSERT_EQUALS( bigWorkspace.dataY(123)[8], 3.33 );
  }

  void testDataE()
  {
    MantidVec e;
    TS_ASSERT_THROWS( smallWorkspace.dataE(-1), std::range_error );
    TS_ASSERT_THROWS_NOTHING( e = smallWorkspace.dataE(0) );
    MantidVec ee;
    TS_ASSERT_THROWS_NOTHING( ee = smallWorkspace.dataE(1) );
    TS_ASSERT_THROWS( smallWorkspace.dataE(2), std::range_error );
    TS_ASSERT_EQUALS( e.size(), 3 );
    TS_ASSERT_EQUALS( ee.size(), 3 );
    for (size_t i = 0; i < e.size(); ++i)
    {
      TS_ASSERT_EQUALS( e[i], sqrt(static_cast<double>(i)*10.0) );
      TS_ASSERT_EQUALS( ee[i], sqrt(static_cast<double>(i)*100.0) );
    }    
    
    // test const version
    const Mantid::DataObjects::ManagedWorkspace2D &constRefToData = smallWorkspace;
    TS_ASSERT_THROWS( const MantidVec v = constRefToData.dataE(-1), std::range_error );
    const MantidVec ec = constRefToData.dataE(0);
    const MantidVec eec = constRefToData.dataE(1);
    TS_ASSERT_THROWS( const MantidVec v = constRefToData.dataE(2), std::range_error );
    TS_ASSERT_EQUALS( ec.size(), 3 );
    TS_ASSERT_EQUALS( eec.size(), 3 );
    for (size_t i = 0; i < ec.size(); ++i)
    {
      TS_ASSERT_EQUALS( ec[i], sqrt(static_cast<double>(i)*10.0) );
      TS_ASSERT_EQUALS( eec[i], sqrt(static_cast<double>(i)*100.0) );
    }    

    TS_ASSERT_EQUALS( bigWorkspace.dataE(0)[23], 4 );
    TS_ASSERT_EQUALS( bigWorkspace.dataE(249)[2], 253 );
    TS_ASSERT_THROWS_NOTHING( bigWorkspace.dataE(11)[11] = 4.44 );
    TS_ASSERT_EQUALS( bigWorkspace.dataE(11)[11], 4.44 );
  }

  void testSpectrumAndDetectorNumbers()
  {
    for (size_t i = 0; i < bigWorkspace.getNumberHistograms(); ++i)
    {
      TS_ASSERT_EQUALS( bigWorkspace.getAxis(1)->spectraNo(i), i+1 );
      // Values were set in the constructor
      TS_ASSERT_EQUALS( bigWorkspace.getSpectrum(i)->getSpectrumNo(), i );
      TS_ASSERT( bigWorkspace.getSpectrum(i)->hasDetectorID((int)i*100) );
    }
  }

  void testMultipleFiles()
  {
    const size_t NHist = 111;
    const size_t NY = 9;
    const size_t NX = NY + 1;

    double dBlockSize = 2 * ( sizeof(int) + ( NX + 2*NY ) * sizeof(double) );

    // This will make sure 1 ManagedDataBlock = 2 Vectors
    Mantid::Kernel::ConfigServiceImpl& conf = Mantid::Kernel::ConfigService::Instance();
    const std::string blocksize = "ManagedWorkspace.DataBlockSize";
    const std::string oldValue = conf.getString(blocksize);
    conf.setString(blocksize,boost::lexical_cast<std::string>(dBlockSize));

    const std::string blockPerFile = "ManagedWorkspace.BlocksPerFile";
    const std::string oldValueBlockPerFile = conf.getString(blockPerFile);
    conf.setString(blockPerFile,"9");

    Mantid::DataObjects::ManagedWorkspace2D ws;
    ws.initialize(NHist, NX, NY);
    
    TS_ASSERT_EQUALS( ws.getNumberFiles(), NHist /( 2 * 9 ) + 1 );

    for(size_t i = 0; i < ws.getNumberHistograms(); ++i )
    {
      auto& y = ws.dataY( i );
      for(size_t j = 0; j < y.size(); ++j)
      {
        y[j] = double(1000*i) + double(j);
      }
    }

    for(size_t i = 0; i < ws.getNumberHistograms(); ++i )
    {
      auto& y = ws.dataY( i );
      for(size_t j = 0; j < y.size(); ++j)
      {
        TS_ASSERT_EQUALS( y[j], double(1000*i) + double(j) );
      }
    }

    conf.setString(blocksize,oldValue);
    conf.setString(blockPerFile,oldValueBlockPerFile);
  }

  void testMultipleFiles1()
  {

    const size_t NHist = 211;
    const size_t NY = 9;
    const size_t NX = NY + 1;
    const size_t StartHist = 90;

    double dBlockSize = /*sizeof(int) +*/ ( NX + 2*NY ) * sizeof(double);

    // This will make sure 1 ManagedDataBlock = 1 Vector
    Mantid::Kernel::ConfigServiceImpl& conf = Mantid::Kernel::ConfigService::Instance();
    const std::string blocksize = "ManagedWorkspace.DataBlockSize";
    const std::string oldValue = conf.getString(blocksize);
    conf.setString(blocksize,boost::lexical_cast<std::string>(dBlockSize));

    const std::string blockPerFile = "ManagedWorkspace.BlocksPerFile";
    const std::string oldValueBlockPerFile = conf.getString(blockPerFile);
    conf.setString(blockPerFile,"40");

    Mantid::DataObjects::ManagedWorkspace2D ws;
    ws.initialize(NHist, NX, NY);

    TS_ASSERT_EQUALS( ws.getNumberFiles(), NHist /( 40 ) + 1 );

    // start writing from some index > 0
    for(size_t i = StartHist; i < ws.getNumberHistograms(); ++i )
    {
      auto& y = ws.dataY( i );
      for(size_t j = 0; j < y.size(); ++j)
      {
        y[j] = double(1000*i) + double(j);
      }
    }

    for(size_t i = StartHist; i < ws.getNumberHistograms(); ++i )
    {
      auto& y = ws.dataY( i );
      for(size_t j = 0; j < y.size(); ++j)
      {
        TS_ASSERT_EQUALS( y[j], double(1000*i) + double(j) );
      }
    }

    // check that front spectra can be read and zero
    TS_ASSERT_EQUALS( ws.readY( 0 )[0], 0.0 );
    TS_ASSERT_EQUALS( ws.readY( 1 )[0], 0.0 );

    conf.setString(blocksize,oldValue);
    conf.setString(blockPerFile,oldValueBlockPerFile);
  }

  void testMultipleFiles2()
  {

    const size_t NHist = 211;
    const size_t NY = 9;
    const size_t NX = NY + 1;
    const size_t StartHist = 90;

    double dBlockSize = /*sizeof(int) +*/ ( NX + 2*NY ) * sizeof(double);

    // This will make sure 1 ManagedDataBlock = 1 Vector
    Mantid::Kernel::ConfigServiceImpl& conf = Mantid::Kernel::ConfigService::Instance();
    const std::string blocksize = "ManagedWorkspace.DataBlockSize";
    const std::string oldValue = conf.getString(blocksize);
    conf.setString(blocksize,boost::lexical_cast<std::string>(dBlockSize));

    const std::string blockPerFile = "ManagedWorkspace.BlocksPerFile";
    const std::string oldValueBlockPerFile = conf.getString(blockPerFile);
    conf.setString(blockPerFile,"40");

    Mantid::DataObjects::ManagedWorkspace2D ws;
    ws.initialize(NHist, NX, NY);

    TS_ASSERT_EQUALS( ws.getNumberFiles(), NHist /( 40 ) + 1 );

    // write at front
    ws.dataY( 0 )[0] = 1.0;
    ws.dataY( 1 )[0] = 2.0;

    // leave a gap
    for(size_t i = StartHist; i < ws.getNumberHistograms(); ++i )
    {
      auto& y = ws.dataY( i );
      for(size_t j = 0; j < y.size(); ++j)
      {
        y[j] = double(1000*i) + double(j);
      }
    }

    // check the filled spectra
    for(size_t i = StartHist; i < ws.getNumberHistograms(); ++i )
    {
      auto& y = ws.dataY( i );
      for(size_t j = 0; j < y.size(); ++j)
      {
        TS_ASSERT_EQUALS( y[j], double(1000*i) + double(j) );
      }
    }

    // check that front spectra weren't changed by padding
    TS_ASSERT_EQUALS( ws.readY( 0 )[0], 1.0 );
    TS_ASSERT_EQUALS( ws.readY( 1 )[0], 2.0 );

    conf.setString(blocksize,oldValue);
    conf.setString(blockPerFile,oldValueBlockPerFile);
  }

  void testPadding()
  {
    //std::cout << "Start!!!!" << std::endl;

    // This will make sure 1 ManagedDataBlock = 1 Vector
    Mantid::Kernel::ConfigServiceImpl& conf = Mantid::Kernel::ConfigService::Instance();
    const std::string blocksize = "ManagedWorkspace.DataBlockSize";
    const std::string oldValue = conf.getString(blocksize);
    conf.setString(blocksize,"1");

    const std::string blockPerFile = "ManagedWorkspace.BlocksPerFile";
    const std::string oldValueBlockPerFile = conf.getString(blockPerFile);
    conf.setString(blockPerFile,"10");

    Mantid::DataObjects::ManagedWorkspace2D ws;
    ws.initialize(111,10,9);

    MantidVec fours(10,4.0);
    MantidVec fives(9,5.0);
    MantidVec sixes(9,6.0);
    for ( std::size_t i = 10; i < ws.getNumberHistograms(); ++i )
    {
      ws.dataX(i) = fours;
      ws.dataY(i) = fives;
      ws.dataE(i) = sixes;
    }

    // Get back a block that should have gone out to disk and check its values
    MantidVec xvals = ws.dataX(50);
    MantidVec yvals = ws.dataY(50);
    MantidVec evals = ws.dataE(50);
    TS_ASSERT_EQUALS( xvals.size(), 10 );
    TS_ASSERT_EQUALS( yvals.size(), 9 );
    TS_ASSERT_EQUALS( evals.size(), 9 );
    for ( std::size_t j = 0; j < 9; ++j )
    {
      TS_ASSERT_EQUALS( xvals[j], 4.0 );
      TS_ASSERT_EQUALS( yvals[j], 5.0 );
      TS_ASSERT_EQUALS( evals[j], 6.0 );
    }
    TS_ASSERT_EQUALS( xvals.back(), 4.0 );

    conf.setString(blocksize,oldValue);
    conf.setString(blockPerFile,oldValueBlockPerFile);
    //std::cout << "End!!!! " <<  std::endl;
  }

  void testDestructor()
  {
    std::string filename;
    { // Scoping block
      Mantid::DataObjects::ManagedWorkspace2D tmp;
      tmp.initialize(1,1,1);
      filename = tmp.get_filename() + "0";
      // File should exist
      TS_ASSERT ( Poco::File(filename).exists() );
    }
    TSM_ASSERT ( "File should have been deleted", ! Poco::File(filename).exists() );
  }

private:
  Mantid::DataObjects::ManagedWorkspace2D smallWorkspace;
  Mantid::DataObjects::ManagedWorkspace2D bigWorkspace;
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class ManagedWorkspace2DTestPerformance : public CxxTest::TestSuite
{
private:
  Mantid::API::MatrixWorkspace_sptr inWS;
  Mantid::API::MatrixWorkspace_sptr managedWS;

public:
  static ManagedWorkspace2DTestPerformance *createSuite() { return new ManagedWorkspace2DTestPerformance(); }
  static void destroySuite( ManagedWorkspace2DTestPerformance *suite ) { delete suite; }

  ManagedWorkspace2DTestPerformance()
  {
    // Make sure the input workspace is NOT managed
    Mantid::Kernel::ConfigServiceImpl& conf = Mantid::Kernel::ConfigService::Instance();
    conf.setString("ManagedWorkspace.AlwaysInMemory","1");
    // Workspace should use up around 800 MB of memory
    inWS = Mantid::API::WorkspaceFactory::Instance().create("Workspace2D",7000,5000,5000);
    conf.setString("ManagedWorkspace.AlwaysInMemory","0");
  }

  // This should take ~no time (nothing should be written to disk)
  void testCreationViaFactory()
  {
    // Make sure we go managed
    Mantid::Kernel::ConfigServiceImpl& conf = Mantid::Kernel::ConfigService::Instance();
    const std::string managed = "ManagedWorkspace.LowerMemoryLimit";
    const std::string oldValue = conf.getString(managed);
    conf.setString(managed,"0");
    const std::string managed2 = "ManagedRawFileWorkspace.DoNotUse";
    const std::string oldValue2 = conf.getString(managed2);
    conf.setString(managed2,"0");
    // 1 MB block size
    conf.setString("ManagedWorkspace.DataBlockSize", "1000000");

    Mantid::Kernel::MemoryStats stats;
    stats.update();
    size_t memBefore = stats.availMem() ;

    managedWS = Mantid::API::WorkspaceFactory::Instance().create(inWS);

    stats.update();
    double memLoss = double(memBefore) - double(stats.availMem());
    TSM_ASSERT_LESS_THAN( "Memory used up in creating a ManagedWorkspace should be minimal", memLoss, 20*1024);
    std::cout << memLoss/(1024.0) << " MB of memory used up in creating an empty ManagedWorkspace." << std::endl;
  }

  // This should also take ~no time (nothing should be written to disk)
  void testReadSpectrumNumber()
  {
    Mantid::Kernel::MemoryStats stats;
    stats.update();
    size_t memBefore = stats.availMem() ;

    Mantid::specid_t num(0);
    for ( std::size_t i = 0 ; i < managedWS->getNumberHistograms(); ++i )
    {
      Mantid::API::ISpectrum * spec = managedWS->getSpectrum(i);
      if ( ! spec->hasDetectorID(0) )
      {
        num = spec->getSpectrumNo();
      }
    }
    TS_ASSERT ( num != 0 );

    stats.update();
    double memLoss = double(memBefore) - double(stats.availMem());
    TSM_ASSERT_LESS_THAN( "Memory used up by looping only for spectrum numbers should be minimal", memLoss, 20*1024);
    std::cout << memLoss/(1024.0) << " MB of memory used up in looping looking only for spectra." << std::endl;
  }

  // This should take a while...
  void testLoopOverHalf()
  {
    Mantid::Kernel::MemoryStats stats;

    // Temporary while I ensure that the memory-per-process code works on each platform
#if _WIN32
    size_t processMemBefore = stats.residentMem();
#else
    size_t memBefore = stats.availMem();
#endif

    boost::shared_ptr<ManagedWorkspace2D> ws = boost::dynamic_pointer_cast<ManagedWorkspace2D>(managedWS);
    TSM_ASSERT("Workspace is really managed", ws);

    for ( std::size_t i = 0; i < 3500; ++i )
    {
      managedWS->dataX(i) = inWS->readX(i);
      managedWS->dataY(i) = inWS->readY(i);
      managedWS->dataE(i) = inWS->readE(i);
    }
    // For linux, make sure to release old memory
    Mantid::API::MemoryManager::Instance().releaseFreeMemory();
    stats.update();

#if _WIN32
    size_t processMemNow = stats.residentMem();
    double memLoss = static_cast<double>(processMemNow) - static_cast<double>(processMemBefore);
#else
    size_t memNow =  stats.availMem();
    double memLoss = static_cast<double>(memBefore) - static_cast<double>(memNow);
#endif
    
    TSM_ASSERT_LESS_THAN( "MRU list should limit the amount of memory to around 100 MB used when accessing the data.", memLoss, 200*1024);
    std::cout << memLoss/(1024.0) << " MB of memory used up in looping. Memory looped over = " << 3500.0*5000*24 / (1024.0*1024.0) << " MB." << std::endl;
  }

  // ...but only about half as long as this
  void testLoopOverWhole()
  {
    Mantid::API::MatrixWorkspace_sptr managedWS2 = Mantid::API::WorkspaceFactory::Instance().create(inWS);
    for ( std::size_t i = 0 ; i < managedWS2->getNumberHistograms(); ++i )
    {
      managedWS2->dataX(i) = inWS->readX(i);
      managedWS2->dataY(i) = inWS->readY(i);
      managedWS2->dataE(i) = inWS->readE(i);
    }
  }
};
#endif /*MANAGEDWORKSPACE2DTEST_H_*/
