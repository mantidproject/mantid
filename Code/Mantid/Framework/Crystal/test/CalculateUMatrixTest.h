#ifndef MANTID_CRYSTAL_CALCULATEUMATRIXTEST_H_
#define MANTID_CRYSTAL_CALCULATEUMATRIXTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidCrystal/CalculateUMatrix.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Crystal;
using Mantid::Geometry::OrientedLattice;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
class CalculateUMatrixTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    CalculateUMatrix alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Name of the output workspace.
    std::string WSName("peaksCalculateUMatrix");
    generatePeaks(WSName);

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve(WSName) ) );
    TS_ASSERT(ws);
    if (!ws) return;
    CalculateUMatrix alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("a", "2.") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("b", "3.") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("c", "4.") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("alpha", "90") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("beta",  "90") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("gamma", "90") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Check that we set an oriented lattice
    TS_ASSERT(ws->mutableSample().hasOrientedLattice());
    // Check that the UB matrix is the same as in TOPAZ_3007.mat
    OrientedLattice latt=ws->mutableSample().getOrientedLattice();
    DblMatrix U(3,3,false);
    U[0][0]=sqrt(3.)*0.5;
    U[2][2]=sqrt(3.)*0.5;
    U[2][0]=0.5;
    U[0][2]=-0.5;
    U[1][1]=1.;


    TS_ASSERT(latt.getU().equals(U,1e-10));

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(WSName);
  }
  
  void test_fail()
  {
    Mantid::Geometry::Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
    inst->setName("SillyInstrument");
    PeaksWorkspace_sptr pw(new PeaksWorkspace);
    pw->setInstrument(inst);
    std::string val = "value";
    pw->mutableRun().addProperty("TestProp", val);
    Peak p(inst, 1, 3.0,V3D(1,0,0)); //HKL=1,0,0
    pw->addPeak(p);
    Peak p1(inst, 1, 3.0,V3D(2,0,0)); //HKL=2,0,0
    Peak p2(inst, 1, 3.0,V3D(2,2,0)); //HKL=2,2,0
    std::string WSName("peaks-fail");
    AnalysisDataService::Instance().addOrReplace(WSName,pw);
    //one peak
    CalculateUMatrix alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("a", "14.1526") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("b", "19.2903") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("c", "8.5813") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("alpha", "90") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("beta", "105.0738") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("gamma", "90") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( !alg.isExecuted() );
    AnalysisDataService::Instance().remove(WSName);
    //collinear peaks
    pw->addPeak(p1);
    AnalysisDataService::Instance().addOrReplace(WSName,pw);
    CalculateUMatrix alg1;
    TS_ASSERT_THROWS_NOTHING( alg1.initialize() )
    TS_ASSERT( alg1.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("a", "14.1526") );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("b", "19.2903") );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("c", "8.5813") );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("alpha", "90") );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("beta", "105.0738") );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("gamma", "90") );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("PeaksWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg1.execute(); );
    TS_ASSERT( !alg1.isExecuted() );
    AnalysisDataService::Instance().remove(WSName);
    // should be ok with non-colinear peaks
    pw->addPeak(p2);
    AnalysisDataService::Instance().addOrReplace(WSName,pw);
    CalculateUMatrix alg2;
    TS_ASSERT_THROWS_NOTHING( alg2.initialize() )
    TS_ASSERT( alg2.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg2.setPropertyValue("a", "14.1526") );
    TS_ASSERT_THROWS_NOTHING( alg2.setPropertyValue("b", "19.2903") );
    TS_ASSERT_THROWS_NOTHING( alg2.setPropertyValue("c", "8.5813") );
    TS_ASSERT_THROWS_NOTHING( alg2.setPropertyValue("alpha", "90") );
    TS_ASSERT_THROWS_NOTHING( alg2.setPropertyValue("beta", "105.0738") );
    TS_ASSERT_THROWS_NOTHING( alg2.setPropertyValue("gamma", "90") );
    TS_ASSERT_THROWS_NOTHING( alg2.setPropertyValue("PeaksWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg2.execute(); );
    TS_ASSERT( alg2.isExecuted() );
    AnalysisDataService::Instance().remove(WSName);
  }

private:
  DblMatrix UB;

  void setupUB()
  {
      OrientedLattice ol;
      DblMatrix U(3,3,false);
      U[0][0]=sqrt(3.)*0.5;
      U[2][2]=sqrt(3.)*0.5;
      U[2][0]=0.5;
      U[0][2]=-0.5;
      U[1][1]=1.;
      ol.set(2,3,4,90,90,90);
      ol.setU(U);
      UB=ol.getUB();
  }

  double QXUB(double H, double K, double L)
  {
     return (UB*V3D(H,K,L))[0]*2.*M_PI;
  }

  double QYUB(double H, double K, double L)
  {
     return (UB*V3D(H,K,L))[1]*2.*M_PI;
  }

  double QZUB(double H, double K, double L)
  {
     return (UB*V3D(H,K,L))[2]*2.*M_PI;
  }

  double lam(double H,double K,double L)
  {
      return 2.*QZUB(H,K,L)/(QXUB(H,K,L)*QXUB(H,K,L)+QYUB(H,K,L)*QYUB(H,K,L)+QZUB(H,K,L)*QZUB(H,K,L))*2.*M_PI;
  }

  double th(double H,double K,double L)
  {
      return acos(1.-QZUB(H,K,L)*lam(H,K,L)/2./M_PI);
  }

  double ph(double H,double K,double L)
  {
      return atan2(-QYUB(H,K,L),-QXUB(H,K,L));
  }

  void generatePeaks(std::string WSName)
  {
      setupUB();

      double Hpeaks[9]={0,1,1,0,-1,-1,1,-3,-2};
      double Kpeaks[9]={3,0,4,0,2,0,2,3,1};
      double Lpeaks[9]={3,5,5,2,3,2,4,5,3};

      std::vector<double> lambda(9),theta(9),phi(9),L2(9,1.);
      for(int i=0;i<=8;i++)
      {
          lambda.at(i)=lam(Hpeaks[i],Kpeaks[i],Lpeaks[i]);
          theta.at(i)=th(Hpeaks[i],Kpeaks[i],Lpeaks[i]);
          phi.at(i)=ph(Hpeaks[i],Kpeaks[i],Lpeaks[i]);
      }

      auto inst = ComponentCreationHelper::createCylInstrumentWithDetInGivenPositions(L2,theta, phi);
      inst->setName("SillyInstrument");
      auto pw = PeaksWorkspace_sptr(new PeaksWorkspace);
      pw->setInstrument(inst);
      for(int i=0;i<=8;i++)
      {
        Peak p(inst, i+1, lambda[i],V3D(Hpeaks[i],Kpeaks[i],Lpeaks[i]));
        pw->addPeak(p);
      }
      AnalysisDataService::Instance().addOrReplace(WSName,pw);
  }

};


#endif /* MANTID_CRYSTAL_CALCULATEUMATRIXTEST_H_ */

