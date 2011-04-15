#ifndef LOADNEXUSMONITORSTEST_H_
#define LOADNEXUSMONITORSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidNexus/LoadNexusMonitors.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidDataObjects/Workspace2D.h"
#include <boost/shared_ptr.hpp>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;

class LoadNexusMonitorsTest : public CxxTest::TestSuite
{
public:
  void testExec()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadNexusMonitors ld;
    std::string outws_name = "cncs";
    ld.initialize();
    ld.setPropertyValue("Filename","CNCS_7860_event.nxs");
    ld.setPropertyValue("OutputWorkspace", outws_name);

    ld.execute();
    TS_ASSERT( ld.isExecuted() );

    MatrixWorkspace_sptr WS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
    //Valid WS and it is an MatrixWorkspace
    TS_ASSERT( WS );
    //Correct number of monitors found
    TS_ASSERT_EQUALS( WS->getNumberHistograms(), 3 );
    // Check some histogram data
    // TOF
    TS_ASSERT_EQUALS( (*WS->refX(1)).size(), 200002 );
    TS_ASSERT_DELTA( (*WS->refX(1))[3412], 3412.0, 1e-6 );
    // Data
    TS_ASSERT_EQUALS( WS->dataY(1).size(), 200001 );
    TS_ASSERT_DELTA( WS->dataY(1)[3412], 197., 1e-6 );
    // Error
    TS_ASSERT_EQUALS( WS->dataE(1).size(), 200001 );
    TS_ASSERT_DELTA( WS->dataE(1)[3412], 14.03567, 1e-4 );
    // Check geometry for a monitor
    IDetector_sptr mon = WS->getDetector(2);
    TS_ASSERT( mon->isMonitor() );
    TS_ASSERT_EQUALS( mon->getID(), -3 );
    boost::shared_ptr<IComponent> sample = WS->getInstrument()->getSample();
    TS_ASSERT_DELTA( mon->getDistance(*sample), 1.426, 1e-6 );
  }
};

#endif /*LOADNEXUSMONITORSTEST_H_*/
