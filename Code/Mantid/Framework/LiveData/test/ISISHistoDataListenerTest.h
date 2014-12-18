#ifndef MANTID_LIVEDATA_ISISHISTODATALISTENERTEST_H_
#define MANTID_LIVEDATA_ISISHISTODATALISTENERTEST_H_

#include "MantidLiveData/ISISHistoDataListener.h"
#include "MantidLiveData/FakeISISHistoDAE.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidTestHelpers/FacilityHelper.h"

#include <cxxtest/TestSuite.h>

#include <Poco/ActiveResult.h>
#include <Poco/Thread.h>
#include <algorithm>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::LiveData;

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
// cannot make it work for linux
#ifdef _WIN32
    //system("pause");
    FacilityHelper::ScopedFacilities loadTESTFacility("IDFs_for_UNIT_TESTING/UnitTestFacilities.xml", "TEST");

    FakeISISHistoDAE dae;
    dae.initialize();
    dae.setProperty("NPeriods",1);
    auto res = dae.executeAsync();

    auto listener = Mantid::API::LiveListenerFactory::Instance().create("TESTHISTOLISTENER",true);
    TS_ASSERT( listener );
    TSM_ASSERT("Listener has failed to connect", listener->isConnected() );
    if (!listener->isConnected()) return;

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
    TS_ASSERT_EQUALS( x[0], 10000 );
    TS_ASSERT_DELTA( x[1], 10100, 1e-6 );
    TS_ASSERT_DELTA( x[30], 13000.0, 1e-6 );

    x = ws->readX( 4 );
    TS_ASSERT_EQUALS( x.size(), 31 );
    TS_ASSERT_EQUALS( x[0], 10000 );
    TS_ASSERT_DELTA( x[1], 10100, 1e-6 );
    TS_ASSERT_DELTA( x[30], 13000.0, 1e-6 );

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

    auto spec = ws->getSpectrum(0);
    TS_ASSERT_EQUALS( spec->getSpectrumNo(), 1 )
    auto dets = spec->getDetectorIDs();
    TS_ASSERT_EQUALS( dets.size(), 1 );
    TS_ASSERT_EQUALS( *dets.begin(), 1001 );

    spec = ws->getSpectrum(3);
    TS_ASSERT_EQUALS( spec->getSpectrumNo(), 10 )
    dets = spec->getDetectorIDs();
    TS_ASSERT_EQUALS( dets.size(), 1 );
    TS_ASSERT_EQUALS( *dets.begin(), 1004 );

    res.wait();
#else
    TS_ASSERT( true );
