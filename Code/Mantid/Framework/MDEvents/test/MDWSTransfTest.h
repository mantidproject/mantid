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

class MDWSTransformTestHelper: public MDWSTransform
{
  public:
      std::vector<double> getTransfMatrix(MDEvents::MDWSDescription &TargWSDescription,CnvrtToMD::TargetFrame frames,CnvrtToMD::CoordScaling scaling)const
   {
       CnvrtToMD::CoordScaling inScaling(scaling);
       return MDWSTransform::getTransfMatrix(TargWSDescription,frames,inScaling);
   }
   CnvrtToMD::TargetFrame findTargetFrame(MDEvents::MDWSDescription &TargWSDescription)const
   {
     return MDWSTransform::findTargetFrame(TargWSDescription);
   }

};


class MDWSTransfTest : public CxxTest::TestSuite
{
    Mantid::API::MatrixWorkspace_sptr ws2D;
    Geometry::OrientedLattice *pLattice;
  // this is permutation matrix which transforms Mantid coordinate system (beam along Z-axis)
   // to Horace coordinate system (beam along X-axis);
  //  Horace transformation matrix and Mantid Transformation matrix are connected by ration T_mantid  = T_hor*PermMH;
   Kernel::Matrix<double> PermMH,PermHM;

  

    //MDWSSliceTest slice;
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDWSTransfTest *createSuite() { return new MDWSTransfTest(); }
  static void destroySuite( MDWSTransfTest *suite ) { delete suite; }


void testFindTargetFrame()
{
   MDEvents::MDWSDescription TargWSDescription;
   Mantid::API::MatrixWorkspace_sptr spws =WorkspaceCreationHelper::Create2DWorkspaceBinned(10,10);
   //Mantid::API::MatrixWorkspace_sptr spws =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
   std::vector<double> minVal(4,-3),maxVal(4,3);
   TargWSDescription.setMinMax(minVal,maxVal);

   TargWSDescription.buildFromMatrixWS(spws,"Q3D","Direct");

   MDWSTransformTestHelper Transf;
   TS_ASSERT_EQUALS(CnvrtToMD::LabFrame,Transf.findTargetFrame(TargWSDescription));

   WorkspaceCreationHelper::SetGoniometer(spws,0,0,0);
   //spws->mutableRun().mutableGoniometer().setRotationAngle(0,20);

   TS_ASSERT_EQUALS(CnvrtToMD::SampleFrame,Transf.findTargetFrame(TargWSDescription));

   spws->mutableSample().setOrientedLattice(pLattice);
   TS_ASSERT_EQUALS(CnvrtToMD::HKLFrame,Transf.findTargetFrame(TargWSDescription));

}
void testForceTargetFrame()
{
   MDEvents::MDWSDescription TargWSDescription;
   
   Mantid::API::MatrixWorkspace_sptr spws =WorkspaceCreationHelper::Create2DWorkspaceBinned(10,10);
   std::vector<double> minVal(4,-3),maxVal(4,3);
   TargWSDescription.setMinMax(minVal,maxVal);
   spws->mutableSample().setOrientedLattice(NULL); 

   TargWSDescription.buildFromMatrixWS(spws,"Q3D","Direct");

   MDWSTransformTestHelper Transf;
   TSM_ASSERT_THROWS("Forced HKL frame would not accept workspace without oriented lattice",Transf.getTransfMatrix(TargWSDescription,CnvrtToMD::HKLFrame,CnvrtToMD::HKLScale),std::invalid_argument);
   TSM_ASSERT_THROWS("Forced SampleFrame frame would not accept workspace without goniometer defined",Transf.getTransfMatrix(TargWSDescription,CnvrtToMD::SampleFrame,CnvrtToMD::HKLScale),std::invalid_argument);
   spws->mutableSample().setOrientedLattice(pLattice);
   
   WorkspaceCreationHelper::SetGoniometer(spws,20,0,0);

   //spws->mutableRun().mutableGoniometer().setRotationAngle(0,20);

   std::vector<double> transf;
   TS_ASSERT_THROWS_NOTHING(transf=Transf.getTransfMatrix(TargWSDescription,CnvrtToMD::SampleFrame,CnvrtToMD::HKLScale));


}



void test_buildDimNames(){

    MDEvents::MDWSDescription TargWSDescription;
    std::vector<double> minVal(4,-3),maxVal(4,3);
    TargWSDescription.setMinMax(minVal,maxVal);

    TargWSDescription.buildFromMatrixWS(ws2D,"Q3D","Direct");

   MDWSTransform MsliceTransf;
   TS_ASSERT_THROWS_NOTHING(MsliceTransf.setQ3DDimensionsNames(TargWSDescription,CnvrtToMD::HKLFrame,CnvrtToMD::NoScaling));

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

     MDWSTransformTestHelper MsliceTransf;
     MsliceTransf.setUVvectors(u,v,w);

     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,0);
     CnvrtToMD::CoordScaling scales = CnvrtToMD::HKLScale;
     TS_ASSERT_THROWS_NOTHING(rot=MsliceTransf.getTransfMatrix(TWS,CnvrtToMD::AutoSelect,scales));
     TS_ASSERT_THROWS_NOTHING(MsliceTransf.setQ3DDimensionsNames(TWS,CnvrtToMD::HKLFrame,CnvrtToMD::HKLScale));

