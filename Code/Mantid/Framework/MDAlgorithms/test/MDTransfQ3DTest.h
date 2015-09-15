#ifndef MANTID_MDALGORITHMS_MDTRANSFQ3D_H_
#define MANTID_MDALGORITHMS_MDTRANSFQ3D_H_

#include "MantidKernel/DeltaEMode.h"
#include "MantidMDAlgorithms/MDTransfQ3D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;

using Mantid::coord_t;

class MDTransfQ3DTestHelper: public MDTransfQ3D
{
public: 
  bool getLorentzCorr()const{return m_isLorentzCorrected;}
  double const * getSinThetaArray()const{return m_SinThetaSqArray;}
  double getCurSinThetaSq()const{return m_SinThetaSq;}
};

//
class MDTransfQ3DTest : public CxxTest::TestSuite
{
  Mantid::API::MatrixWorkspace_sptr ws2D ;
public:
static MDTransfQ3DTest *createSuite() { return new MDTransfQ3DTest(); }
static void destroySuite(MDTransfQ3DTest * suite) { delete suite; }    

void testWSDescriptionPart()
{
 

  MDTransfQ3D Q3DTransformer;
  TS_ASSERT_EQUALS("Q3D",Q3DTransformer.transfID());

  TS_ASSERT_EQUALS(4,Q3DTransformer.getNMatrixDimensions(DeltaEMode::Direct));
  TS_ASSERT_EQUALS(3,Q3DTransformer.getNMatrixDimensions(DeltaEMode::Elastic));
  TS_ASSERT_EQUALS(4,Q3DTransformer.getNMatrixDimensions(DeltaEMode::Indirect));
}
void testWSDescrUnitsPart()
{
  MDTransfQ3D Q3DTransformer;
  std::vector<std::string> outputDimUnits;

  TS_ASSERT_THROWS_NOTHING(outputDimUnits=Q3DTransformer.outputUnitID(DeltaEMode::Direct));
  TS_ASSERT_EQUALS(4,outputDimUnits.size());
  TS_ASSERT_EQUALS("MomentumTransfer",outputDimUnits[0]);
  TS_ASSERT_EQUALS("MomentumTransfer",outputDimUnits[1]);
  TS_ASSERT_EQUALS("MomentumTransfer",outputDimUnits[2]);
  TS_ASSERT_EQUALS("DeltaE",outputDimUnits[3]);

  TS_ASSERT_THROWS_NOTHING(outputDimUnits=Q3DTransformer.outputUnitID(DeltaEMode::Elastic));
  TS_ASSERT_EQUALS(3,outputDimUnits.size());
}
void testWSDescrIDPart()
{
  MDTransfQ3D Q3DTransformer;
  std::vector<std::string> outputDimID;

  TS_ASSERT_THROWS_NOTHING(outputDimID=Q3DTransformer.getDefaultDimID(DeltaEMode::Direct));
  TS_ASSERT_EQUALS(4,outputDimID.size());
  TS_ASSERT_EQUALS("Q1",outputDimID[0]);
  TS_ASSERT_EQUALS("Q2",outputDimID[1]);
  TS_ASSERT_EQUALS("Q3",outputDimID[2]);
  TS_ASSERT_EQUALS("DeltaE",outputDimID[3]);

  TS_ASSERT_THROWS_NOTHING(outputDimID=Q3DTransformer.getDefaultDimID(DeltaEMode::Elastic));
  TS_ASSERT_EQUALS(3,outputDimID.size());
  TS_ASSERT_EQUALS("Q1",outputDimID[0]);
  TS_ASSERT_EQUALS("Q2",outputDimID[1]);
  TS_ASSERT_EQUALS("Q3",outputDimID[2]);

}
void testWSDescrInputUnitID()
{
  MDTransfQ3D Q3DTransformer;
  std::string inputUnitID;

  TS_ASSERT_THROWS_NOTHING(inputUnitID=Q3DTransformer.inputUnitID(DeltaEMode::Direct));
  TS_ASSERT_EQUALS("DeltaE",inputUnitID);

  TS_ASSERT_THROWS_NOTHING(inputUnitID=Q3DTransformer.inputUnitID(DeltaEMode::Indirect));
  TS_ASSERT_EQUALS("DeltaE",inputUnitID);

  TS_ASSERT_THROWS_NOTHING(inputUnitID=Q3DTransformer.inputUnitID(DeltaEMode::Elastic));
  TS_ASSERT_EQUALS("Momentum",inputUnitID);


}
void testISLorents()
{
  MDTransfQ3DTestHelper Q3DTransf;


  TSM_ASSERT("Should not be lorentz corrected by default ",!Q3DTransf.getLorentzCorr());


  MDWSDescription WSDescr(5);
  std::string QMode = Q3DTransf.transfID();
  std::string dEMode= DeltaEMode::asString(DeltaEMode::Elastic);
  std::vector<std::string> dimPropNames(2,"T");
  dimPropNames[1]="Ei";

  WSDescr.buildFromMatrixWS(ws2D,QMode,dEMode,dimPropNames);

  TSM_ASSERT_THROWS("No detectors yet defined, so should thow run time error: ",Q3DTransf.initialize(WSDescr),std::runtime_error);

  // let's preprocess detectors positions to go any further
  WSDescr.m_PreprDetTable = WorkspaceCreationHelper::buildPreprocessedDetectorsWorkspace(ws2D);
  // let's set 2Theta=0 for simplicity and violate const correctness for testing purposes here
  auto &TwoTheta  = const_cast<std::vector<double> &>(WSDescr.m_PreprDetTable->getColVector<double>("TwoTheta"));
  for(size_t i=0;i<TwoTheta.size();i++)
  {
    TwoTheta[i]=0;
  }

  TSM_ASSERT_THROWS_NOTHING("should initialize properly: ",Q3DTransf.initialize(WSDescr));
  TSM_ASSERT("Should not be lorentz corrected by default ",!Q3DTransf.getLorentzCorr());


  WSDescr.setLorentsCorr(true);
  TSM_ASSERT_THROWS_NOTHING("should initialize properly: ",Q3DTransf.initialize(WSDescr));
  TSM_ASSERT("Lorentz corrections should be now set ",Q3DTransf.getLorentzCorr());

  TSM_ASSERT("Array of sin(Theta)^2 should be defined: ",Q3DTransf.getSinThetaArray());
 

  // specify a 5D vector to accept MD coordinates
  std::vector<coord_t> coord(5);
  TSM_ASSERT("Generic coordinates should be in range, so should be true ",Q3DTransf.calcGenericVariables(coord,5));
  TSM_ASSERT_DELTA("4th Generic coordinates should be temperature ",70,coord[3],2.e-8);
  TSM_ASSERT_DELTA("5th Generic coordinates should be Ei ",13,coord[4],2.e-8);

  TSM_ASSERT(" Y-dependent coordinates should be in range so it should be true: ",Q3DTransf.calcYDepCoordinates(coord,0));
  TSM_ASSERT_DELTA("Sin(theta)^2 should be set to 0 by previous command ",0,Q3DTransf.getCurSinThetaSq(),2.e-8);

  double signal(1),errorSq(1);

  TSM_ASSERT(" Matrix coordinates should be in range so function return true: ",Q3DTransf.calcMatrixCoord(10,coord,signal,errorSq));

  TSM_ASSERT_DELTA(" But lorentz corrections for the detector placed on the beam path should set signal to 0 ",0,signal,2.e-8);
  TSM_ASSERT_DELTA(" But lorentz corrections for the detector placed on the beam path should set  error to 0 ",0,errorSq,2.e-8);
}



MDTransfQ3DTest()
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
