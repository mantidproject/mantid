#ifndef MANTID_MDEVENTS_MDTRANSF_MODQTEST_H_
#define MANTID_MDEVENTS_MDTRANSF_MODQTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDEvents/MDTransfQ3D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
//#include "MantidMDEvents/MDTransfDEHelper.h"
#include "MantidKernel/DeltaEMode.h"


using namespace Mantid;
using namespace Mantid::MDEvents;


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

  std::string checkMinMaxRangesCorrect(MDWSDescription &WSDescr,Mantid::API::MatrixWorkspace_sptr &ws2D,MDTransfInterface &MDTransf)
  {

    std::string result("");
    // auxiliary variables not used here
    double signal(1),errorSq(1);
    unsigned int nDims = WSDescr.nDimensions();

    auto detIDMap = WSDescr.m_PreprDetTable->getColVector<size_t>("detIDMap");
 
    std::vector<coord_t> locCoord(nDims);
    std::vector<coord_t> minCoord(nDims,FLT_MAX);
    std::vector<coord_t> maxCoord(nDims,-FLT_MAX);

    const size_t specSize = ws2D->blocksize();    
    size_t nDetectors = ws2D->getNumberHistograms();

    MDTransf.calcGenericVariables(locCoord,nDims);

    for(size_t i=0;i<nDetectors;i++)
    {
      size_t iSpctr             = detIDMap[i];
      const MantidVec& X        = ws2D->readX(iSpctr);

      auto range = MDTransf.getExtremumPoints(X[0],X[specSize],i);


      // calculate the coordinates which depend on detector posision and set up the detectors parameters
      MDTransf.calcYDepCoordinates(locCoord,i);

      for(size_t k=0;k<range.size();k++)
      {
        MDTransf.calcMatrixCoord(range[k],locCoord,signal, errorSq);
        for (size_t j=0;j<nDims;j++)
        {
          if (locCoord[j]<minCoord[j])minCoord[j]=locCoord[j];
          if (locCoord[j]>maxCoord[j])maxCoord[j]=locCoord[j];
        }
      }



      //=> START INTERNAL LOOP OVER THE "TIME"
      for (size_t j = 0; j < specSize; ++j)
      {
        MDTransf.calcMatrixCoordinates(X,i,j,locCoord,signal,errorSq);
        for (size_t j=0;j<nDims;j++)
        {
          if (locCoord[j]<minCoord[j])
          {
            result = "Local transformed coordinate in direction "+boost::lexical_cast<std::string>(j)+" at bin N: "+boost::lexical_cast<std::string>(i)+
                     " is smaller then identified conversion range";
            return result;
          }
          if (locCoord[j]>maxCoord[j])
          {
            result = "Local transformed coordinate in direction "+boost::lexical_cast<std::string>(j)+" at bin N: "+boost::lexical_cast<std::string>(i)+
                     " is bigger then identified conversion range";
            return result;
          }
        }

      }
    }
    return result;

  }

  void testExtremums()
  {
    MDTransfModQ ModQTransf;

    size_t nDims = 4;
    std::vector<double> L2,polar,azimuthal;
    WorkspaceCreationHelper::create2DAngles(L2,polar,azimuthal);

    MDWSDescription WSDescr(static_cast<unsigned int>(nDims));
    std::string QMode = ModQTransf.transfID();
    std::string dEMode= Kernel::DeltaEMode::asString(Kernel::DeltaEMode::Direct);
    std::vector<std::string> dimPropNames(2,"T");
    dimPropNames[1]="Ei";


    auto ws2Dbig =WorkspaceCreationHelper::createProcessedInelasticWS(L2,polar,azimuthal,100,-11,9.9,10);

    ws2Dbig->mutableRun().mutableGoniometer().setRotationAngle(0,20);
    // add workspace energy
    ws2Dbig->mutableRun().addProperty("Ei",13.,"meV",true);
    ws2Dbig->mutableRun().addProperty("T",70.,"K",true);


    WSDescr.buildFromMatrixWS(ws2Dbig,QMode,dEMode,dimPropNames);

    std::vector<double> minVal(nDims,-FLT_MAX),maxVal(nDims,FLT_MAX);
    WSDescr.setMinMax(minVal,maxVal);

    TSM_ASSERT_THROWS("No detectors yet defined, so should thow run time error: ",ModQTransf.initialize(WSDescr),std::runtime_error);

    // let's preprocess detectors positions to go any further
    WSDescr.m_PreprDetTable = WorkspaceCreationHelper::buildPreprocessedDetectorsWorkspace(ws2Dbig);
    TSM_ASSERT_THROWS_NOTHING("should initialize properly: ",ModQTransf.initialize(WSDescr));
    std::vector<coord_t> coord(4);
    TSM_ASSERT("Generic coordinates should be in range, so should be true ",ModQTransf.calcGenericVariables(coord,4));
    TSM_ASSERT_DELTA("3th Generic coordinates should be temperature ",70,coord[2],2.e-8);
    TSM_ASSERT_DELTA("4th Generic coordinates should be Ei ",13,coord[3],2.e-8);

    TSM_ASSERT(" Y-dependent coordinates should be in range so it should be true: ",ModQTransf.calcYDepCoordinates(coord,0));

    auto convResult = checkMinMaxRangesCorrect(WSDescr,ws2Dbig,ModQTransf);

    TSM_ASSERT_EQUALS(convResult,0,convResult.size());

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