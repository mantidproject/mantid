#ifndef MANTID_MD_CONVERT2_QxyzDE_TEST_H_
#define MANTID_MD_CONVERT2_QxyzDE_TEST_H_

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDAlgorithms/ConvertToMD.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidMDEvents/MDWSDescription.h"
#include "MantidAPI/FrameworkManager.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Kernel;
using Mantid::Geometry::OrientedLattice;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDEvents;

class ConvertTo3DdETestHelper: public ConvertToMD
{
public:
    ConvertTo3DdETestHelper(){};
};

// Test is transformed from ConvetToQ3DdE but actually tests some aspects of ConvertToMD algorithm. 
class ConvertToQ3DdETest : public CxxTest::TestSuite
{
 std::auto_ptr<ConvertTo3DdETestHelper> pAlg;
public:
static ConvertToQ3DdETest *createSuite() { return new ConvertToQ3DdETest(); }
static void destroySuite(ConvertToQ3DdETest * suite) { delete suite; }    

void testInit(){
  
    TS_ASSERT( pAlg->isInitialized() )
 
}

void testExecThrow(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::CreateGroupedWorkspace2DWithRingsAndBoxes();

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS(" the workspace X axis does not have units ",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()),std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));

}

/** Calculate min-max value defaults*/
Mantid::API::IAlgorithm * calcMinMaxValDefaults(const std::string &QMode,const std::string &QFrame,std::string OtherProperties=std::string(""))
{

  Mantid::API::IAlgorithm *childAlg = Mantid::API::FrameworkManager::Instance().createAlgorithm("ConvertToMDMinMaxLocal");
  if(!childAlg)
    {
      TSM_ASSERT("Can not create child ChildAlgorithm to found min/max values",false);
      return NULL;
    }
    childAlg->initialize();
    if(!childAlg->isInitialized())
    {
      TSM_ASSERT("Can not initialize child ChildAlgorithm to found min/max values",false);
      return NULL;
    }
    childAlg->setPropertyValue("InputWorkspace", "testWSProcessed");
    childAlg->setPropertyValue("QDimensions",QMode);
    childAlg->setPropertyValue("dEAnalysisMode","Direct");
    childAlg->setPropertyValue("Q3DFrames",QFrame);
    childAlg->setPropertyValue("OtherDimensions",OtherProperties);
 
    childAlg->execute();
    if(!childAlg->isExecuted() )
    {
      TSM_ASSERT("Can not execute child ChildAlgorithm to found min/max values",false);
      return NULL;
    }
    return childAlg;

}


void testExecRunsOnNewWorkspaceNoLimits()
{
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);
    // add workspace energy
     ws2D->mutableRun().addProperty("Ei",12.,"meV",true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);
    // clear stuff from analysis data service for test to work in specified way
    AnalysisDataService::Instance().remove("EnergyTransfer4DWS");

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions","Q3D") );
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Direct"));
  

    pAlg->execute();
    if(!pAlg->isExecuted())
    {
      TSM_ASSERT("have not executed convertToMD without min-max limits specied ",false);
      return;
    }

    auto childAlg = calcMinMaxValDefaults("Q3D","HKL");
    if (!childAlg) return;
    // get the results
    std::vector<double> minVal = childAlg->getProperty("MinValues");
    std::vector<double> maxVal = childAlg->getProperty("MaxValues");
    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>("EnergyTransfer4DWS");

    size_t NDims = outWS->getNumDims();
    for(size_t i=0;i<NDims;i++)
    {
        const Geometry::IMDDimension *pDim = outWS->getDimension(i).get();
        TS_ASSERT_DELTA(minVal[i],pDim->getMinimum(),1.e-4);
        TS_ASSERT_DELTA(maxVal[i],pDim->getMaximum(),1.e-4);
    }
}

