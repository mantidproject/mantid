#ifndef MANTID_MDALGORITHMS_CALCULATECOVERAGEDGSTEST_H_
#define MANTID_MDALGORITHMS_CALCULATECOVERAGEDGSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/CalculateCoverageDGS.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Matrix.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include <vector>
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"

using Mantid::MDAlgorithms::CalculateCoverageDGS;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;

class CalculateCoverageDGSTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateCoverageDGSTest *createSuite() { return new CalculateCoverageDGSTest(); }
  static void destroySuite( CalculateCoverageDGSTest *suite ) { delete suite; }


  void test_Init()
  {
    CalculateCoverageDGS alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("CalculateCoverageDGSTest_OutputWS"),inputWSName("CalculateCoverageDGSTest_InputWS");
    MatrixWorkspace_sptr inputWorkspace=WorkspaceCreationHelper::Create2DWorkspace(1,1);
    std::vector<V3D> detectorPositions;
    V3D sampPos(0.,0.,0.),sourcePos(0,0,-1.);
    detectorPositions.push_back(V3D(1,1,1));
    WorkspaceCreationHelper::createInstrumentForWorkspaceWithDistances(inputWorkspace,sampPos,sourcePos, detectorPositions);
    OrientedLattice ol(2,2,2,90,90,90);
    inputWorkspace->mutableSample().setOrientedLattice(&ol);
    Goniometer gon(DblMatrix(3,3,true));
    inputWorkspace->mutableRun().setGoniometer(gon, true);
    inputWorkspace->mutableRun().addLogData(new PropertyWithValue<double>("Ei", 3.));
    AnalysisDataService::Instance().add(inputWSName, inputWorkspace);
    CalculateCoverageDGS alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inputWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Dimension1Min", "-1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Dimension1Max", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Dimension1Step", "0.05") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Dimension2Min", "-1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Dimension2Max", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Dimension2Step", "0.05") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Dimension3Min", "-1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Dimension3Max", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Dimension3Step", "0.05") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Dimension4Min", "-1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Dimension4Max", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Dimension4Step", "0.05") );


    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    MDHistoWorkspace_sptr out = boost::dynamic_pointer_cast<MDHistoWorkspace>(ws);
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->getNumDims(),4);
    TS_ASSERT_EQUALS(out->getDimension(0)->getNBins(),40);
    double Ei=3.,dE,ki,kf,energyToK = 8.0*M_PI*M_PI*Mantid::PhysicalConstants::NeutronMass*Mantid::PhysicalConstants::meV*1e-20 /
            (Mantid::PhysicalConstants::h*Mantid::PhysicalConstants::h);
    DblMatrix inverserubw(3,3,true);
    inverserubw/=M_PI;
    double phi=inputWorkspace->getInstrument()->getDetector(0)->getPhi();
    double twoTheta=inputWorkspace->getInstrument()->getDetector(0)->getTwoTheta(sampPos,sourcePos*(-1.));
    for(dE=-0.99;dE<0.99;dE+=0.05)
    {
        ki=std::sqrt(energyToK*Ei);
        kf=std::sqrt(energyToK*(Ei-dE));
        V3D q(-kf*sin(twoTheta)*cos(phi),-kf*sin(twoTheta)*sin(phi),ki-kf*cos(twoTheta));
        q=inverserubw*q;
        double h=q.X(),k=q.Y(),l=q.Z();
        double signal;
        std::vector<Mantid::coord_t> pos(4);
        pos[0]=static_cast<Mantid::coord_t>(h);
        pos[1]=static_cast<Mantid::coord_t>(k);
        pos[2]=static_cast<Mantid::coord_t>(l);
        pos[3]=static_cast<Mantid::coord_t>(dE);
        size_t index=out->getLinearIndexAtCoord(pos.data());
        signal=out->getSignalAt(index);
        TS_ASSERT_EQUALS(signal,1);
        pos[0]=0.5f;
        index=out->getLinearIndexAtCoord(pos.data());
        signal=out->getSignalAt(index);
        TS_ASSERT_LESS_THAN(signal,0.1);
    }
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  


};


#endif /* MANTID_MDALGORITHMS_CALCULATECOVERAGEDGSTEST_H_ */
