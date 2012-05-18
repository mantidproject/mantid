#ifndef EXTRACTMASKINGTEST_H_
#define EXTRACTMASKINGTEST_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/ExtractMask.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidGeometry/IDetector.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::Algorithms::ExtractMask;
using Mantid::Kernel::Property;
using Mantid::MantidVecPtr;
using Mantid::detid_t;
using Mantid::specid_t;

class ExtractMaskTest : public CxxTest::TestSuite
{

public:
  
  void test_Init_Gives_An_Input_And_An_Output_Workspace_Property()
  {
    ExtractMask maskExtractor;
    maskExtractor.initialize();
    std::vector<Property*> properties = maskExtractor.getProperties();
    TS_ASSERT_EQUALS(properties.size(), 3);
    if( properties.size() == 3 )
    {
      TS_ASSERT_EQUALS(properties[0]->name(), "InputWorkspace");
      TS_ASSERT_EQUALS(properties[1]->name(), "OutputWorkspace");
    }
  }

  // Commenting out test because I am not sure that this is indeed the correct behaviour.
  void xtest_That_Input_Masked_Spectra_Are_Assigned_Zero_And_Remain_Masked_On_Output()
  {
    // Create a simple test workspace
    const int nvectors(50), nbins(10);
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::Create2DWorkspace(nvectors,nbins);
    // Mask every 10th spectra
    std::set<int64_t> maskedIndices;
    for( int i = 0; i < 50; i += 10 )
    {
      maskedIndices.insert(i);
    }
    // A few randoms
    maskedIndices.insert(5);
    maskedIndices.insert(23);
    maskedIndices.insert(37);
    inputWS = WorkspaceCreationHelper::maskSpectra(inputWS, maskedIndices);

    const std::string inputName("inputWS");
    AnalysisDataService::Instance().add(inputName, inputWS);
    MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(outputWS = runExtractMask(inputName));
    TS_ASSERT(outputWS);
    if( outputWS )
    {
      doTest(inputWS, outputWS);
    }

    AnalysisDataService::Instance().remove(inputName);
    AnalysisDataService::Instance().remove(outputWS->getName());
  }

private:
  
  // The input workspace should be in the analysis data service
  MatrixWorkspace_sptr runExtractMask(const std::string & inputName)
  {
    ExtractMask maskExtractor;
    maskExtractor.initialize();
    maskExtractor.setPropertyValue("InputWorkspace", inputName);
    const std::string outputName("masking");
    maskExtractor.setPropertyValue("OutputWorkspace", outputName);
    maskExtractor.setRethrows(true);
    maskExtractor.execute();

    Workspace_sptr workspace = AnalysisDataService::Instance().retrieve(outputName);
    if( workspace )
    {
      MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
      return outputWS;
    }
    else
    {
      return MatrixWorkspace_sptr();
    }
 
  }

  void doTest(MatrixWorkspace_const_sptr inputWS, MatrixWorkspace_const_sptr outputWS)
  {
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1);
    size_t nOutputHists(outputWS->getNumberHistograms());
    TS_ASSERT_EQUALS(nOutputHists, inputWS->getNumberHistograms());
    for( size_t i = 0; i < nOutputHists; ++i )
    {
      // Sizes
      TS_ASSERT_EQUALS(outputWS->readX(i).size(), 1);
      TS_ASSERT_EQUALS(outputWS->readY(i).size(), 1);
      TS_ASSERT_EQUALS(outputWS->readE(i).size(), 1);
      // Data
      double expectedValue(-1.0);
      bool outputMasked(false);
      IDetector_const_sptr inputDet, outputDet;
      try
      {
        inputDet = inputWS->getDetector(i);
        outputDet = outputWS->getDetector(i);
      }
      catch(Mantid::Kernel::Exception::NotFoundError&)
      {
        expectedValue = 1.0;
        inputDet = IDetector_sptr();
        outputDet = IDetector_sptr();
      }
      
      if( inputDet && inputDet->isMasked() )
      {
        expectedValue = 1.0;
        outputMasked = true;
      }
      else
      {
        expectedValue = 0.0;
        outputMasked = false;
      }
      
      TS_ASSERT_EQUALS(outputWS->dataY(i)[0], expectedValue);
      TS_ASSERT_EQUALS(outputWS->dataE(i)[0], expectedValue);
      TS_ASSERT_EQUALS(outputWS->dataX(i)[0], 0.0);
      if( inputDet )
      {
        TS_ASSERT_EQUALS(outputDet->isMasked(), outputMasked);
      }      
    }
    
  }

  //------------------------------------------------------------------------------
  // Test for workspace with grouped detectors
  /*
    * Generate a Workspace which can be (1) EventWorkspace, (2) Workspace2D, and (3) SpecialWorkspace2D
    * with grouped detectors
    */
   void setUpWSwGroupedDetectors(std::set<size_t> maskwsindexList, const std::string & name = "testSpace")
   {
     // 1. Instrument
     // a) By this will create an instrument with 9 detectors
     Instrument_sptr instr = boost::dynamic_pointer_cast<Instrument>(ComponentCreationHelper::createTestInstrumentCylindrical(1, false));
     // b) Detectors ID
     for (detid_t i = 10; i <= 36; ++i)
     {
       Detector *d = new Detector("det", i, 0);
       instr->markAsDetector(d);
     }

     // 2. Workspace
     MatrixWorkspace_sptr space;
     space = WorkspaceFactory::Instance().create("EventWorkspace",9, 6, 5);
     EventWorkspace_sptr spaceEvent = boost::dynamic_pointer_cast<EventWorkspace>(space);

     // b) Set by each spectrum
     MantidVecPtr x,vec;
     vec.access().resize(5,1.0);
     detid_t currdetid = 1;
     for (int j = 0; j < 9; ++j)
     {
       //Just one event per pixel
       for (int k = 0; k < 4; ++ k)
       {
         TofEvent event(1.23*(1.+double(k)*0.01), int64_t(4.56));
         spaceEvent->getEventList(j).addEventQuickly(event);
       }
       // spaceEvent->getEventList(j).setDetectorID(j);
       spaceEvent->getAxis(1)->spectraNo(j) = j;
       Mantid::API::ISpectrum* spec = spaceEvent->getSpectrum(j);
       std::vector<int> detids;
       for (size_t k = 0; k < 4; ++k)
       {
         detids.push_back(currdetid);
         currdetid ++;
       }
       spec->addDetectorIDs(detids);
     }
     spaceEvent->doneAddingEventLists();
     x.access().push_back(0.0);
     x.access().push_back(10.0);
     spaceEvent->setAllX(x);

     space->setInstrument(instr);
     space->generateSpectraMap();

     // 3. Mask some spectra
     for (std::set<size_t>::iterator wsiter = maskwsindexList.begin(); wsiter != maskwsindexList.end(); ++ wsiter)
     {
       size_t wsindex = *wsiter;
       space->maskWorkspaceIndex(wsindex);
     }

     // 4. Register the workspace in the data service
     AnalysisDataService::Instance().addOrReplace(name, space);

   }

