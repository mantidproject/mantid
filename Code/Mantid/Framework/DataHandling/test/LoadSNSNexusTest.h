#ifndef LOADSNSNEXUSTEST_H_
#define LOADSNSNEXUSTEST_H_

#include "MantidDataHandling/LoadInstrument.h" 
#include "MantidDataHandling/LoadSNSNexus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FrameworkManager.h"
using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::NeXus;
using namespace Mantid::API;
using namespace Mantid::Kernel;

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceGroup.h"
#include <iostream>

class LoadSNSNexusTest : public CxxTest::TestSuite
{
public:
  void xtestCNCS()
  {
      Mantid::API::FrameworkManager::Instance();
      LoadSNSNexus ld;
      std::string outws_name = "CNCS_7860";
      ld.initialize();
      ld.setPropertyValue("Filename","CNCS_7860.nxs");
      ld.setPropertyValue("OutputWorkspace",outws_name);
      TS_ASSERT_THROWS_NOTHING(ld.execute());
      TS_ASSERT(ld.isExecuted());
  }

  void xtestRefl()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadSNSNexus ld;
        std::string outws_name = "nickr0x0r";
        ld.initialize();
        ld.setPropertyValue("Filename","../../../../Test/Nexus/SNS/REF_L_16055.nxs");
        ld.setPropertyValue("OutputWorkspace",outws_name);
        TS_ASSERT_THROWS_NOTHING(ld.execute());
        TS_ASSERT(ld.isExecuted());

        MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
        TS_ASSERT(ws);
        TS_ASSERT_EQUALS(ws->blocksize(),501);
        TS_ASSERT_EQUALS(ws->getNumberHistograms(),77824);
        TS_ASSERT_EQUALS(ws->readX(0)[0],0.);
        TS_ASSERT_EQUALS(ws->readX(0)[1],200.);
        TS_ASSERT_EQUALS(ws->readX(0)[2],400.);

        TS_ASSERT_EQUALS(ws->readY(41799)[62],191.);
        TS_ASSERT_EQUALS(ws->readY(51223)[66],8.);
        TS_ASSERT_EQUALS(ws->readY(13873)[227],1.);

        TS_ASSERT_EQUALS(ws->spectraMap().nElements(),77824);

        const std::vector< Property* >& logs = ws->run().getLogData();
        TS_ASSERT_EQUALS(logs.size(),1);

        //------------ Instrument Loading Sub-Test -----------------------
        IInstrument_sptr inst = ws->getInstrument();

        TS_ASSERT_EQUALS(inst->getName(), "REF_L");
        std::map<int, Geometry::IDetector_sptr> detectors = inst->getDetectors();
        TS_ASSERT_EQUALS(detectors.size(), 304*256); //304*256

        V3D pos(0,0,0);
        //Test a few pixels in bank 1
        pos.spherical_rad(1.3571243, 0.1025134, -0.6979992);
        TS_ASSERT(detectors[0]->getRelativePos() == pos);
        TS_ASSERT_EQUALS(detectors[0]->getName(), "bank1, (0,0)");
        //Pixel 303
        pos.spherical_rad(1.3570696, 0.10212083, -2.4403417);
        TS_ASSERT(detectors[303]->getRelativePos() == pos);
        TS_ASSERT_EQUALS(detectors[304]->getName(), "bank1, (1,0)");

/*
        TimeSeriesProperty<std::string>* slog = dynamic_cast<TimeSeriesProperty<std::string>*>(ws->run().getLogData("icp_event"));
        TS_ASSERT(slog);
        std::string str = slog->value();
        TS_ASSERT_EQUALS(str.size(),1023);
        TS_ASSERT_EQUALS(str.substr(0,37),"2009-Apr-28 09:20:29  CHANGE_PERIOD 1");

        slog = dynamic_cast<TimeSeriesProperty<std::string>*>(ws->run().getLogData("icp_debug"));
        TS_ASSERT(slog);
        TS_ASSERT_EQUALS(slog->size(),50);

        TimeSeriesProperty<double>* dlog = dynamic_cast<TimeSeriesProperty<double>*>(ws->run().getLogData("total_counts"));
        TS_ASSERT(dlog);
        TS_ASSERT_EQUALS(dlog->size(),172);

        dlog = dynamic_cast<TimeSeriesProperty<double>*>(ws->run().getLogData("period"));
        TS_ASSERT(dlog);
        TS_ASSERT_EQUALS(dlog->size(),172);

        TimeSeriesProperty<bool>* blog = dynamic_cast<TimeSeriesProperty<bool>*>(ws->run().getLogData("period 1"));
        TS_ASSERT(blog);
        TS_ASSERT_EQUALS(blog->size(),1);
*/
        //TS_ASSERT_EQUALS(ws->sample().getName(),"NONE");
    }
};

#endif /*LOADSNSNEXUSTEST_H_*/