      dimNames = TWS.getDimNames();
      TS_ASSERT_EQUALS("[H,0,0]",dimNames[0]);
      TS_ASSERT_EQUALS("[0,0,L]",dimNames[1]);
      TS_ASSERT_EQUALS("[0,-K,0]",dimNames[2]);
      TS_ASSERT_EQUALS("DeltaE",dimNames[3]);


      std::vector<double> rot1;
      scales = CnvrtToMD::OrthogonalHKLScale;
      TS_ASSERT_THROWS_NOTHING(rot1=MsliceTransf.getTransfMatrix(TWS,CnvrtToMD::AutoSelect,scales));
      TS_ASSERT_THROWS_NOTHING(MsliceTransf.setQ3DDimensionsNames(TWS,CnvrtToMD::AutoSelect,scales));

      dimNames = TWS.getDimNames();
      TS_ASSERT_EQUALS("[H,0,0]",dimNames[0]);
      TS_ASSERT_EQUALS("[0,0,L]",dimNames[1]);
      TS_ASSERT_EQUALS("[0,-K,0]",dimNames[2]);
      TS_ASSERT_EQUALS("DeltaE",dimNames[3]);

    
      TSM_ASSERT_DELTA(" element 1 should be a/2Pi",pLattice->a1()/(2*M_PI),rot[0],1.e-6);
      TSM_ASSERT_DELTA(" element 2 should be -b/2Pi",-pLattice->a2()/(2*M_PI),rot[7],1.e-6);
      TSM_ASSERT_DELTA(" element 3 should be c/2Pi",pLattice->a3()/(2*M_PI),rot[5],1.e-6);

      // Orthogonal HKL and HKL are equivalent for rectilinear lattice
      for(int i=0;i<9;i++){
          TSM_ASSERT_DELTA(" element: "+boost::lexical_cast<std::string>(i)+" wrong",rot[i],rot1[i],1.e-6);
      }
      // Orthogonal HKL and HKL are equivalent for rectilinear lattice for any goniometer position
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,60);
     scales = CnvrtToMD::HKLScale;
     TS_ASSERT_THROWS_NOTHING(rot=MsliceTransf.getTransfMatrix(TWS,CnvrtToMD::AutoSelect,scales));
     scales = CnvrtToMD::OrthogonalHKLScale;
     TS_ASSERT_THROWS_NOTHING(rot1=MsliceTransf.getTransfMatrix(TWS,CnvrtToMD::AutoSelect,scales));
     for(int i=0;i<9;i++)
     {
          TSM_ASSERT_DELTA(" element: "+boost::lexical_cast<std::string>(i)+" wrong",rot[i],rot1[i],1.e-6);
     }


}

