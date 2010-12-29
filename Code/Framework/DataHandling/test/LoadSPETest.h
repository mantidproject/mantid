#ifndef LOADSPETEST_H_
#define LOADSPETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadSPE.h"

using namespace Mantid::API;

class LoadSPETest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( loader.name(), "LoadSPE" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( loader.version(), 1 );
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( loader.category(), "DataHandling" );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( loader.initialize() );
    TS_ASSERT( loader.isInitialized() );
  }

  void testExec()
  {
    if ( !loader.isInitialized() ) loader.initialize();

    std::string outWS("outWS");

    TS_ASSERT_THROWS_NOTHING( loader.setPropertyValue("Filename","../../../../Test/AutoTestData/Example.spe") );
    TS_ASSERT_THROWS_NOTHING( loader.setPropertyValue("OutputWorkspace",outWS) );

    TS_ASSERT_THROWS_NOTHING( loader.execute() );
    TS_ASSERT( loader.isExecuted() );

    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<MatrixWorkspace>
                                (AnalysisDataService::Instance().retrieve(outWS)) );

    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 32 );
    TS_ASSERT_EQUALS( ws->blocksize(), 195 );
    TS_ASSERT( ws->isDistribution() );
    TS_ASSERT_EQUALS( ws->YUnit(), "" );
    TS_ASSERT_EQUALS( ws->YUnitLabel(), "S(Phi,Energy)" );
    TS_ASSERT_EQUALS( ws->getAxis(0)->unit()->unitID(), "DeltaE" );
    TS_ASSERT_EQUALS( ws->getAxis(1)->unit()->caption(), "Phi" );
    TS_ASSERT( ws->getAxis(1)->isNumeric() );
    TS_ASSERT_EQUALS( (*(ws->getAxis(1)))(0), 0.5 );
    TS_ASSERT_EQUALS( (*(ws->getAxis(1)))(12), 12.5 );
    TS_ASSERT_EQUALS( (*(ws->getAxis(1)))(32), 32.5 );

    TS_ASSERT_EQUALS( ws->readX(0)[0], -20.0 );
    TS_ASSERT_EQUALS( ws->readX(22)[86], -2.8 );
    TS_ASSERT_EQUALS( ws->readX(31)[195], 19.0 );

    TS_ASSERT_DIFFERS( ws->readY(4)[99], ws->readY(4)[99] );
    TS_ASSERT_EQUALS( ws->readY(5)[0], 0.0 );
    TS_ASSERT_EQUALS( ws->readY(9)[48], -3.911 );
    TS_ASSERT_EQUALS( ws->readY(13)[137], 4.313 );
    TS_ASSERT_EQUALS( ws->readY(31)[194], 158.9 );

    TS_ASSERT_EQUALS( ws->readE(4)[173], 0.0 );
    TS_ASSERT_EQUALS( ws->readE(9)[111], 16.48 );
    TS_ASSERT_EQUALS( ws->readE(18)[0], 0.0 );
    TS_ASSERT_EQUALS( ws->readE(26)[35], 4.908 );
    TS_ASSERT_EQUALS( ws->readE(31)[194], 60.38 );

    AnalysisDataService::Instance().remove(outWS);
  }

private:
  Mantid::DataHandling::LoadSPE loader;
};

#endif /*LoadSPETEST_H_*/
