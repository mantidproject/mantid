#ifndef __MDHISTOTOWORKSPACE2DTEST
#define __MDHISTOTOWORKSPACE2DTEST

#include <cxxtest/TestSuite.h>
#include "MantidSINQ/MDHistoToWorkspace2D.h"
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

class MDHistoToWorkspace2DTest: public CxxTest::TestSuite {
public:

  void testName() {
    MDHistoToWorkspace2D loader;
    TS_ASSERT_EQUALS( loader.name(), "MDHistoToWorkspace2D");
  }

  void testInit() {
    MDHistoToWorkspace2D loader;
    TS_ASSERT_THROWS_NOTHING( loader.initialize());
    TS_ASSERT( loader.isInitialized());
  }

  void testExec()
  {
    makeTestMD();
    
    MDHistoToWorkspace2D pmd;
    pmd.initialize();
    pmd.setPropertyValue("InputWorkspace", "PMDTest");
    std::string outputSpace = "PMD_out";
    pmd.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING( pmd.execute());

    // test data 
    MatrixWorkspace_sptr data =  
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
								  outputSpace);
    TS_ASSERT_EQUALS(data->getNumberHistograms(),12000);
    MantidVec& X = data->dataX(0);
    MantidVec& Y = data->dataY(0);
    double dSum = .0;
    for(size_t i = 0; i < Y.size(); i++){
      dSum += Y[i];
    } 
    TS_ASSERT_EQUALS(dSum,200);

    // test X
    TS_ASSERT_EQUALS(X.size(),200);
    TS_ASSERT_DELTA(X[0],-100,.1);
    TS_ASSERT_DELTA(X[X.size()-1],99,.1);

    std::string tst = data->getTitle();
    size_t found = tst.find("Hugo");
    TS_ASSERT_DIFFERS(found,std::string::npos);

    const Run r = data->run();
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
