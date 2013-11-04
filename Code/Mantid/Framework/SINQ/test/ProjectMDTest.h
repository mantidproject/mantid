#ifndef __PROJECTMDTEST
#define __PROJECTMDTEST

#include <cxxtest/TestSuite.h>
#include "MantidSINQ/ProjectMD.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid;
using namespace Mantid::MDEvents;


class ProjectMDTest: public CxxTest::TestSuite {
public:

  void testName() {
    ProjectMD loader;
    TS_ASSERT_EQUALS( loader.name(), "ProjectMD");
  }

  void testInit() {
    ProjectMD loader;
    TS_ASSERT_THROWS_NOTHING( loader.initialize());
    TS_ASSERT( loader.isInitialized());
  }

  void testProjectZ()
  {
    makeTestMD();
    
    ProjectMD pmd;
    pmd.initialize();
    pmd.setPropertyValue("InputWorkspace", "PMDTest");
    pmd.setPropertyValue("ProjectDirection", "Z");
    pmd.setPropertyValue("StartIndex", "0");
    pmd.setPropertyValue("EndIndex", "200");
    std::string outputSpace = "PMD_out";
    pmd.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING( pmd.execute());

    IMDHistoWorkspace_sptr data =  
      AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(
								    outputSpace);
    TS_ASSERT_EQUALS(2,data->getNumDims());
    long nBin = data->getNPoints();
    long sum = 0;
    double *sdata = data->getSignalArray();
    for(long i = 0; i < nBin; i++){
      sum += (long)sdata[i];
    }
    TS_ASSERT_EQUALS(sum,2400000);

    // test dimensions
    boost::shared_ptr<const IMDDimension> dimi = data->getDimension(0);
    TS_ASSERT_EQUALS(dimi->getNBins(),100);
    TS_ASSERT_DELTA(dimi->getMinimum(),-50.,.1);
    TS_ASSERT_DELTA(dimi->getMaximum(),50.,.1);

    dimi = data->getDimension(1);
    TS_ASSERT_EQUALS(dimi->getNBins(),120);
    TS_ASSERT_DELTA(dimi->getMinimum(),-60.,.1);
    TS_ASSERT_DELTA(dimi->getMaximum(),60.,.1);

    AnalysisDataService::Instance().clear();

  }

  void testProjectHalfZ()
  {
    makeTestMD();
    
    ProjectMD pmd;
    pmd.initialize();
    pmd.setPropertyValue("InputWorkspace", "PMDTest");
    pmd.setPropertyValue("ProjectDirection", "Z");
    pmd.setPropertyValue("StartIndex", "50");
    pmd.setPropertyValue("EndIndex", "150");
    std::string outputSpace = "PMD_out";
    pmd.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING( pmd.execute());

    IMDHistoWorkspace_sptr data =  
      AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(
								    outputSpace);
    TS_ASSERT_EQUALS(2,data->getNumDims());
    long nBin = data->getNPoints();
    long sum = 0;
    double *sdata = data->getSignalArray();
    for(long i = 0; i < nBin; i++){
      sum += (long)sdata[i];
    }
    TS_ASSERT_EQUALS(sum,1200000);

    // test dimensions
    boost::shared_ptr<const IMDDimension> dimi = data->getDimension(0);
    TS_ASSERT_EQUALS(dimi->getNBins(),100);
    TS_ASSERT_DELTA(dimi->getMinimum(),-50.,.1);
    TS_ASSERT_DELTA(dimi->getMaximum(),50.,.1);

    dimi = data->getDimension(1);
    TS_ASSERT_EQUALS(dimi->getNBins(),120);
    TS_ASSERT_DELTA(dimi->getMinimum(),-60.,.1);
    TS_ASSERT_DELTA(dimi->getMaximum(),60.,.1);

    AnalysisDataService::Instance().clear();

  }

