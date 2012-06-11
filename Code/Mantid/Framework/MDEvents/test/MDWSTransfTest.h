#ifndef MANTID_MDWS_SLICE_H_
#define MANTID_MDWS_SLICE_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDEvents/MDTransfAxisNames.h"
#include "MantidMDEvents/MDWSDescription.h"
#include "MantidMDEvents/MDWSTransform.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::Kernel;




class MDWSTransfTest : public CxxTest::TestSuite
{
    Mantid::API::MatrixWorkspace_sptr ws2D;
    Geometry::OrientedLattice *pLattice;
    //MDWSSliceTest slice;
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDWSTransfTest *createSuite() { return new MDWSTransfTest(); }
  static void destroySuite( MDWSTransfTest *suite ) { delete suite; }


void test_buildDimNames(){

    MDEvents::MDWSDescription TargWSDescription;
    std::vector<double> minVal(4,-3),maxVal(4,3);
    TargWSDescription.setMinMax(minVal,maxVal);

    TargWSDescription.buildFromMatrixWS(ws2D,"Q3D","Direct");

   MDWSTransform MsliceTransf;
   TS_ASSERT_THROWS_NOTHING(MsliceTransf.setQ3DDimensionsNames(TargWSDescription,ConvertToMD::NoScaling));

   std::vector<std::string> dimNames = TargWSDescription.getDimNames();
   TS_ASSERT_EQUALS("[H,0,0]",dimNames[0]);
   TS_ASSERT_EQUALS("[0,K,0]",dimNames[1]);
   TS_ASSERT_EQUALS("[0,0,L]",dimNames[2]);
   TS_ASSERT_EQUALS("DeltaE",dimNames[3]);
    

}

void testCoplanarProjections()
{
    std::vector<double> u(3,0);
    std::vector<double> v(3,0);
    std::vector<double> w(3,0);
    u[0]=1;
    v[2]=1;
    w[2]=-1;
    MDWSTransform MsliceTransf;
    TS_ASSERT_THROWS_ANYTHING(MsliceTransf.setUVvectors(u,v,w));
}

void testTransfMat1()
{
     std::vector<std::string> dimNames;
     MDEvents::MDWSDescription TWS;
     std::vector<double> minVal(4,-3),maxVal(4,3);
     TWS.setMinMax(minVal,maxVal);

     if(pLattice)delete pLattice;
     pLattice = new Geometry::OrientedLattice(5*M_PI,M_PI,2*M_PI, 90., 90., 90.);
     ws2D->mutableSample().setOrientedLattice(pLattice);
     TWS.buildFromMatrixWS(ws2D,"Q3D","Direct");

     std::vector<double> u(3,0);
     std::vector<double> v(3,0);
     std::vector<double> w(3,0);
     u[0]=1;
     v[2]=1;
     w[1]=-1;
     std::vector<double> rot;

     MDWSTransform MsliceTransf;
     MsliceTransf.setUVvectors(u,v,w);


      TS_ASSERT_THROWS_NOTHING(rot=MsliceTransf.getTransfMatrix(TWS,ConvertToMD::HKLScale));
      TS_ASSERT_THROWS_NOTHING(MsliceTransf.setQ3DDimensionsNames(TWS,ConvertToMD::HKLScale));

      dimNames = TWS.getDimNames();
      TS_ASSERT_EQUALS("[H,0,0]",dimNames[0]);
      TS_ASSERT_EQUALS("[0,0,L]",dimNames[1]);
      TS_ASSERT_EQUALS("[0,-K,0]",dimNames[2]);
      TS_ASSERT_EQUALS("DeltaE",dimNames[3]);


      std::vector<double> rot1;
      TS_ASSERT_THROWS_NOTHING(rot1=MsliceTransf.getTransfMatrix(TWS,ConvertToMD::OrthogonalHKLScale));
      TS_ASSERT_THROWS_NOTHING(MsliceTransf.setQ3DDimensionsNames(TWS,ConvertToMD::OrthogonalHKLScale));

      dimNames = TWS.getDimNames();
      TS_ASSERT_EQUALS("[H,0,0]",dimNames[0]);
      TS_ASSERT_EQUALS("[0,0,L]",dimNames[1]);
      TS_ASSERT_EQUALS("[0,-K,0]",dimNames[2]);
      TS_ASSERT_EQUALS("DeltaE",dimNames[3]);

      //TODO ->>> possible problem with orthogonal hkl == this dest does not holds
      //TSM_ASSERT_DELTA(" element 1 should be a/2Pi",pLattice->a1()/(2*M_PI),rot[0],1.e-6);
      //TSM_ASSERT_DELTA(" element 2 should be -b/2Pi",-pLattice->a2()/(2*M_PI),rot[7],1.e-6);
      //TSM_ASSERT_DELTA(" element 3 should be c/2Pi",pLattice->a3()/(2*M_PI),rot[5],1.e-6);

      for(int i=0;i<9;i++){
          TSM_ASSERT_DELTA(" element: "+boost::lexical_cast<std::string>(i)+" wrong",rot[i],rot1[i],1.e-6);
      }
}


MDWSTransfTest()
{
     ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    // rotate the crystal by twenty degrees back;
     ws2D->mutableRun().getGoniometer().setRotationAngle(0,20);
     // add workspace energy
     ws2D->mutableRun().addProperty("Ei",13.,"meV",true);

     pLattice = new Geometry::OrientedLattice(3,3,2,90,90,90);
     ws2D->mutableSample().setOrientedLattice(pLattice);

}


};
#endif
