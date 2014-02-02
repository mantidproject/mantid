#ifndef MANTID_MDEVENTS_MDTRANSF_MODQTEST_H_
#define MANTID_MDEVENTS_MDTRANSF_MODQTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDEvents/MDTransfQ3D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
//#include "MantidMDEvents/MDTransfDEHelper.h"
#include "MantidKernel/DeltaEMode.h"


using namespace Mantid;
using namespace Mantid::MDEvents;

class MDTransfModQTestHelper: public MDTransfModQ
{
public: 
  double const * getSinThetaArray()const{return m_Sin2ThetaSqArray;}
  //double getCurSinThetaSq()const{return m_Sin2ThetaSq;}
};

//
class MDTransfModQTest : public CxxTest::TestSuite
{
  Mantid::API::MatrixWorkspace_sptr ws2D ;
public:
static MDTransfModQTest *createSuite() { return new MDTransfModQTest(); }
static void destroySuite(MDTransfModQTest * suite) { delete suite; }    

void testWSDescriptionPart()
{
 

  MDTransfModQ ModQTransformer;
  TS_ASSERT_EQUALS("|Q|",ModQTransformer.transfID());

  TS_ASSERT_EQUALS(2,ModQTransformer.getNMatrixDimensions(Kernel::DeltaEMode::Direct));
  TS_ASSERT_EQUALS(1,ModQTransformer.getNMatrixDimensions(Kernel::DeltaEMode::Elastic));
  TS_ASSERT_EQUALS(2,ModQTransformer.getNMatrixDimensions(Kernel::DeltaEMode::Indirect));
}
void testWSDescrUnitsPart()
{
  MDTransfModQ ModQTransformer;
  std::vector<std::string> outputDimUnits;

  TS_ASSERT_THROWS_NOTHING(outputDimUnits=ModQTransformer.outputUnitID(Kernel::DeltaEMode::Direct));
  TS_ASSERT_EQUALS(2,outputDimUnits.size());
  TS_ASSERT_EQUALS("MomentumTransfer",outputDimUnits[0]);
  TS_ASSERT_EQUALS("DeltaE",outputDimUnits[1]);

  TS_ASSERT_THROWS_NOTHING(outputDimUnits=ModQTransformer.outputUnitID(Kernel::DeltaEMode::Elastic));
  TS_ASSERT_EQUALS(1,outputDimUnits.size());
}
void testWSDescrIDPart()
{
  MDTransfModQ ModQTransformer;
  std::vector<std::string> outputDimID;

  TS_ASSERT_THROWS_NOTHING(outputDimID=ModQTransformer.getDefaultDimID(Kernel::DeltaEMode::Direct));
  TS_ASSERT_EQUALS(2,outputDimID.size());
  TS_ASSERT_EQUALS("|Q|",outputDimID[0]);
  TS_ASSERT_EQUALS("DeltaE",outputDimID[1]);

  TS_ASSERT_THROWS_NOTHING(outputDimID=ModQTransformer.getDefaultDimID(Kernel::DeltaEMode::Elastic));
  TS_ASSERT_EQUALS(1,outputDimID.size());
  TS_ASSERT_EQUALS("|Q|",outputDimID[0]);

}
void testWSDescrInputUnitID()
{
  MDTransfModQ ModQTransformer;
  std::string inputUnitID;

  TS_ASSERT_THROWS_NOTHING(inputUnitID=ModQTransformer.inputUnitID(Kernel::DeltaEMode::Direct));
  TS_ASSERT_EQUALS("DeltaE",inputUnitID);

  TS_ASSERT_THROWS_NOTHING(inputUnitID=ModQTransformer.inputUnitID(Kernel::DeltaEMode::Indirect));
  TS_ASSERT_EQUALS("DeltaE",inputUnitID);

  TS_ASSERT_THROWS_NOTHING(inputUnitID=ModQTransformer.inputUnitID(Kernel::DeltaEMode::Elastic));
  TS_ASSERT_EQUALS("Momentum",inputUnitID);


}
void xestISLorents()
{
  MDTransfModQTestHelper ModQTransf;




  MDWSDescription WSDescr(5);
  std::string QMode = ModQTransf.transfID();
  std::string dEMode= Kernel::DeltaEMode::asString(Kernel::DeltaEMode::Elastic);
  std::vector<std::string> dimPropNames(2,"T");
  dimPropNames[1]="Ei";

  WSDescr.buildFromMatrixWS(ws2D,QMode,dEMode,dimPropNames);

  TSM_ASSERT_THROWS("No detectors yet defined, so should thow run time error: ",ModQTransf.initialize(WSDescr),std::runtime_error);

  // let's preprocess detectors positions to go any further
  WSDescr.m_PreprDetTable = WorkspaceCreationHelper::buildPreprocessedDetectorsWorkspace(ws2D);
  // let's set 2Theta=0 for simplicity and violate const correctness for testing purposes here
  auto &TwoTheta  = const_cast<std::vector<double> &>(WSDescr.m_PreprDetTable->getColVector<double>("TwoTheta"));
  for(size_t i=0;i<TwoTheta.size();i++)
  {
    TwoTheta[i]=0;
  }

  TSM_ASSERT_THROWS_NOTHING("should initialize properly: ",ModQTransf.initialize(WSDescr));


  WSDescr.setLorentsCorr(true);
  TSM_ASSERT_THROWS_NOTHING("should initialize properly: ",ModQTransf.initialize(WSDescr));

  TSM_ASSERT("Array of sin(Theta)^2 should be defined: ",ModQTransf.getSinThetaArray());
 

  // specify a 5D vector to accept MD coordinates
  std::vector<coord_t> coord(5);
  TSM_ASSERT("Generic coordinates should be in range, so should be true ",ModQTransf.calcGenericVariables(coord,5));
  TSM_ASSERT_DELTA("4th Generic coordinates should be temperature ",70,coord[3],2.e-8);
  TSM_ASSERT_DELTA("5th Generic coordinates should be Ei ",13,coord[4],2.e-8);

  TSM_ASSERT(" Y-dependent coordinates should be in range so it should be true: ",ModQTransf.calcYDepCoordinates(coord,0));
  //TSM_ASSERT_DELTA("Sin(theta)^2 should be set to 0 by previous command ",0,ModQTransf.getCurSinThetaSq(),2.e-8);

  double signal(1),errorSq(1);

  TSM_ASSERT(" Matrix coordinates should be in range so function return true: ",ModQTransf.calcMatrixCoord(10,coord,signal,errorSq));

  TSM_ASSERT_DELTA(" But lorentz corrections for the detector placed on the beam path should set signal to 0 ",0,signal,2.e-8);
  TSM_ASSERT_DELTA(" But lorentz corrections for the detector placed on the beam path should set  error to 0 ",0,errorSq,2.e-8);
}



MDTransfModQTest()
{

     ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    // rotate the crystal by twenty degrees back;
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,20);
     // add workspace energy
     ws2D->mutableRun().addProperty("Ei",13.,"meV",true);
     ws2D->mutableRun().addProperty("T",70.,"K",true);

}
};

#endif