  void testProjectX()
  {
    makeTestMD();
    
    ProjectMD pmd;
    pmd.initialize();
    pmd.setPropertyValue("InputWorkspace", "PMDTest");
    pmd.setPropertyValue("ProjectDirection", "X");
    pmd.setPropertyValue("StartIndex", "0");
    pmd.setPropertyValue("EndIndex", "100");
    std::string outputSpace = "PMD_out";
    pmd.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING( pmd.execute());

    IMDHistoWorkspace_sptr data =  
      AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(
								    outputSpace);
    TS_ASSERT_EQUALS(2,data->getNumDims());
    long nBin = data->getNPoints();
    long sum = 0;
    double *sdata = data->getSignalArray();
    for(long i = 0; i < nBin; i++){
      sum += (long)sdata[i];
    }
    TS_ASSERT_EQUALS(sum,2400000);

    // test dimensions
    boost::shared_ptr<const IMDDimension> dimi = data->getDimension(0);
    TS_ASSERT_EQUALS(dimi->getNBins(),120);
    TS_ASSERT_DELTA(dimi->getMinimum(),-60.,.1);
    TS_ASSERT_DELTA(dimi->getMaximum(),60.,.1);

    dimi = data->getDimension(1);
    TS_ASSERT_EQUALS(dimi->getNBins(),200);
    TS_ASSERT_DELTA(dimi->getMinimum(),-100.,.1);
    TS_ASSERT_DELTA(dimi->getMaximum(),100.,.1);

    AnalysisDataService::Instance().clear();

  }

  void testProjectY()
  {
    makeTestMD();
    
    ProjectMD pmd;
    pmd.initialize();
    pmd.setPropertyValue("InputWorkspace", "PMDTest");
    pmd.setPropertyValue("ProjectDirection", "Y");
    pmd.setPropertyValue("StartIndex", "0");
    pmd.setPropertyValue("EndIndex", "120");
    std::string outputSpace = "PMD_out";
    pmd.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING( pmd.execute());

    IMDHistoWorkspace_sptr data =  
      AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(
								    outputSpace);
    TS_ASSERT_EQUALS(2,data->getNumDims());
    long nBin = data->getNPoints();
    long sum = 0;
    double *sdata = data->getSignalArray();
    for(long i = 0; i < nBin; i++){
      sum += (long)sdata[i];
    }
    TS_ASSERT_EQUALS(sum,2400000);

    // test dimensions
    boost::shared_ptr<const IMDDimension> dimi = data->getDimension(0);
    TS_ASSERT_EQUALS(dimi->getNBins(),100);
    TS_ASSERT_DELTA(dimi->getMinimum(),-50.,.1);
    TS_ASSERT_DELTA(dimi->getMaximum(),50.,.1);

    dimi = data->getDimension(1);
    TS_ASSERT_EQUALS(dimi->getNBins(),200);
    TS_ASSERT_DELTA(dimi->getMinimum(),-100.,.1);
    TS_ASSERT_DELTA(dimi->getMaximum(),100.,.1);

    AnalysisDataService::Instance().clear();

  }

  void testMetaDataCopy()
  {
    makeTestMD();
    IMDHistoWorkspace_sptr data =  
      AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(
								    std::string("PMDTest"));
    data->setTitle("Hugo");
    if(data->getNumExperimentInfo() == 0){
    	data->addExperimentInfo((ExperimentInfo_sptr)new ExperimentInfo());
    }
    Run &rr = data->getExperimentInfo(0)->mutableRun();
    rr.addProperty("Gwendolin",27.8,true);
    
    ProjectMD pmd;
    pmd.initialize();
    pmd.setPropertyValue("InputWorkspace", "PMDTest");
    pmd.setPropertyValue("ProjectDirection", "Y");
    pmd.setPropertyValue("StartIndex", "0");
    pmd.setPropertyValue("EndIndex", "120");
    std::string outputSpace = "PMD_out";
    pmd.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING( pmd.execute());

    data =  
      AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(
								    outputSpace);
    std::string tst = data->getTitle();
    size_t found = tst.find("Hugo");
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
    AnalysisDataService::Instance().add("PMDTest",outWS);
    return outWS;
  }
};

#endif