void testTransf2HoraceQinA()
{
     MDEvents::MDWSDescription TWS;
     std::vector<double> minVal(4,-3),maxVal(4,3);
     TWS.setMinMax(minVal,maxVal);
     Geometry::OrientedLattice latt(5*M_PI,M_PI,2*M_PI, 90., 90., 90.);

     ws2D->mutableSample().setOrientedLattice(&latt);
     TWS.buildFromMatrixWS(ws2D,"Q3D","Direct");

     std::vector<double> u(3,0);
     std::vector<double> v(3,0);
     std::vector<double> w(3,0);
     u[0]=1;
     v[1]=1;
     w[2]=1;
     std::vector<double> rot;

     MDWSTransformTestHelper MsliceTransf;
     MsliceTransf.setUVvectors(u,v,w);


     CnvrtToMD::CoordScaling scales = CnvrtToMD::NoScaling;
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,20);
     TS_ASSERT_THROWS_NOTHING(rot=MsliceTransf.getTransfMatrix(TWS,CnvrtToMD::AutoSelect,scales));

    // and this is Horace transformation matrix obtained from   [transf,u_to_rlu]=calc_proj_matrix(alat, angldeg, u, v, 20*deg2rad, omega, dpsi, gl, gs) (private folder in sqw)
    //    0.9397    0.3420      0
    //   -0.3420    0.9397      0
    //      0         0       1.0000   
     Kernel::Matrix<double> Transf(3,3);  // 20 deg rotation
     Transf[0][0] = 0.9397;      Transf[0][1] = 0.3420; Transf[0][2] = 0.;
     Transf[1][0] =-0.3420;      Transf[1][1] = 0.9397; Transf[1][2] = 0.;
     Transf[2][0] = 0.;          Transf[2][1] = 0.;     Transf[2][2] = 1;

     std::vector<double> sample = (PermHM*Transf*PermMH).getVector();
     for(size_t i=0;i<9;i++)
     {
       TS_ASSERT_DELTA(sample[i],rot[i],1.e-4);
     }
     // 40 degree rotation: [transf,u_to_rlu]=calc_proj_matrix(alat, angldeg, u, v, 40*deg2rad, omega, dpsi, gl, gs)
     Transf[0][0] = 0.7660;      Transf[0][1] = 0.6428; Transf[0][2] = 0.;
     Transf[1][0] =-0.6428;      Transf[1][1] = 0.7660; Transf[1][2] = 0.;
     Transf[2][0] = 0.;          Transf[2][1] = 0.;     Transf[2][2] = 1;

     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,40);
     TS_ASSERT_THROWS_NOTHING(rot=MsliceTransf.getTransfMatrix(TWS,CnvrtToMD::AutoSelect,scales));
     sample = (PermHM*Transf*PermMH).getVector();
     for(size_t i=0;i<9;i++)
     {
       TS_ASSERT_DELTA(sample[i],rot[i],1.e-4);
     }
     

