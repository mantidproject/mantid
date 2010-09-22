#ifndef LOADISISNEXUSTEST_H_
#define LOADISISNEXUSTEST_H_

#include "MantidDataHandling/LoadInstrument.h" 

#include "MantidNexus/LoadISISNexus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidNexus/LoadISISNexus2.h"
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceGroup.h"

class LoadISISNexusTest : public CxxTest::TestSuite
{
public:
    void testExec()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadISISNexus2 ld;
        ld.initialize();
        ld.setPropertyValue("Filename","../../../../Test/AutoTestData/LOQ49886.nxs");
        ld.setPropertyValue("OutputWorkspace","outWS");
        TS_ASSERT_THROWS_NOTHING(ld.execute());
        TS_ASSERT(ld.isExecuted());

        MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outWS"));
        TS_ASSERT_EQUALS(ws->blocksize(),5);
        TS_ASSERT_EQUALS(ws->getNumberHistograms(),17792);
        TS_ASSERT_EQUALS(ws->readX(0)[0],5.);
        TS_ASSERT_EQUALS(ws->readX(0)[1],4005.);
        TS_ASSERT_EQUALS(ws->readX(0)[2],8005.);
        
        TS_ASSERT_EQUALS(ws->readY(5)[1],1.);
        TS_ASSERT_EQUALS(ws->readY(6)[0],1.);
        TS_ASSERT_EQUALS(ws->readY(8)[3],1.);

        TS_ASSERT_EQUALS(ws->spectraMap().nElements(),17792);

        const std::vector< Property* >& logs = ws->run().getLogData();
        TS_ASSERT_EQUALS(logs.size(), 52);

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

        TS_ASSERT_EQUALS(ws->sample().getName(),"");
        
        Property *l_property = ws->run().getLogData( "run_number" );
        TS_ASSERT_EQUALS( l_property->value(), "49886" );
    }
    void testExec2()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadISISNexus2 ld;
        ld.initialize();
        ld.setPropertyValue("Filename","../../../../Test/AutoTestData/LOQ49886.nxs");
        ld.setPropertyValue("OutputWorkspace","outWS");
        ld.setPropertyValue("SpectrumMin","10");
        ld.setPropertyValue("SpectrumMax","20");
        ld.setPropertyValue("SpectrumList","30,33,38");
        TS_ASSERT_THROWS_NOTHING(ld.execute());
        TS_ASSERT(ld.isExecuted());

	    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outWS"));
        TS_ASSERT_EQUALS(ws->blocksize(),5);
        TS_ASSERT_EQUALS(ws->getNumberHistograms(),14);

        TS_ASSERT_EQUALS(ws->readX(0)[0],5.);
        TS_ASSERT_EQUALS(ws->readX(0)[1],4005.);
        TS_ASSERT_EQUALS(ws->readX(0)[2],8005.);
        
        TS_ASSERT_EQUALS(ws->readY(5)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(6)[0],0.);
        TS_ASSERT_EQUALS(ws->readY(8)[3],0.);

        TS_ASSERT_EQUALS(ws->readY(8)[1],2.);
        TS_ASSERT_EQUALS(ws->readY(10)[3],1.);
        TS_ASSERT_EQUALS(ws->readY(13)[4],1.);
    }
	 void testMultiPeriodEntryNumberZero()
    {
		Mantid::API::FrameworkManager::Instance();
        LoadISISNexus2 ld;
        ld.initialize();
        ld.setPropertyValue("Filename","../../../../Test/AutoTestData/TEST00000008.nxs");
	    ld.setPropertyValue("OutputWorkspace","outWS");
        ld.setPropertyValue("SpectrumMin","10");
        ld.setPropertyValue("SpectrumMax","19");
 		ld.setPropertyValue("EntryNumber","0");
		//ld.setPropertyValue("SpectrumList","30,31");
        TS_ASSERT_THROWS_NOTHING(ld.execute());
        TS_ASSERT(ld.isExecuted());
		
		WorkspaceGroup_sptr grpout;//=WorkspaceGroup_sptr(new WorkspaceGroup);
		TS_ASSERT_THROWS_NOTHING(grpout=boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("outWS")));

        MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outWS_1"));
        TS_ASSERT_EQUALS(ws->blocksize(),995);
        TS_ASSERT_EQUALS(ws->getNumberHistograms(),10);
	TS_ASSERT_DELTA(ws->run().getProtonCharge(), 0.069991, 1e-6);

        TS_ASSERT_EQUALS(ws->readX(0)[0],5.);
        TS_ASSERT_EQUALS(ws->readX(0)[1],6.);
        TS_ASSERT_EQUALS(ws->readX(0)[2],7.);
        
        TS_ASSERT_EQUALS(ws->readY(5)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(6)[0],0.);
        TS_ASSERT_EQUALS(ws->readY(8)[3],0.);

        TS_ASSERT_EQUALS(ws->readY(7)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(9)[3],0.);
        TS_ASSERT_EQUALS(ws->readY(9)[1],0.);
    }
	  void testMultiPeriodEntryNumberNonZero()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadISISNexus2 ld;
        ld.initialize();
        ld.setPropertyValue("Filename","../../../../Test/AutoTestData/TEST00000008.nxs");
        ld.setPropertyValue("OutputWorkspace","outWS");
        ld.setPropertyValue("SpectrumMin","10");
        ld.setPropertyValue("SpectrumMax","20");
	//	ld.setPropertyValue("SpectrumList","29,30,31");
  		ld.setPropertyValue("EntryNumber","5");
        TS_ASSERT_THROWS_NOTHING(ld.execute());
        TS_ASSERT(ld.isExecuted());
			

        MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outWS"));
        TS_ASSERT_EQUALS(ws->blocksize(),995);
     //   TS_ASSERT_EQUALS(ws->getNumberHistograms(),14);
        TS_ASSERT_EQUALS(ws->getTitle(), "hello\\0");
	TS_ASSERT_DELTA(ws->run().getProtonCharge(), 0.069991, 1e-6);
        TS_ASSERT_EQUALS(ws->readX(0)[0],5.);
        TS_ASSERT_EQUALS(ws->readX(0)[1],6.);
        TS_ASSERT_EQUALS(ws->readX(0)[2],7.);
        
        TS_ASSERT_EQUALS(ws->readY(5)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(6)[0],0.);
        TS_ASSERT_EQUALS(ws->readY(8)[3],0.);

        TS_ASSERT_EQUALS(ws->readY(7)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(9)[3],0.);
        TS_ASSERT_EQUALS(ws->readY(9)[1],0.);
    }
};

#endif /*LOADISISNEXUSTEST_H_*/
