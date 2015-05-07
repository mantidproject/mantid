#ifndef CROPWORKSPACETEST_H_
#define CROPWORKSPACETEST_H_

#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <limits>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class CropWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CropWorkspaceTest *createSuite() { return new CropWorkspaceTest(); }
  static void destroySuite( CropWorkspaceTest *suite ) { delete suite; }

  std::string createInputWorkspace()
  {
    std::string name = "toCrop";
    if( !AnalysisDataService::Instance().doesExist("toCrop") )
    {
      // Set up a small workspace for testing
      Workspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",5,6,5);
      Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
      double *a = new double[25];
      double *e = new double[25];
      for (int i = 0; i < 25; ++i)
      {
        a[i]=i;
        e[i]=sqrt(double(i));
      }
      for (int j = 0; j < 5; ++j) {
        for (int k = 0; k < 6; ++k) {
          space2D->dataX(j)[k] = k;
        }
        space2D->setData(j, boost::shared_ptr<Mantid::MantidVec>(new std::vector<double>(a+(5*j), a+(5*j)+5)),
                         boost::shared_ptr<Mantid::MantidVec>(new std::vector<double>(e+(5*j), e+(5*j)+5)));
      }
      // Register the workspace in the data service
      AnalysisDataService::Instance().add(name, space);
    }
    return name;
  }


  void testName()
  {
    TS_ASSERT_EQUALS( crop.name(), "CropWorkspace" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( crop.version(), 1 );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( crop.initialize() );
    TS_ASSERT( crop.isInitialized() );
  }

  void testInvalidInputs()
  {
    std::string inputName = createInputWorkspace();
    if ( !crop.isInitialized() ) crop.initialize();

    TS_ASSERT_THROWS( crop.execute(), std::runtime_error );
    TS_ASSERT( !crop.isExecuted() );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("InputWorkspace",inputName) );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("OutputWorkspace","nothing") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("XMin","2") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("XMax","1") );
    TS_ASSERT_THROWS_NOTHING( crop.execute() );
    TS_ASSERT( !crop.isExecuted() );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("XMax","2.5") );
    TS_ASSERT_THROWS_NOTHING( crop.execute() );
    TS_ASSERT( !crop.isExecuted() );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("XMax","5") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("StartWorkspaceIndex","10") );
    TS_ASSERT_THROWS_NOTHING( crop.execute() );
    TS_ASSERT( !crop.isExecuted() );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("StartWorkspaceIndex","4") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("EndWorkspaceIndex","10") );
    TS_ASSERT_THROWS_NOTHING( crop.execute() );
    TS_ASSERT( !crop.isExecuted() );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("EndWorkspaceIndex","2") );
    TS_ASSERT_THROWS_NOTHING( crop.execute() );
    TS_ASSERT( !crop.isExecuted() );
  }

  void makeFakeEventWorkspace(std::string wsName)
  {
    //Make an event workspace with 2 events in each bin.
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::CreateEventWorkspace(36, 50, 50, 0.0, 2., 2);
    //Fake a unit in the data.
    test_in->getAxis(0)->unit() =UnitFactory::Instance().create("TOF");
    test_in->setInstrument( ComponentCreationHelper::createTestInstrumentCylindrical(4, false) );
    //Add it to the workspace
    AnalysisDataService::Instance().add(wsName, test_in);
  }

  void test_CropWorkspaceEventsInplace()
  {
    // setup
    std::string eventname("TestEvents");
    this->makeFakeEventWorkspace(eventname);
    EventWorkspace_sptr ws
         = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(eventname);

    // run the algorithm
    CropWorkspace algo;
    if (!algo.isInitialized()) algo.initialize();
    algo.setPropertyValue("InputWorkspace", eventname);
    algo.setPropertyValue("OutputWorkspace", eventname);
    TS_ASSERT_THROWS_NOTHING( algo.setPropertyValue("XMin","40.") );
    TS_ASSERT_THROWS_NOTHING( algo.setPropertyValue("XMax","50.") );
    TS_ASSERT_THROWS_NOTHING( algo.setPropertyValue("StartWorkspaceIndex","2") );
    TS_ASSERT_THROWS_NOTHING( algo.setPropertyValue("EndWorkspaceIndex","4") );
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());
    if ( !algo.isExecuted() ) return;

    // verify the output workspace
    ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(eventname);
    TS_ASSERT_EQUALS(3, ws->getNumberHistograms()); // reduced histograms
    TS_ASSERT_EQUALS(30, ws->getNumberEvents()); 

    TS_ASSERT(40. <= ws->getEventList(0).getTofMin());
    TS_ASSERT(50. >= ws->getEventList(0).getTofMax());

    TS_ASSERT(40. <= ws->getEventList(2).getTofMin());
    TS_ASSERT(50. >= ws->getEventList(2).getTofMax());
  }

  void testExec()
  {
    std::string inputName = createInputWorkspace();
    if ( !crop.isInitialized() ) crop.initialize();

    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("InputWorkspace",inputName) );
    std::string outputWS("cropped");
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("XMin","0.1") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("XMax","4") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("StartWorkspaceIndex","2") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("EndWorkspaceIndex","4") );

    TS_ASSERT_THROWS_NOTHING( crop.execute() );
    TS_ASSERT( crop.isExecuted() );
    if ( !crop.isExecuted() ) return;

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWS) );

    TS_ASSERT_EQUALS( output->getNumberHistograms(), 3 );
    TS_ASSERT_EQUALS( output->blocksize(), 3 );

    MatrixWorkspace_const_sptr input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("toCrop");
    for (int i=0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        TS_ASSERT_EQUALS( output->readX(i)[j], input->readX(i+2)[j+1] );
        TS_ASSERT_EQUALS( output->readY(i)[j], input->readY(i+2)[j+1] );
        TS_ASSERT_EQUALS( output->readE(i)[j], input->readE(i+2)[j+1] );
      }
      TS_ASSERT_EQUALS( output->readX(i)[3], input->readX(i+2)[4] );
      TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(i), input->getAxis(1)->spectraNo(i+2) );
      TS_ASSERT_EQUALS( output->getSpectrum(i)->getDetectorIDs(), input->getSpectrum(i+2)->getDetectorIDs() );
    }
  }

  void testExecWithDefaults()
  {
    std::string inputName = createInputWorkspace();
    CropWorkspace crop2;
    TS_ASSERT_THROWS_NOTHING( crop2.initialize() );
    TS_ASSERT_THROWS_NOTHING( crop2.setPropertyValue("InputWorkspace",inputName) );
    TS_ASSERT_THROWS_NOTHING( crop2.setPropertyValue("OutputWorkspace","unCropped") );
    TS_ASSERT_THROWS_NOTHING( crop2.execute() );
    TS_ASSERT( crop2.isExecuted() );
    if ( !crop2.isExecuted() ) return;

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("unCropped") );
    MatrixWorkspace_const_sptr input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("toCrop");

    const size_t xsize = output->blocksize();
    for(size_t i = 0; i < output->getNumberHistograms(); ++i)
    {
      const auto & outX = output->readX(i);
      const auto & outY = output->readY(i);
      const auto & outE = output->readE(i);
      const auto & inX = input->readX(i);
      const auto & inY = input->readY(i);
      const auto & inE = input->readE(i);

      for(size_t j = 0; j < xsize; ++j)
      {
        TS_ASSERT_EQUALS(outX[j], inX[j]);
        TS_ASSERT_EQUALS(outY[j], inY[j]);
        TS_ASSERT_EQUALS(outE[j], inE[j]);
      }
      TS_ASSERT_EQUALS(outX[xsize], inX[xsize]);
    }

    for (int i = 0; i < 5; ++i)
    {
      TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(i), input->getAxis(1)->spectraNo(i) );
      TS_ASSERT_EQUALS( output->getSpectrum(i)->getDetectorIDs(), input->getSpectrum(i)->getDetectorIDs() );
    }
  }

  void testWithPointData()
  {
    AnalysisDataService::Instance().add("point",WorkspaceCreationHelper::Create2DWorkspace123(5,5));
    CropWorkspace crop3;
    TS_ASSERT_THROWS_NOTHING( crop3.initialize() );
    TS_ASSERT_THROWS_NOTHING( crop3.setPropertyValue("InputWorkspace","point") );
    TS_ASSERT_THROWS_NOTHING( crop3.setPropertyValue("OutputWorkspace","pointOut") );
    TS_ASSERT_THROWS_NOTHING( crop3.execute() );
    TS_ASSERT( crop3.isExecuted() );
    if ( !crop3.isExecuted() ) return;

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("pointOut") );
    MatrixWorkspace_const_sptr input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("point");

    const size_t xsize = output->blocksize();
    for(size_t i = 0; i < output->getNumberHistograms(); ++i)
    {
      const auto & outX = output->readX(i);
      const auto & outY = output->readY(i);
      const auto & outE = output->readE(i);
      const auto & inX = input->readX(i);
      const auto & inY = input->readY(i);
      const auto & inE = input->readE(i);

      for(size_t j = 0; j < xsize; ++j)
      {
        TS_ASSERT_EQUALS(outX[j], inX[j]);
        TS_ASSERT_EQUALS(outY[j], inY[j]);
        TS_ASSERT_EQUALS(outE[j], inE[j]);
      }
    }


    AnalysisDataService::Instance().remove("point");
    AnalysisDataService::Instance().remove("pointOut");
  }

  void testRagged()
  {
    MatrixWorkspace_sptr input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("toCrop");
    // Change the first X vector
    for (int k = 0; k < 6; ++k) {
      input->dataX(0)[k] = k+3;
    }

    CropWorkspace crop4;
    TS_ASSERT_THROWS_NOTHING( crop4.initialize() );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("InputWorkspace","toCrop") );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("OutputWorkspace","raggedOut") );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("XMin","2.9") );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("XMax","4.1") );
    TS_ASSERT_THROWS_NOTHING( crop4.execute() );
    TS_ASSERT( crop4.isExecuted() );
    if ( !crop4.isExecuted() ) return;
    
    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("raggedOut") );

    TS_ASSERT_EQUALS( output->size(), input->size() );

    for (int i = 0; i < 5; ++i)
    {
      for (int j = 0; j < 5; ++j)
      {
        if ( (i == 0 && j == 0) || (i != 0 && j == 3) )
        {
          TS_ASSERT_EQUALS( output->readY(i)[j], input->readY(i)[j] );
        }
        else
        {
          TS_ASSERT_EQUALS( output->readY(i)[j], 0.0 );
        }
      }
    }
  }

  void testRagged_events()
  {
    // Event workspace with 10 bins from 0 to 10
    EventWorkspace_sptr input = WorkspaceCreationHelper::CreateEventWorkspace(5, 10, 10, 0.0, 1.0);
    // Change the first X vector to 3, 4, 5 ..
    for (int k = 0; k <= 10; ++k) {
      input->dataX(0)[k] = k+3;
    }
    CropWorkspace crop4;
    TS_ASSERT_THROWS_NOTHING( crop4.initialize() );
    TS_ASSERT_THROWS_NOTHING( crop4.setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(input) ) );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("OutputWorkspace","raggedOut") );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("XMin","2.9") );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("XMax","5.1") );
    TS_ASSERT_THROWS_NOTHING( crop4.execute() );
    TS_ASSERT( crop4.isExecuted() );
    if ( !crop4.isExecuted() ) return;

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("raggedOut") );
    // The number of bins is UNCHANGED because of ragged bins
    TS_ASSERT_EQUALS( output->size(), input->size() );
    TS_ASSERT_EQUALS( output->blocksize(), input->blocksize() );

    for (size_t i = 0; i < 5; ++i)
    {
      const MantidVec & iX = input->readX(i);
      const MantidVec & oX = output->readX(i);
      for (size_t j = 0 ; j < iX.size(); j++)
      { TS_ASSERT_EQUALS( iX[j], oX[j] ); }
    }
  }

  void testNegativeBinBoundaries()
  {
    const std::string wsName("neg");
    AnalysisDataService::Instance().add(wsName,WorkspaceCreationHelper::Create2DWorkspaceBinned(1,5,-6));
    CropWorkspace crop4;
    TS_ASSERT_THROWS_NOTHING( crop4.initialize() );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("InputWorkspace",wsName) );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("OutputWorkspace",wsName) );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("XMin","-5") );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("XMax","-2") );
    TS_ASSERT( crop4.execute() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName) );

    TSM_ASSERT_EQUALS( "The number of bins", 3, output->blocksize() );
    TSM_ASSERT_EQUALS( "First bin boundary", -5, output->readX(0).front() );
    TSM_ASSERT_EQUALS( "Last bin boundary", -2, output->readX(0).back() );

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_Input_With_TextAxis()
  {
    Algorithm *cropper = new CropWorkspace;
    cropper->initialize();
    cropper->setPropertyValue("StartWorkspaceIndex", "1");
    cropper->setPropertyValue("EndWorkspaceIndex", "1");
    doTestWithTextAxis(cropper); //Takes ownership
  }

  // Public so it can be used within ExtractSingleSpectrum test
  static void doTestWithTextAxis(Algorithm *alg)
  {
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::Create2DWorkspace(3, 10);
    // Change the data so we know we've cropped the correct one
    const size_t croppedIndex(1);
    const double flagged(100.0);
    for( size_t i = 0; i < inputWS->blocksize(); ++i )
    {
      inputWS->dataY(croppedIndex)[i] = flagged;
    }
    const char *labels[3] = {"Entry1","Entry2","Entry3"};
    TextAxis *inputTextAxis = new TextAxis(3);
    for( int i = 0; i < 3; ++i )
    {
      inputTextAxis->setLabel(i, labels[i]);
    }
    inputWS->replaceAxis(1, inputTextAxis);
    
    // Run and test
    alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
    const std::string wsName("CropWS_TextAxis");
    alg->setPropertyValue("OutputWorkspace",wsName);
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    if ( !alg->isExecuted() ) return;

    // Check the output
    MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));
    if( !outputWS ) TS_FAIL("CropWorkspace did not execute correctly.");

    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outputWS->blocksize(), 10);
    TS_ASSERT_EQUALS(outputWS->getAxis(1)->isText(), true);
    TS_ASSERT_EQUALS(outputWS->getAxis(1)->label(0), labels[1]);
   
    AnalysisDataService::Instance().remove(wsName);
    delete alg;
    
  }
  
private:
  CropWorkspace crop;
};

class CropWorkspaceTestPerformance : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CropWorkspaceTestPerformance *createSuite() { return new CropWorkspaceTestPerformance(); }
  static void destroySuite( CropWorkspaceTestPerformance *suite ) { delete suite; }

  void setUp()
  {
    AnalysisDataService::Instance().add("ToCrop",
        WorkspaceCreationHelper::CreateEventWorkspace(5000,10000,8000, 0.0, 1.0, 3) );
  }

  void tearDown()
  {
    AnalysisDataService::Instance().remove("ToCrop");
  }

  void test_crop_events_inplace()
  {
    CropWorkspace cropper;
    cropper.initialize();
    cropper.setPropertyValue("InputWorkspace","ToCrop");
    cropper.setPropertyValue("OutputWorkspace","ToCrop");
    cropper.setProperty("XMin",5000.0);
    cropper.setProperty("XMax",7500.0);
    TS_ASSERT( cropper.execute() );
  }
};

#endif /*CROPWORKSPACETEST_H_*/
