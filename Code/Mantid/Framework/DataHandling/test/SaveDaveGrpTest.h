#ifndef MANTID_DATAHANDLING_SAVEDAVEGRPTEST_H_
#define MANTID_DATAHANDLING_SAVEDAVEGRPTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/SaveDaveGrp.h"
#include "MantidDataHandling/LoadDaveGrp.h"
using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class SaveDaveGrpTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    SaveDaveGrp alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
  }

  void test_exec_event()
  {
      Mantid::API::FrameworkManager::Instance();
      LoadEventNexus ld;
      std::string outws = "cncs";
      ld.initialize();
      ld.setPropertyValue("Filename","CNCS_7860_event.nxs");
      ld.setPropertyValue("OutputWorkspace",outws);
      ld.setPropertyValue("Precount", "0");
      ld.execute();
      TS_ASSERT( ld.isExecuted() );

      AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
      TS_ASSERT_EQUALS( dataStore.doesExist(outws), true);
      Workspace_sptr output;
      TS_ASSERT_THROWS_NOTHING(output = dataStore.retrieve(outws));
      MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(output);
/*
      std::cout<<outputWS->getAxis(0)->unit()->unitID()<<std::endl;
      std::cout<<outputWS->getAxis(0)->unit()->label()<<std::endl;
      std::cout<<outputWS->getAxis(0)->unit()->caption()<<std::endl;
      std::cout<<outputWS->getAxis(1)->unit()->unitID()<<std::endl;
      std::cout<<outputWS->getAxis(1)->length()<<std::endl;
      std::cout<<outputWS->getAxis(0)->length()<<std::endl;
      std::vector<double> data=outputWS->readX(0);
      std::vector<double>::iterator it;
      std::cout<<"Yaxis"<<std::endl;
      std::cout<<"Xaxis"<<std::endl;
      for (it=data.begin();it!=data.end();++it) std::cout<<*it<<std::endl;
      std::cout<<"Histogram"<<outputWS->isHistogramData();
      */
  }
  



};


#endif /* MANTID_DATAHANDLING_SAVEDAVEGRPTEST_H_ */

