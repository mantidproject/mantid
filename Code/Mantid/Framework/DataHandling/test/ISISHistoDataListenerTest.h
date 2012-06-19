#ifndef MANTID_DATAHANDLING_ISISHISTODATALISTENERTEST_H_
#define MANTID_DATAHANDLING_ISISHISTODATALISTENERTEST_H_

#include "MantidDataHandling/ISISHistoDataListener.h"
#include "MantidDataHandling/FakeISISHistoDAE.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/ISpectraDetectorMap.h"
#include <cxxtest/TestSuite.h>

#include <Poco/Thread.h>
#include <algorithm>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;

class ISISHistoDataListenerTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ISISHistoDataListenerTest *createSuite() { return new ISISHistoDataListenerTest(); }
  static void destroySuite( ISISHistoDataListenerTest *suite ) { delete suite; }

  ISISHistoDataListenerTest()
  {
    Mantid::API::FrameworkManager::Instance();
  }

  void test_Receiving_data()
  {

    //system("pause");
    FakeISISHistoDAE dae;
    dae.initialize();
    dae.setProperty("NPeriods",1);
    auto res = dae.executeAsync();

    auto listener = Mantid::API::LiveListenerFactory::Instance().create("TESTHISTOLISTENER");
    TS_ASSERT( listener );
    TS_ASSERT( listener->isConnected() );

    int s[] = {1,2,3,10,11,95,96,97,98,99,100};
    std::vector<specid_t> specs;
    specs.assign( s, s + 11 );
    listener->setSpectra( specs );
    auto outWS = listener->extractData();
    auto ws = boost::dynamic_pointer_cast<API::MatrixWorkspace>( outWS );
    //TS_ASSERT( ws );
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 11 );
    TS_ASSERT_EQUALS( ws->blocksize(), 30 );

    dae.cancel();

    auto x = ws->readX( 0 );
    TS_ASSERT_EQUALS( x.size(), 31 );
    TS_ASSERT_EQUALS( x[0], 0 );
    TS_ASSERT_DELTA( x[1], 0.1, 1e-6 );
    TS_ASSERT_DELTA( x[30], 3.0, 1e-6 );

    x = ws->readX( 4 );
    TS_ASSERT_EQUALS( x.size(), 31 );
    TS_ASSERT_EQUALS( x[0], 0 );
    TS_ASSERT_DELTA( x[1], 0.1, 1e-6 );
    TS_ASSERT_DELTA( x[30], 3.0, 1e-6 );

    auto y = ws->readY( 2 );
    TS_ASSERT_EQUALS( y[0], 3 );
    TS_ASSERT_EQUALS( y[5], 3 );
    TS_ASSERT_EQUALS( y[29], 3 );

    y = ws->readY( 4 );
    TS_ASSERT_EQUALS( y[0], 11 );
    TS_ASSERT_EQUALS( y[5], 11 );
    TS_ASSERT_EQUALS( y[29], 11 );

    y = ws->readY( 7 );
    TS_ASSERT_EQUALS( y[0], 97 );
    TS_ASSERT_EQUALS( y[5], 97 );
    TS_ASSERT_EQUALS( y[29], 97 );

    auto e = ws->readE( 2 );
    TS_ASSERT_EQUALS( e[0], sqrt(3.0) );
    TS_ASSERT_EQUALS( e[5], sqrt(3.0) );
    TS_ASSERT_EQUALS( e[29], sqrt(3.0) );

    e = ws->readE( 4 );
    TS_ASSERT_EQUALS( e[0], sqrt(11.0) );
    TS_ASSERT_EQUALS( e[5], sqrt(11.0) );
    TS_ASSERT_EQUALS( e[29], sqrt(11.0) );

    e = ws->readE( 7 );
    TS_ASSERT_EQUALS( e[0], sqrt(97.0) );
    TS_ASSERT_EQUALS( e[5], sqrt(97.0) );
    TS_ASSERT_EQUALS( e[29], sqrt(97.0) );

    auto& sm = ws->spectraMap();
    TS_ASSERT_EQUALS( sm.nSpectra(), 100 );

    auto d = sm.getDetectors( 1 );
    TS_ASSERT_EQUALS( d.size(), 1 );
    TS_ASSERT_EQUALS( d[0], 1001 );

    d = sm.getDetectors( 4 );
    TS_ASSERT_EQUALS( d.size(), 1 );
    TS_ASSERT_EQUALS( d[0], 1004 );

    d = sm.getDetectors( 100 );
    TS_ASSERT_EQUALS( d.size(), 1 );
    TS_ASSERT_EQUALS( d[0], 1100 );

    res.wait();

  }
  
  void xtest_Receiving_multiperiod_data()
  {

    FakeISISHistoDAE dae;
    dae.initialize();
    dae.setProperty("NPeriods",2);
    auto res = dae.executeAsync();

    auto listener = Mantid::API::LiveListenerFactory::Instance().create("TESTHISTOLISTENER");
    TS_ASSERT( listener );
    TS_ASSERT( listener->isConnected() );

    auto outWS = listener->extractData();
    //TS_ASSERT_EQUALS( ws->getNumberHistograms(), 100 );
    //TS_ASSERT_EQUALS( ws->blocksize(), 30 );

    dae.cancel();
    res.wait();
  }
};


#endif /* MANTID_DATAHANDLING_ISISHISTODATALISTENERTEST_H_ */
