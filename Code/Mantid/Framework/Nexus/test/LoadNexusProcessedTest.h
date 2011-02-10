#ifndef LOADNEXUSPROCESSEDTEST_H_
#define LOADNEXUSPROCESSEDTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidNexus/LoadNexusProcessed.h"
#include "MantidNexus/SaveNexusProcessed.h"
#include "SaveNexusProcessedTest.h"
#include <cxxtest/TestSuite.h>
#include <iostream>
#include <Poco/File.h>

using namespace Mantid::NeXus;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class LoadNexusProcessedTest : public CxxTest::TestSuite
{
public:

  LoadNexusProcessedTest() :
    testFile("GEM38370_Focussed_Legacy.nxs"),
    output_ws("nxstest")
  {

  }

  ~LoadNexusProcessedTest()
  {
    AnalysisDataService::Instance().clear();
  }

  void testProcessedFile()
  {
    LoadNexusProcessed alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    alg.setPropertyValue("Filename", testFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    
    //Test some aspects of the file
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING( workspace = AnalysisDataService::Instance().retrieve(output_ws) );
    TS_ASSERT( workspace.get() );

    MatrixWorkspace_sptr matrix_ws = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
    TS_ASSERT( matrix_ws.get() );


    // Test proton charge from the sample block
    TS_ASSERT_DELTA(matrix_ws->run().getProtonCharge(), 30.14816, 1e-5);

    //Test history
    const std::vector<AlgorithmHistory>& alghist = matrix_ws->getHistory().getAlgorithmHistories();
    int nalgs = static_cast<int>(alghist.size());
    TS_ASSERT_EQUALS(nalgs, 4);
      
    if( nalgs == 4 )
    {
      TS_ASSERT_EQUALS(alghist[0].name(), "LoadRaw");
      TS_ASSERT_EQUALS(alghist[1].name(), "AlignDetectors");
      TS_ASSERT_EQUALS(alghist[2].name(), "DiffractionFocussing");
      TS_ASSERT_EQUALS(alghist[3].name(), "LoadNexusProcessed");
    }
    
    boost::shared_ptr<Mantid::Geometry::Instrument> inst = matrix_ws->getBaseInstrument(); 
    TS_ASSERT_EQUALS(inst->getName(), "GEM");
    TS_ASSERT_EQUALS(inst->getSource()->getPos().Z(), -17);
   }

  void testNexusProcessed_Min_Max()
  {

    LoadNexusProcessed alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
    testFile="focussed.nxs";
    alg.setPropertyValue("Filename", testFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
	  alg.setPropertyValue("SpectrumMin","2");
    alg.setPropertyValue("SpectrumMax","4");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    
    //Test some aspects of the file
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING( workspace = AnalysisDataService::Instance().retrieve(output_ws) );
    TS_ASSERT( workspace.get() );

    MatrixWorkspace_sptr matrix_ws = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
    TS_ASSERT( matrix_ws.get() );

	  //Testing the number of histograms
    TS_ASSERT_EQUALS(matrix_ws->getNumberHistograms(),3);

    //Test history
    const std::vector<AlgorithmHistory>& alghist = matrix_ws->getHistory().getAlgorithmHistories();
    int nalgs = static_cast<int>(alghist.size());
    TS_ASSERT_EQUALS(nalgs, 4);
      
    if( nalgs == 4 )
    {
      TS_ASSERT_EQUALS(alghist[0].name(), "LoadRaw");
      TS_ASSERT_EQUALS(alghist[1].name(), "AlignDetectors");
      TS_ASSERT_EQUALS(alghist[2].name(), "DiffractionFocussing");
      TS_ASSERT_EQUALS(alghist[3].name(), "LoadNexusProcessed");
    }
    
    boost::shared_ptr<Mantid::Geometry::Instrument> inst = matrix_ws->getBaseInstrument(); 
    TS_ASSERT_EQUALS(inst->getName(), "GEM");
    TS_ASSERT_EQUALS(inst->getSource()->getPos().Z(), -17);

   }

  void testNexusProcessed_List()
  {
    LoadNexusProcessed alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
    testFile="focussed.nxs";
    alg.setPropertyValue("Filename", testFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
	  alg.setPropertyValue("SpectrumList","1,2,3,4");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    
    //Test some aspects of the file
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING( workspace = AnalysisDataService::Instance().retrieve(output_ws) );
    TS_ASSERT( workspace.get() );

    MatrixWorkspace_sptr matrix_ws = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
    TS_ASSERT( matrix_ws.get() );

	  //Testing the number of histograms
    TS_ASSERT_EQUALS(matrix_ws->getNumberHistograms(),4);

    //Test history
    const std::vector<AlgorithmHistory>& alghist = matrix_ws->getHistory().getAlgorithmHistories();
    int nalgs = static_cast<int>(alghist.size());
    TS_ASSERT_EQUALS(nalgs, 4);
      
    if( nalgs == 4 )
    {
      TS_ASSERT_EQUALS(alghist[0].name(), "LoadRaw");
      TS_ASSERT_EQUALS(alghist[1].name(), "AlignDetectors");
      TS_ASSERT_EQUALS(alghist[2].name(), "DiffractionFocussing");
      TS_ASSERT_EQUALS(alghist[3].name(), "LoadNexusProcessed");
    }
    
    boost::shared_ptr<Mantid::Geometry::Instrument> inst = matrix_ws->getBaseInstrument(); 
    TS_ASSERT_EQUALS(inst->getName(), "GEM");
    TS_ASSERT_EQUALS(inst->getSource()->getPos().Z(), -17);

   }

  void testNexusProcessed_Min_Max_List()
  {
    LoadNexusProcessed alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
    testFile="focussed.nxs";
    alg.setPropertyValue("Filename", testFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
	  alg.setPropertyValue("SpectrumMin","1");
    alg.setPropertyValue("SpectrumMax","3");
	  alg.setPropertyValue("SpectrumList","4,5");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    
    //Test some aspects of the file
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING( workspace = AnalysisDataService::Instance().retrieve(output_ws) );
    TS_ASSERT( workspace.get() );

    MatrixWorkspace_sptr matrix_ws = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
    TS_ASSERT( matrix_ws.get() );

	  //Testing the number of histograms
    TS_ASSERT_EQUALS(matrix_ws->getNumberHistograms(),5);

    //Test history
    const std::vector<AlgorithmHistory>& alghist = matrix_ws->getHistory().getAlgorithmHistories();
    int nalgs = static_cast<int>(alghist.size());
    TS_ASSERT_EQUALS(nalgs, 4);
      
    if( nalgs == 4 )
    {
      TS_ASSERT_EQUALS(alghist[0].name(), "LoadRaw");
      TS_ASSERT_EQUALS(alghist[1].name(), "AlignDetectors");
      TS_ASSERT_EQUALS(alghist[2].name(), "DiffractionFocussing");
      TS_ASSERT_EQUALS(alghist[3].name(), "LoadNexusProcessed");
    }
    
    boost::shared_ptr<Mantid::Geometry::Instrument> inst = matrix_ws->getBaseInstrument(); 
    TS_ASSERT_EQUALS(inst->getName(), "GEM");
    TS_ASSERT_EQUALS(inst->getSource()->getPos().Z(), -17);

   }

  void testNexusProcessed_Min()
  {
    LoadNexusProcessed alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
    testFile="focussed.nxs";
    alg.setPropertyValue("Filename", testFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
	  alg.setPropertyValue("SpectrumMin","4");
    
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    
    //Test some aspects of the file
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING( workspace = AnalysisDataService::Instance().retrieve(output_ws) );
    TS_ASSERT( workspace.get() );

    MatrixWorkspace_sptr matrix_ws = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
    TS_ASSERT( matrix_ws.get() );

	  //Testing the number of histograms
    TS_ASSERT_EQUALS(matrix_ws->getNumberHistograms(),3);

    //Test history
    const std::vector<AlgorithmHistory>& alghist = matrix_ws->getHistory().getAlgorithmHistories();
    int nalgs = static_cast<int>(alghist.size());
    TS_ASSERT_EQUALS(nalgs, 4);
      
    if( nalgs == 4 )
    {
      TS_ASSERT_EQUALS(alghist[0].name(), "LoadRaw");
      TS_ASSERT_EQUALS(alghist[1].name(), "AlignDetectors");
      TS_ASSERT_EQUALS(alghist[2].name(), "DiffractionFocussing");
      TS_ASSERT_EQUALS(alghist[3].name(), "LoadNexusProcessed");
    }
    
    boost::shared_ptr<Mantid::Geometry::Instrument> inst = matrix_ws->getBaseInstrument(); 
    TS_ASSERT_EQUALS(inst->getName(), "GEM");
    TS_ASSERT_EQUALS(inst->getSource()->getPos().Z(), -17);

   }

   void testNexusProcessed_Max()
  {
    LoadNexusProcessed alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
    testFile="focussed.nxs";
    alg.setPropertyValue("Filename", testFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
	  alg.setPropertyValue("SpectrumMax","3");
    
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    
    //Test some aspects of the file
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING( workspace = AnalysisDataService::Instance().retrieve(output_ws) );
    TS_ASSERT( workspace.get() );

    MatrixWorkspace_sptr matrix_ws = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
    TS_ASSERT( matrix_ws.get() );

	  //Testing the number of histograms
    TS_ASSERT_EQUALS(matrix_ws->getNumberHistograms(),3);

    //Test history
    const std::vector<AlgorithmHistory>& alghist = matrix_ws->getHistory().getAlgorithmHistories();
    int nalgs = static_cast<int>(alghist.size());
    TS_ASSERT_EQUALS(nalgs, 4);
      
    if( nalgs == 4 )
    {
      TS_ASSERT_EQUALS(alghist[0].name(), "LoadRaw");
      TS_ASSERT_EQUALS(alghist[1].name(), "AlignDetectors");
      TS_ASSERT_EQUALS(alghist[2].name(), "DiffractionFocussing");
      TS_ASSERT_EQUALS(alghist[3].name(), "LoadNexusProcessed");
    }
    
    boost::shared_ptr<Mantid::Geometry::Instrument> inst = matrix_ws->getBaseInstrument(); 
    TS_ASSERT_EQUALS(inst->getName(), "GEM");
    TS_ASSERT_EQUALS(inst->getSource()->getPos().Z(), -17);

   }


   // Saving and reading masking correctly
   void testMasked()
   {
     LoadNexusProcessed alg;

     TS_ASSERT_THROWS_NOTHING(alg.initialize());
     TS_ASSERT( alg.isInitialized() );
     testFile="focussed.nxs";
     alg.setPropertyValue("Filename", testFile);
     testFile = alg.getPropertyValue("Filename");

     alg.setPropertyValue("OutputWorkspace", output_ws);

     TS_ASSERT_THROWS_NOTHING(alg.execute());

     //Test some aspects of the file
     MatrixWorkspace_sptr workspace;
     workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(output_ws) );
     TS_ASSERT( workspace.get() );

     for(int si = 0; si < workspace->getNumberHistograms(); ++si)
     {
       workspace->maskBin(si,0,1.0);
       workspace->maskBin(si,1,1.0);
       workspace->maskBin(si,2,1.0);
     }

     SaveNexusProcessed save;
     save.initialize();
     save.setPropertyValue("InputWorkspace",output_ws);
     std::string filename = "LoadNexusProcessed_tmp.nxs";
     save.setPropertyValue("Filename",filename);
     filename = save.getPropertyValue("Filename");
     save.execute();
     LoadNexusProcessed load;
     load.initialize();
     load.setPropertyValue("Filename",filename);
     load.setPropertyValue("OutputWorkspace",output_ws);
     load.execute();

     workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(output_ws) );
     TS_ASSERT( workspace.get() );

     TS_ASSERT_EQUALS(workspace->getNumberHistograms(),6);

     TS_ASSERT(workspace->hasMaskedBins(0));
     TS_ASSERT(workspace->hasMaskedBins(1));
     TS_ASSERT(workspace->hasMaskedBins(2));
     TS_ASSERT(workspace->hasMaskedBins(3));
     TS_ASSERT(workspace->hasMaskedBins(4));
     TS_ASSERT(workspace->hasMaskedBins(5));

     if( Poco::File(filename).exists() )
       Poco::File(filename).remove();
   }




   void dotest_LoadAnEventFile(EventType type)
   {
     std::string filename_root = "LoadNexusProcessed_ExecEvent_";

     // Call a function that writes out the file
     std::string outputFile;
     EventWorkspace_sptr origWS = SaveNexusProcessedTest::do_testExec_EventWorkspaces(filename_root, type, outputFile, false, false);

     LoadNexusProcessed alg;
     TS_ASSERT_THROWS_NOTHING(alg.initialize());
     TS_ASSERT( alg.isInitialized() );
     alg.setPropertyValue("Filename", outputFile);
     alg.setPropertyValue("OutputWorkspace", output_ws);

     TS_ASSERT_THROWS_NOTHING(alg.execute());

     //Test some aspects of the file
     Workspace_sptr workspace;
     TS_ASSERT_THROWS_NOTHING( workspace = AnalysisDataService::Instance().retrieve(output_ws) );
     TS_ASSERT( workspace.get() );

     EventWorkspace_sptr ws = boost::dynamic_pointer_cast<EventWorkspace>(workspace);
     TS_ASSERT( ws );
     if (!ws) return;

     //Testing the number of histograms
     TS_ASSERT_EQUALS(ws->getNumberHistograms(),5);

     for (size_t wi=0; wi < 5; wi++)
     {
       const EventList & el = ws->getEventList(wi);
       TS_ASSERT_EQUALS( el.getEventType(), type );
     }
     TS_ASSERT_EQUALS( ws->getEventList(0).getNumberEvents(), 300 );
     TS_ASSERT_EQUALS( ws->getEventList(1).getNumberEvents(), 100 );
     TS_ASSERT_EQUALS( ws->getEventList(2).getNumberEvents(), 200 );
     TS_ASSERT_EQUALS( ws->getEventList(3).getNumberEvents(), 0 );
     TS_ASSERT_EQUALS( ws->getEventList(4).getNumberEvents(), 100 );

     // Do the comparison algo to check that they really are the same
     origWS->sortAll(TOF_SORT, NULL);
     ws->sortAll(TOF_SORT, NULL);

     IAlgorithm_sptr alg2 = AlgorithmManager::Instance().createUnmanaged("CheckWorkspacesMatch");
     alg2->initialize();
     alg2->setProperty<MatrixWorkspace_sptr>("Workspace1",origWS);
     alg2->setProperty<MatrixWorkspace_sptr>("Workspace2",ws);
     alg2->setProperty<double>("Tolerance", 1e-5);
     alg2->setProperty<bool>("CheckAxes", false);
     alg2->execute();
     if (alg2->isExecuted())
     {
       TS_ASSERT (alg2->getPropertyValue("Result") == "Success!");
     }
     else
     {
       TS_ASSERT ( false );
     }
     //TS_ASSERT( equals(origWS, ws, 1e-4) );

     //Clear old file
     if( Poco::File(outputFile).exists() ) Poco::File(outputFile).remove();

   }

   void test_LoadEventNexus_TOF()
   {
     dotest_LoadAnEventFile(TOF);
   }

   void test_LoadEventNexus_WEIGHTED()
   {
     dotest_LoadAnEventFile(WEIGHTED);
   }

   void test_LoadEventNexus_WEIGHTED_NOTIME()
   {
     dotest_LoadAnEventFile(WEIGHTED_NOTIME);
   }

private:
  LoadNexusProcessed algToBeTested;
  std::string testFile, output_ws;
};

#endif /*LOADNEXUSPROCESSEDTESTRAW_H_*/