//     // crystal misalighned -- this test does not fully holds
//     Mantid::Kernel::Matrix<double> Uhor(3,3,true);
////    Horace Umatrix build on the coordinate system, constructed aroung two vectors u=[0.9,0.1,0] and v=[0.1,0.9,0.1]
//     // 0.9939   0.1104  0.00
//     //-0.1097   0.9876  0.1125
//    //  0.0124  -0.1118  0.9937
//     Uhor[0][0]= 0.9938837;  Uhor[0][1]= 0.11043152607;  Uhor[0][2]= 0.;
//     Uhor[1][0]=-0.1097308;  Uhor[1][1]= 0.9875772045;   Uhor[1][2]= 0.1124740705;
//     Uhor[2][0]= 0.01242068; Uhor[2][1]=-0.111786149;    Uhor[2][2]= 0.99365466;
//
//     auto ol = new Geometry::OrientedLattice(2.73,2.73,2.73, 90., 90., 90.);
//     ol->setUFromVectors(Kernel::V3D(0.9,0.1,0),Kernel::V3D(0.1,0.9,0.1));
//     auto Uman = ol->getU();
//     auto U0vec = Uman.getVector();
//     // this gives us Umat in the form
//     Uman[1][0]==-0.1097308;  Uman[1][1]== 0.9875772045;   Uman[1][2]== 0.1124740705; // -> row2 in Uhor
//     Uman[2][0]== 0.01242068; Uman[2][1]==-0.111786149;    Uman[2][2]== 0.99365466;   // -> row3 in Uhor
//     Uman[0][0]== 0.9938837;  Uman[0][1]== 0.11043152607;  Uman[0][2]== 0.;           // -> row1 in Uhor
//
//
// and the rotation around this matrix is different!

    // ws2D->mutableSample().setOrientedLattice(ol);
    // ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,40);
    // TS_ASSERT_THROWS_NOTHING(rot=MsliceTransf.getTransfMatrix(TWS,scales));
    // // 40 degree rotation: [transf,u_to_rlu]=calc_proj_matrix(alat, angldeg, u, v, 40*deg2rad, omega, dpsi, gl, gs), u,v as above
    //  //  0.8319    0.5548    0.0124
    ////  -0.5502    0.8275   -0.1118
    ////  -0.0723    0.0862    0.9937
    // Transf[0][0] = 0.8319;      Transf[0][1] = 0.5548; Transf[0][2] = 0.0124;
    // Transf[1][0] =-0.5502;      Transf[1][1] = 0.8275; Transf[1][2] =-0.1118;
    // Transf[2][0] = -0.0723;     Transf[2][1] = 0.0862; Transf[2][2] = 0.9937;

    // sample = (PermHM*Transf*PermMH).getVector();
    // for(size_t i=0;i<9;i++)
    // {
    //   TS_ASSERT_DELTA(sample[i],rot[i],1.e-4);
    // }
}
void testTransf2HKL()
{
     MDEvents::MDWSDescription TWS;
     std::vector<double> minVal(4,-3),maxVal(4,3);
     TWS.setMinMax(minVal,maxVal);

     TWS.buildFromMatrixWS(ws2D,"Q3D","Direct");

     std::vector<double> rot;

     MDWSTransformTestHelper MsliceTransf;
 
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,0);
     // this is Wollastonite
     Geometry::OrientedLattice latt(7.9250,7.3200,7.0650,90.0550,95.2170,103.4200);
     ws2D->mutableSample().setOrientedLattice(&latt);
     // 
     //[transf,u_to_rlu]=calc_proj_matrix([7.9250,7.3200,7.0650], 90.0550,95.2170,103.4200, u, v, 0, omega, dpsi, gl, gs)
     // u to rlu
     //1.2215   -0.2928  -0.1147
     //    0    1.1650   -0.0011
     //    0         0    1.1244
     Kernel::Matrix<double> U2RLU(3,3);  // 0 deg rotation, these settins results in inverse B-matrix:
     U2RLU[0][0] = 1.2215;      U2RLU[0][1] = -0.2928; U2RLU[0][2]  = -0.1147;
     U2RLU[1][0] = 0;           U2RLU[1][1] = 1.1650;  U2RLU[1][2]  = -0.0011;
     U2RLU[2][0] = 0.;          U2RLU[2][1] = 0.;      U2RLU[2][2]   = 1.1244;

     CnvrtToMD::CoordScaling scales = CnvrtToMD::HKLScale;
     TS_ASSERT_THROWS_NOTHING(rot=MsliceTransf.getTransfMatrix(TWS,CnvrtToMD::AutoSelect,scales));

     auto sample = U2RLU.getVector();
     for(size_t i=0;i<9;i++)
     {
       TS_ASSERT_DELTA(sample[i],rot[i],1.e-4);
     }

}


void testModQAnyLattice()
{
   MDEvents::MDWSDescription TWS;
   std::vector<double> rot,sample(9,0);

   Mantid::API::MatrixWorkspace_sptr spws =WorkspaceCreationHelper::Create2DWorkspaceBinned(10,10);
   //Mantid::API::MatrixWorkspace_sptr spws =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
   std::vector<double> minVal(2,0),maxVal(2,3);
   TWS.setMinMax(minVal,maxVal);

   TWS.buildFromMatrixWS(spws,"|Q|","Direct");


   MDWSTransformTestHelper MsliceTransf;

   CnvrtToMD::CoordScaling scales = CnvrtToMD::NoScaling;
   TS_ASSERT_THROWS_NOTHING(rot=MsliceTransf.getTransfMatrix(TWS,CnvrtToMD::HKLFrame,scales));
   sample[0]=sample[4]=sample[8]=1;

   TS_ASSERT_DELTA(sample,rot,1.e-7);

}


MDWSTransfTest():
PermMH(3,3)
{
     ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    // rotate the crystal by twenty degrees back around y-axis;
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,20);
     // add workspace energy
     ws2D->mutableRun().addProperty("Ei",13.,"meV",true);

     pLattice = new Geometry::OrientedLattice(3,3,2,90,90,90);
     ws2D->mutableSample().setOrientedLattice(pLattice);

    // S_mantid*k_mantid = S_hor*k_hor; -- both Mantid and Horace produce the same kind of crystal frame
    // Define the permutation matrix which transforms Mantid coordinate system (beam along Z-axis)
    // to Horace  coordinate system (beam along X-axis) 
    // k_horace = PermHM*k_mantid
  //  Horace transformation matrix and Mantid Transformation matrix are connected by ration T_mantid  = T_hor*PermMH;
//     0     0     1
//     1     0     0
//     0     1     0
     PermMH[0][2]=1; PermMH[1][0]=1; PermMH[2][1]=1;
     PermHM = PermMH;
     PermHM.Invert();
 

}


};
#endif