void testExecRunsOnNewWorkspaceNoLimits5D()
{
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);
    // add workspace energy
     ws2D->mutableRun().addProperty("Ei",12.,"meV",true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);
    // clear stuff from analysis data service for test to work in specified way
    AnalysisDataService::Instance().remove("EnergyTransfer4DWS");

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer5DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions","Q3D") );
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OtherDimensions","Ei") );
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Direct"));
  

    pAlg->execute();
    if(!pAlg->isExecuted())
    {
      TSM_ASSERT("have not executed convertToMD without min-max limits specied ",false);
      return;
    }

    auto childAlg = calcMinMaxValDefaults("Q3D","HKL",std::string("Ei"));
    if (!childAlg) return;
    // get the results
    std::vector<double> minVal = childAlg->getProperty("MinValues");
    std::vector<double> maxVal = childAlg->getProperty("MaxValues");
    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>("EnergyTransfer5DWS");

    size_t NDims = outWS->getNumDims();
    for(size_t i=0;i<NDims-1;i++)
    {
        const Geometry::IMDDimension *pDim = outWS->getDimension(i).get();
        TS_ASSERT_DELTA(minVal[i],pDim->getMinimum(),1.e-4);
        TS_ASSERT_DELTA(maxVal[i],pDim->getMaximum(),1.e-4);
    }
    size_t nun5D=4;
    const Geometry::IMDDimension *pDim = outWS->getDimension(nun5D).get();
    TS_ASSERT_DELTA(minVal[nun5D]*0.9,pDim->getMinimum(),1.e-4);
    TS_ASSERT_DELTA(maxVal[nun5D]*1.1,pDim->getMaximum(),1.e-4);




}


void testExecWorksAutoLimitsOnNewWorkspaceNoMinMaxLimits()
{
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);
 // add workspace energy
    ws2D->mutableRun().addProperty("Ei",12.,"meV",true);


    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);
    // clear stuff from analysis data service for test to work in specified way
    AnalysisDataService::Instance().remove("EnergyTransfer4DWS");


    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions","Q3D"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Direct"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OtherDimensions", ""));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
 
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MaxValues", ""));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MinValues", ""));

//  pAlg->setRethrows(true);
    pAlg->execute();
    if(!pAlg->isExecuted())
    {
      TSM_ASSERT("have not executed convertToMD with only min limits specied ",false);
      return;
    }


    auto childAlg = calcMinMaxValDefaults("Q3D","HKL");
    if (!childAlg) return;

    // get the results
    std::vector<double> minVal = childAlg->getProperty("MinValues");
    std::vector<double> maxVal = childAlg->getProperty("MaxValues");

    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>("EnergyTransfer4DWS");

    size_t NDims = outWS->getNumDims();
    for(size_t i=0;i<NDims;i++)
    {
        const Geometry::IMDDimension *pDim = outWS->getDimension(i).get();
        TS_ASSERT_DELTA(minVal[i],pDim->getMinimum(),1.e-4);
        TS_ASSERT_DELTA(maxVal[i],pDim->getMaximum(),1.e-4);
    }




}
void testExecFine(){
    // create model processed workpsace with 10x10 cylindrical detectors, 10 energy levels and oriented lattice
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);
 // add workspace energy
     ws2D->mutableRun().addProperty("Ei",12.,"meV",true);


    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions","Q3D"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Direct"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
 
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MinValues", "-50.,-50.,-50,-2"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MaxValues", " 50., 50., 50, 20"));

    pAlg->execute();
    TSM_ASSERT("Should be successful ",pAlg->isExecuted());

}
void testExecAndAdd(){
    // create model processed workpsace with 10x10 cylindrical detectors, 10 energy levels and oriented lattice
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);

  // rotate the crystal by twenty degrees back;
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,20);
     // add workspace energy
     ws2D->mutableRun().addProperty("Ei",13.,"meV",true);
 //  

     AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions","Q3D"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Indirect"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
 
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MinValues", "-50.,-50.,-50,-2"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MaxValues", " 50., 50., 50, 20"));


    pAlg->execute();
    TSM_ASSERT("Should succseed as adding should work fine ",pAlg->isExecuted());

 

}