public:
   /* Test extract mask from workspace w/ grouped detectors
    *
    */
   void test_OnGroupedDetectors()
   {
     // 1. Generate input workspace
     std::set<size_t> maskwsindexList;
     maskwsindexList.insert(1);
     maskwsindexList.insert(3);
     maskwsindexList.insert(6);
     std::string wsname("TestGroupedDetectorsWS");
     setUpWSwGroupedDetectors(maskwsindexList, wsname);
     DataObjects::EventWorkspace_sptr eventWS =
       AnalysisDataService::Instance().retrieveWS<DataObjects::EventWorkspace>(wsname);
     TS_ASSERT(eventWS);

     // 2. Extract mask
     ExtractMask maskExtractor;
     maskExtractor.initialize();
     maskExtractor.setPropertyValue("InputWorkspace", wsname);
     const std::string outputName("masking");
     maskExtractor.setPropertyValue("OutputWorkspace", outputName);
     maskExtractor.setRethrows(true);
     TS_ASSERT_THROWS_NOTHING(maskExtractor.execute());
     TS_ASSERT(maskExtractor.isExecuted());

     Workspace_sptr workspace = AnalysisDataService::Instance().retrieve(outputName);
     TS_ASSERT(workspace);
     if (!workspace)
     {
       return;
     }

     // 3. Check type of output workspace
     DataObjects::MaskWorkspace_sptr maskws = boost::dynamic_pointer_cast<DataObjects::MaskWorkspace>(workspace);
     TS_ASSERT(!maskws);
     DataObjects::Workspace2D_sptr ws2d = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(workspace);
     TS_ASSERT(ws2d);

     // 4. Check values of output workspace
     TS_ASSERT_EQUALS(ws2d->getNumberHistograms(), 9);

     for (size_t iws = 0; iws < ws2d->getNumberHistograms(); ++iws)
     {
       double value = ws2d->readY(iws)[0];
       if (iws == 1 || iws == 3 || iws == 6)
       {
         TS_ASSERT(value > 0.5);
       }
       else
       {
         TS_ASSERT(value < 0.5);
       }
     }

     // 5. Check detectors
     Geometry::Instrument_const_sptr instrument = ws2d->getInstrument();
     TS_ASSERT(instrument);

     for (size_t iws = 0; iws < ws2d->getNumberHistograms(); ++iws)
     {
       API::ISpectrum *spec = ws2d->getSpectrum(iws);
       std::set<int> detids = spec->getDetectorIDs();
       for (std::set<int>::iterator it = detids.begin(); it != detids.end(); ++it)
       {
         detid_t detid = detid_t(*it);
         Geometry::IDetector_const_sptr tmpdet = instrument->getDetector(detid);
         if (iws ==  1 || iws == 3 || iws == 6)
         {
           TS_ASSERT_EQUALS(tmpdet->isMasked(), true);
         }
         else
         {
           TS_ASSERT_EQUALS(tmpdet->isMasked(), false);
         }
       }
     }

     return;
   }

};


#endif //EXTRACTMASKINGTEST_H_