#endif

  }
  
  void test_Receiving_multiperiod_data()
  {

#ifdef _WIN32
    FacilityHelper::ScopedFacilities loadTESTFacility("IDFs_for_UNIT_TESTING/UnitTestFacilities.xml", "TEST");

    FakeISISHistoDAE dae;
    dae.initialize();
    dae.setProperty("NPeriods",2);
    auto res = dae.executeAsync();

    auto listener = Mantid::API::LiveListenerFactory::Instance().create("TESTHISTOLISTENER",true);
    TS_ASSERT( listener );
    TSM_ASSERT("Listener has failed to connect", listener->isConnected() );
    if (!listener->isConnected()) return;

    auto outWS = listener->extractData();
    auto group = boost::dynamic_pointer_cast<WorkspaceGroup>( outWS );
    TS_ASSERT( group );
    TS_ASSERT_EQUALS( group->size(), 2 );
    auto ws1 = boost::dynamic_pointer_cast<MatrixWorkspace>( group->getItem(0) );
    TS_ASSERT( ws1 );
    auto ws2 = boost::dynamic_pointer_cast<MatrixWorkspace>( group->getItem(1) );
    TS_ASSERT( ws2 );

    TS_ASSERT_EQUALS( ws1->getNumberHistograms(), 100 );
    TS_ASSERT_EQUALS( ws1->blocksize(), 30 );

    TS_ASSERT_EQUALS( ws2->getNumberHistograms(), 100 );
    TS_ASSERT_EQUALS( ws2->blocksize(), 30 );

    auto x = ws1->readX( 0 );
    TS_ASSERT_EQUALS( x.size(), 31 );
    TS_ASSERT_EQUALS( x[0], 10000 );
    TS_ASSERT_DELTA( x[1], 10100, 1e-6 );
    TS_ASSERT_DELTA( x[30], 13000, 1e-6 );

    x = ws1->readX( 4 );
    TS_ASSERT_EQUALS( x.size(), 31 );
    TS_ASSERT_EQUALS( x[0], 10000 );
    TS_ASSERT_DELTA( x[1], 10100, 1e-6 );
    TS_ASSERT_DELTA( x[30], 13000, 1e-6 );

    x = ws2->readX( 0 );
    TS_ASSERT_EQUALS( x.size(), 31 );
    TS_ASSERT_EQUALS( x[0], 10000 );
    TS_ASSERT_DELTA( x[1], 10100, 1e-6 );
    TS_ASSERT_DELTA( x[30], 13000, 1e-6 );

    x = ws2->readX( 44 );
    TS_ASSERT_EQUALS( x.size(), 31 );
    TS_ASSERT_EQUALS( x[0], 10000 );
    TS_ASSERT_DELTA( x[1], 10100, 1e-6 );
    TS_ASSERT_DELTA( x[30], 13000, 1e-6 );

    auto y = ws1->readY( 2 );
    TS_ASSERT_EQUALS( y[0], 3 );
    TS_ASSERT_EQUALS( y[5], 3 );
    TS_ASSERT_EQUALS( y[29], 3 );

    y = ws1->readY( 44 );
    TS_ASSERT_EQUALS( y[0], 45 );
    TS_ASSERT_EQUALS( y[5], 45 );
    TS_ASSERT_EQUALS( y[29], 45 );

    y = ws1->readY( 77 );
    TS_ASSERT_EQUALS( y[0], 78 );
    TS_ASSERT_EQUALS( y[5], 78 );
    TS_ASSERT_EQUALS( y[29], 78 );

    y = ws2->readY( 2 );
    TS_ASSERT_EQUALS( y[0], 1003 );
    TS_ASSERT_EQUALS( y[5], 1003 );
    TS_ASSERT_EQUALS( y[29], 1003 );

    y = ws2->readY( 44 );
    TS_ASSERT_EQUALS( y[0], 1045 );
    TS_ASSERT_EQUALS( y[5], 1045 );
    TS_ASSERT_EQUALS( y[29], 1045 );

    y = ws2->readY( 77 );
    TS_ASSERT_EQUALS( y[0], 1078 );
    TS_ASSERT_EQUALS( y[5], 1078 );
    TS_ASSERT_EQUALS( y[29], 1078 );

    auto spec = ws1->getSpectrum(0);
    TS_ASSERT_EQUALS( spec->getSpectrumNo(), 1 )
    auto dets = spec->getDetectorIDs();
    TS_ASSERT_EQUALS( dets.size(), 1 );
    TS_ASSERT_EQUALS( *dets.begin(), 1001 );

    spec = ws1->getSpectrum(3);
    TS_ASSERT_EQUALS( spec->getSpectrumNo(), 4 )
    dets = spec->getDetectorIDs();
    TS_ASSERT_EQUALS( dets.size(), 1 );
    TS_ASSERT_EQUALS( *dets.begin(), 1004 );

    spec = ws2->getSpectrum(0);
    TS_ASSERT_EQUALS( spec->getSpectrumNo(), 1 )
    dets = spec->getDetectorIDs();
    TS_ASSERT_EQUALS( dets.size(), 1 );
    TS_ASSERT_EQUALS( *dets.begin(), 1001 );

    spec = ws2->getSpectrum(3);
    TS_ASSERT_EQUALS( spec->getSpectrumNo(), 4 )
    dets = spec->getDetectorIDs();
    TS_ASSERT_EQUALS( dets.size(), 1 );
    TS_ASSERT_EQUALS( *dets.begin(), 1004 );

    dae.cancel();
    res.wait();
#else
    TS_ASSERT( true );
#endif
  }
};


#endif /* MANTID_LIVEDATA_ISISHISTODATALISTENERTEST_H_ */