// COMPARISON WITH HORACE:  --->    DISABLED
void xtestTransfMat1()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(16,10,true);
     OrientedLattice * latt = new OrientedLattice(1,2,3, 90., 90., 90.);
     ws2D->mutableSample().setOrientedLattice(latt);
     delete latt;
     MDWSDescription TWS(4);


     std::vector<double> rot;
//    std::vector<double> rot=pAlg->get_transf_matrix(ws2D,Kernel::V3D(1,0,0),Kernel::V3D(0,1,0));
     Kernel::Matrix<double> unit = Kernel::Matrix<double>(3,3, true);
     Kernel::Matrix<double> rez(rot);
     TS_ASSERT(unit.equals(rez,1.e-4));
}
void xtestTransfMat2()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(16,10,true);
     OrientedLattice * latt = new OrientedLattice(1,2,3, 75., 45., 35.);
     ws2D->mutableSample().setOrientedLattice(latt);
     delete latt;
      std::vector<double> rot;
    //std::vector<double> rot=pAlg->get_transf_matrix(ws2D,Kernel::V3D(1,0,0),Kernel::V3D(0,1,0));
     Kernel::Matrix<double> unit = Kernel::Matrix<double>(3,3, true);
     Kernel::Matrix<double> rez(rot);
     TS_ASSERT(unit.equals(rez,1.e-4));
}
void xtestTransfMat3()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(16,10,true);
     OrientedLattice * latt = new OrientedLattice(1,2,3, 75., 45., 35.);
     ws2D->mutableSample().setOrientedLattice(latt);
     delete latt;
      std::vector<double> rot;
     //std::vector<double> rot=pAlg->get_transf_matrix(ws2D,Kernel::V3D(1,0,0),Kernel::V3D(0,-1,0));
     Kernel::Matrix<double> unit = Kernel::Matrix<double>(3,3, true);
     unit[1][1]=-1;
     unit[2][2]=-1;
     Kernel::Matrix<double> rez(rot);
     TS_ASSERT(unit.equals(rez,1.e-4));
}
void xtestTransfMat4()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(16,10,true);
     OrientedLattice * latt = new OrientedLattice(1,1,3, 90., 90., 90.);
     ws2D->mutableSample().setOrientedLattice(latt);
     delete latt;
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,0);
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(1,0);
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(2,0);

        std::vector<double> rot;
    //std::vector<double> rot=pAlg->get_transf_matrix(ws2D,Kernel::V3D(1,1,0),Kernel::V3D(1,-1,0));
     Kernel::Matrix<double> sample = Kernel::Matrix<double>(3,3, true);
     sample[0][0]= sqrt(2.)/2 ;
     sample[0][1]= sqrt(2.)/2 ;
     sample[1][0]= sqrt(2.)/2 ;
     sample[1][1]=-sqrt(2.)/2 ;
     sample[2][2]= -1 ;
     Kernel::Matrix<double> rez(rot);
     TS_ASSERT(sample.equals(rez,1.e-4));
}
void xtestTransfMat5()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(16,10,true);
     OrientedLattice * latt = new OrientedLattice(1,2,3, 75., 45., 90.);
     ws2D->mutableSample().setOrientedLattice(latt);
     delete latt;
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,0);
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(1,0);
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(2,0);

       std::vector<double> rot;
     //std::vector<double> rot=pAlg->get_transf_matrix(ws2D,Kernel::V3D(1,1,0),Kernel::V3D(1,-1,0));
     Kernel::Matrix<double> sample = Kernel::Matrix<double>(3,3, true);
     //aa=[0.9521 0.3058  0.0000;  0.3058   -0.9521    0.0000;   0         0   -1.000];
     sample[0][0]= 0.9521 ;
     sample[0][1]= 0.3058 ;
     sample[1][0]= 0.3058 ;
     sample[1][1]=-0.9521 ;
     sample[2][2]= -1 ;
     Kernel::Matrix<double> rez(rot);
     TS_ASSERT(sample.equals(rez,1.e-4));
}
void xtestTransf_PSI_DPSI()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(16,10,true);
     OrientedLattice * latt = new OrientedLattice(1,1,1, 90., 90., 90.);
     ws2D->mutableSample().setOrientedLattice(latt);
     delete latt;
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,0);
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(1,-20); // Psi, dPsi
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(2,0);

        std::vector<double> rot;
    //std::vector<double> rot=pAlg->get_transf_matrix(ws2D,Kernel::V3D(1,0,0),Kernel::V3D(0,1,0));
     Kernel::Matrix<double> sample = Kernel::Matrix<double>(3,3, true);   
     sample[0][0]= 0.9397 ;
     sample[0][1]= 0.3420 ;
     sample[1][0]=-0.3420 ;
     sample[1][1]= 0.9397 ;
     sample[2][2]=  1 ;
     Kernel::Matrix<double> rez(rot);
     TS_ASSERT(sample.equals(rez,1.e-4));
}
void xtestTransf_GL()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(16,10,true);
     OrientedLattice * latt = new OrientedLattice(1,1,1, 90., 90., 90.);
     ws2D->mutableSample().setOrientedLattice(latt);
     delete latt;
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,20);  //gl
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(1,0);
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(2,0);

        std::vector<double> rot;
   // std::vector<double> rot=pAlg->get_transf_matrix(ws2D,Kernel::V3D(1,0,0),Kernel::V3D(0,1,0));
     Kernel::Matrix<double> sample = Kernel::Matrix<double>(3,3, true);
     
     sample[0][0]= 0.9397 ;
     sample[0][2]= 0.3420 ;
     sample[2][0]=-0.3420 ;
     sample[2][2]= 0.9397 ;
     sample[1][1]=  1 ;
     Kernel::Matrix<double> rez(rot);
     TS_ASSERT(sample.equals(rez,1.e-4));
}
// check the results;
void t__tResult(){
     std::vector<double> L2(3,10),polar(3,0),azim(3,0);
     polar[1]=1;
     polar[2]=2;
     azim[0]=-1;
     azim[2]= 1;

     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedInelasticWS(L2,polar,azim,3,-1,2,10);

     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,0);  //gl
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(1,0);
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(2,0);
  
     AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("EnergyInput", "12."));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QdEValuesMin", "-10.,-10.,-10,-2"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QdEValuesMax", " 10., 10., 10, 8"))
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("PreprocDetectorsWS",""));

    pAlg->execute();
    TSM_ASSERT("Should be successful ",pAlg->isExecuted());

    Mantid::API::Workspace_sptr wsOut =  AnalysisDataService::Instance().retrieve("EnergyTransfer4DWS");
    TSM_ASSERT("Can not retrieve resulting workspace from the analysis data service ",wsOut);
}


// COMPARISON WITH HORACE: END  <---
//TODO:  this check has to be implemented !!!!
void t__tWithExistingLatticeTrowsLowEnergy(){
    // create model processed workpsace with 10x10 cylindrical detectors, 10 energy levels and oriented lattice
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);
    // add workspace energy
     ws2D->mutableRun().addProperty("Ei",2.,"meV",true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions","Q3D"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Inelastic"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MinValues", "-50.,-50.,-50,-2"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MaxValues", " 50., 50.,-50,10"));



    pAlg->execute();
    TSM_ASSERT("Should be not-successful as input energy was lower then obtained",!pAlg->isExecuted());

}



ConvertToQ3DdETest()
{
    pAlg = std::auto_ptr<ConvertTo3DdETestHelper>(new ConvertTo3DdETestHelper());
    pAlg->initialize();
    // initialize (load)Matid algorithm framework -- needed to run this test separately
    Mantid::API::IAlgorithm *childAlg = Mantid::API::FrameworkManager::Instance().createAlgorithm("ConvertUnits");
    TSM_ASSERT("Can not initialize Mantid algorithm framework",childAlg);
    if(!childAlg)
    {
      throw(std::runtime_error("Can not initalize/Load MantidAlgorithm dll"));
    }

}

};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

