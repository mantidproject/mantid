#ifndef __SLICEMDHISTO
#define __SLICEMDHISTO


#include <cxxtest/TestSuite.h>
#include "MantidSINQ/SliceMDHisto.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidDataObjects/MDHistoWorkspace.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid;
using namespace Mantid::DataObjects;

class SliceMDHistoTest: public CxxTest::TestSuite {
public:

  void testName() {
    SliceMDHisto loader;
    TS_ASSERT_EQUALS( loader.name(), "SliceMDHisto");
  }

  void testInit() {
    SliceMDHisto loader;
    TS_ASSERT_THROWS_NOTHING( loader.initialize());
    TS_ASSERT( loader.isInitialized());
  }

  void testExec()
  {
    makeTestMD();
    
    SliceMDHisto pmd;
    pmd.initialize();
    pmd.setPropertyValue("InputWorkspace", "PMDTest");
    pmd.setPropertyValue("Start", "50,60,100");
    pmd.setPropertyValue("End", "100,120,200");
    std::string outputSpace = "PMD_out";
    pmd.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING( pmd.execute());

    // test data 
    IMDHistoWorkspace_sptr data =  
      AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(
								    outputSpace);
    long nBin = static_cast<long>(data->getNPoints());
    long sum = 0;
    double *sdata = data->getSignalArray();
    for(long i = 0; i < nBin; i++){
      sum += (long)sdata[i];
    }
    TS_ASSERT_EQUALS(sum,300000);

    // test dimensions
    boost::shared_ptr<const IMDDimension> dimi = data->getDimension(0);
    TS_ASSERT_EQUALS(dimi->getNBins(),50);
    TS_ASSERT_DELTA(dimi->getMinimum(),0,.1);
    TS_ASSERT_DELTA(dimi->getMaximum(),50.,.1);

    dimi = data->getDimension(1);
    TS_ASSERT_EQUALS(dimi->getNBins(),60);
    TS_ASSERT_DELTA(dimi->getMinimum(),0,.1);
    TS_ASSERT_DELTA(dimi->getMaximum(),60.,.1);

    dimi = data->getDimension(2);
    TS_ASSERT_EQUALS(dimi->getNBins(),100);
    TS_ASSERT_DELTA(dimi->getMinimum(),0,.1);
    TS_ASSERT_DELTA(dimi->getMaximum(),100.,.1);

    // test some meta data
    std::string title = data->getTitle();
    size_t found = title.find("Hugo");
    TS_ASSERT_DIFFERS(found,std::string::npos);

    ExperimentInfo_sptr info;
    info = data->getExperimentInfo(0);
    const Run r = info->run();
    Mantid::Kernel::Property *p = r.getProperty("Gwendolin"); 
    std::string cd = p->value();
    found = cd.find("27.8");
    TS_ASSERT_DIFFERS(found,std::string::npos);


    AnalysisDataService::Instance().clear();

  }

 private:
  MDHistoWorkspace_sptr makeTestMD()
  {
    IMDDimension_sptr dim;
    std::vector<IMDDimension_sptr> dimensions;
    dim = MDHistoDimension_sptr( new MDHistoDimension(std::string("x"),
						      std::string("ID0"), std::string("mm"),
						      coord_t(-50),coord_t(50),size_t(100)));  
    dimensions.push_back(boost::const_pointer_cast<IMDDimension>(dim));
    dim = MDHistoDimension_sptr( new MDHistoDimension(std::string("y"),
						      std::string("ID1"), std::string("mm"),
						      coord_t(-60),coord_t(60),size_t(120)));  
    dimensions.push_back(boost::const_pointer_cast<IMDDimension>(dim));
    dim = MDHistoDimension_sptr( new MDHistoDimension(std::string("z"),
						      std::string("ID2"), std::string("mm"),
						      coord_t(-100),coord_t(100),size_t(200)));  
    dimensions.push_back(boost::const_pointer_cast<IMDDimension>(dim));

    MDHistoWorkspace_sptr outWS (new MDHistoWorkspace(dimensions));
    outWS->setTo(1.,1.,.0);
    outWS->setTitle("Hugo");
    outWS->addExperimentInfo((ExperimentInfo_sptr)new ExperimentInfo());
    Run &rr = outWS->getExperimentInfo(0)->mutableRun();
    rr.addProperty("Gwendolin",27.8,true);

    AnalysisDataService::Instance().add("PMDTest",outWS);
    return outWS;
  }

};




#endif
