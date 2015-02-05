#ifndef CONVERT2_MDEVENTS_METHODS_TEST_H_
#define CONVERT2_MDEVENTS_METHODS_TEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/FrameworkManager.h"

#include "MantidMDEvents/MDBoxIterator.h"

// stuff for convertToEventWorkspace ChildAlgorithm
#include "MantidDataObjects/Events.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"

//
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDBoxBase.h"
#include "MantidMDEvents/ConvToMDBase.h"
#include "MantidMDEvents/ConvToMDSelector.h"



using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::MDEvents;



class ConvToMDEventsVSHistoTest : public CxxTest::TestSuite
{
  // matrix ws and event ws which contains the same data
   Mantid::API::MatrixWorkspace_sptr ws2D;
   Mantid::API::MatrixWorkspace_sptr ws_events;

   // MD ws obtained from histo and MD ws obtained from events, which should be again similar
   boost::shared_ptr<MDEvents::MDEventWSWrapper> pHistoMDWSWrapper;
   boost::shared_ptr<MDEvents::MDEventWSWrapper> pEventMDWSWrapper;

   // preprocessed detectors positions and target ws description
   DataObjects::TableWorkspace_sptr detLoc;
   MDEvents::MDWSDescription TestWS;

   std::auto_ptr<ConvToMDBase> pConvMethods;

   // class which would select the solver as function of ws type
   ConvToMDSelector WSAlgoSelector;

   // the helper claa which woudl provide log and progress --> algorithm's properties 
   WorkspaceCreationHelper::MockAlgorithm logProvider;

public:
static ConvToMDEventsVSHistoTest *createSuite() {
    return new ConvToMDEventsVSHistoTest();    
}
static void destroySuite(ConvToMDEventsVSHistoTest  * suite) { delete suite; }    


void test_TwoTransfMethods()
{

    // define the parameters of the conversion
    std::vector<std::string> dimProperyNames; //--- empty property names
    TS_ASSERT_THROWS_NOTHING(TestWS.buildFromMatrixWS(ws2D,"Q3D","Direct",dimProperyNames));
    TestWS.m_PreprDetTable = detLoc;

    std::vector<double> dimMin(4,-3);
    std::vector<double> dimMax(4, 3);
    TS_ASSERT_THROWS_NOTHING(TestWS.setMinMax(dimMin,dimMax));

    // define transformation
    TestWS.m_RotMatrix.assign(9,0);
    TestWS.m_RotMatrix[0]=1;
    TestWS.m_RotMatrix[4]=1;
    TestWS.m_RotMatrix[8]=1;

    // create target md workspace
    pHistoMDWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());
    pHistoMDWSWrapper->createEmptyMDWS(TestWS);

    Mantid::API::BoxController_sptr bc=pHistoMDWSWrapper->pWorkspace()->getBoxController();
    bc->setSplitThreshold(5);
    bc->setMaxDepth(100);
    bc->setSplitInto(5);

    // initialize solver converting from Matrix ws to md ws
    boost::shared_ptr<ConvToMDBase> pSolver;
    TS_ASSERT_THROWS_NOTHING(pSolver = WSAlgoSelector.convSelector(ws2D,pSolver));
    TS_ASSERT_THROWS_NOTHING(pSolver->initialize(TestWS,pHistoMDWSWrapper));

    logProvider.resetProgress(4);
    TS_ASSERT_THROWS_NOTHING(pSolver->runConversion(logProvider.getProgress()));

    TS_ASSERT_EQUALS(50,pHistoMDWSWrapper->pWorkspace()->getNPoints());

}
void test_buildFromEWS()
{
    // create empty target ws
     pEventMDWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());
     pEventMDWSWrapper->createEmptyMDWS(TestWS);
     // convert initial matrix ws into event ws
     DataObjects::Workspace2D_const_sptr inWS = boost::static_pointer_cast<const DataObjects::Workspace2D>(ws2D);
     EventWorkspace_sptr outWS = convertToEvents(inWS);

     // build ws description from event ws
     std::vector<std::string> dimProperyNames; //--- empty property names
     TS_ASSERT_THROWS_NOTHING(TestWS.buildFromMatrixWS(outWS,"Q3D","Direct",dimProperyNames));
     TestWS.m_PreprDetTable = detLoc;

     ws_events =boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(outWS);
     if (!ws_events){
          throw std::runtime_error("Error in ConvertToEventWorkspace. Cannot proceed.");
     }

    // create target md workspace wrapper
    pEventMDWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());
    pEventMDWSWrapper->createEmptyMDWS(TestWS);

     Mantid::API::BoxController_sptr bc=pEventMDWSWrapper->pWorkspace()->getBoxController();
     bc->setSplitThreshold(5);
     bc->setMaxDepth(100);
     bc->setSplitInto(5);

    // initialize solver converting from Event ws to md ws
    boost::shared_ptr<ConvToMDBase> pTOFConv;
    TS_ASSERT_THROWS_NOTHING(pTOFConv = WSAlgoSelector.convSelector(outWS));
    TS_ASSERT_THROWS_NOTHING(pTOFConv->initialize(TestWS,pEventMDWSWrapper));

     logProvider.resetProgress(4);
     TS_ASSERT_THROWS_NOTHING(pTOFConv->runConversion(logProvider.getProgress()));
     TS_ASSERT_EQUALS(50,pEventMDWSWrapper->pWorkspace()->getNPoints());


}

void test_compareTwoConversions()
{

   MDEvents::MDEventWorkspace<MDEvents::MDEvent<4>,4> * pMatrWs = dynamic_cast<MDEvents::MDEventWorkspace<MDEvents::MDEvent<4>,4> *>(this->pHistoMDWSWrapper->pWorkspace().get());
   MDEvents::MDEventWorkspace<MDEvents::MDEvent<4>,4> * pEvntWs = dynamic_cast<MDEvents::MDEventWorkspace<MDEvents::MDEvent<4>,4> *>(this->pEventMDWSWrapper->pWorkspace().get());
   if(!pMatrWs){
     TS_FAIL(" can not retrieve workspace obtained from matrix WS");
     return;
   }
   if(!pEvntWs){
     TS_FAIL(" can not retrieve workspace obtained from event WS");
     return;
   }
    // Get all the MDBoxes contained    
    //MDBoxBase<MDE,nd> * parentBox = ws->getBox();
    //std::vector<MDBoxBase<MDE,nd> *> boxes;
    //parentBox->getBoxes(boxes, 1000, true);

    MDBoxBase<MDEvent<4> ,4> * parentBox = pMatrWs->getBox();
    std::vector<MDBoxBase<MDEvent<4>,4> *> boxesM;
    parentBox->getBoxes(boxesM, 1000, true);

    parentBox = pEvntWs->getBox();
    std::vector<MDBoxBase<MDEvent<4>,4> *> boxesE;
    parentBox->getBoxes(boxesE, 1000, true);


    for (size_t i=0; i<boxesM.size(); i++){
        MDBox<MDEvent<4>,4> * boxM = dynamic_cast<MDBox<MDEvent<4>,4> *>(boxesM[i]);
        if (boxM){
            MDBox<MDEvent<4>,4> * boxE = dynamic_cast<MDBox<MDEvent<4>,4> *>(boxesE[i]);

          std::vector<MDEvent<4> > & eventsM = boxM->getEvents();
          std::vector<MDEvent<4> > & eventsE = boxE->getEvents();
          if(eventsM.size()!=eventsE.size())
          {
            TS_FAIL(" sizes of the boxes, obtained from matrix workspace="+boost::lexical_cast<std::string>(eventsM.size())+" and from event worskpace="+boost::lexical_cast<std::string>(eventsE.size())+" and are different");
            return;
          }


          std::vector<MDEvent<4> >::iterator itM = eventsM.begin();
          std::vector<MDEvent<4> >::iterator itE = eventsE.begin();
          std::vector<MDEvent<4> >::iterator it_end = eventsM.end();

          for (; itM != it_end; itM++){
    
            float Signal1 = itM->getSignal();
            float Signal2 = itE->getSignal();
            TS_ASSERT_DELTA(Signal1,Signal2,1.e-5);
            float Err1    = itM->getErrorSquared();
            float Err2    = itE->getErrorSquared();
            TS_ASSERT_DELTA(Err1,Err2,1.e-5);

            for(size_t j=0;i<4;i++){
                TS_ASSERT_DELTA(itM->getCenter(j),itE->getCenter(j),1.e-4);
            }
            TS_ASSERT_EQUALS(itM->getDetectorID(),itE->getDetectorID());
            TS_ASSERT_EQUALS(itM->getRunIndex(),itE->getRunIndex());
            itE++;
          }
          boxE->releaseEvents();
          boxM->releaseEvents();
       }
    }

}

// constructor:
ConvToMDEventsVSHistoTest():
TestWS(4),
logProvider(100)
{    
   API::FrameworkManager::Instance();

   std::vector<double> L2(5,5);
   std::vector<double> polar(5,(30./180.)*M_PI);
   polar[0]=0;
   std::vector<double> azimutal(5,0);
   azimutal[1]=(45./180.)*M_PI;
   azimutal[2]=(90./180.)*M_PI;
   azimutal[3]=(135./180.)*M_PI;
   azimutal[4]=(180./180.)*M_PI;

   int numBins=10;
   ws2D =WorkspaceCreationHelper::createProcessedInelasticWS(L2, polar, azimutal,numBins,-1,3,3);
   // this should disable multithreading
   ws2D->mutableRun().addProperty("NUM_THREADS",0);  

   detLoc = WorkspaceCreationHelper::buildPreprocessedDetectorsWorkspace(ws2D);
  
}
// function repeats convert to events algorithm which for some mysterious reasons do not work here as ChildAlgorithm.
EventWorkspace_sptr convertToEvents(DataObjects::Workspace2D_const_sptr inWS)
{
 
    // set up conversion to Time of flight
    UnitsConversionHelper TOFCONV;

    TOFCONV.initialize(TestWS,"TOF");

    //Create the event workspace
    EventWorkspace_sptr  outWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", inWS->getNumberHistograms(), inWS->blocksize()+1, inWS->blocksize()));

    //Copy geometry, etc. over.
    API::WorkspaceFactory::Instance().initializeFromParent(inWS, outWS, false);

    // Cached values for later checks
    double inf = std::numeric_limits<double>::infinity();
    double ninf = -inf;

    logProvider.resetProgress(inWS->getNumberHistograms());
    Progress *prog = logProvider.getProgress();
    //PARALLEL_FOR1(inWS)
    for (int iwi=0; iwi<int(inWS->getNumberHistograms()); iwi++)
    {
      //PARALLEL_START_INTERUPT_REGION
      size_t wi = size_t(iwi);
      // The input spectrum (a histogram)
      const ISpectrum * inSpec = inWS->getSpectrum(wi);
      const MantidVec & X = inSpec->readX();
      const MantidVec & Y = inSpec->readY();
      const MantidVec & E = inSpec->readE();

      TOFCONV.updateConversion(iwi);

      // The output event list
      EventList & el = outWS->getEventList(wi);
      // Copy detector IDs and spectra
      el.copyInfoFrom( *inSpec );
      // We need weights but have no way to set the time. So use weighted, no time
      el.switchTo(WEIGHTED_NOTIME);

      for (size_t i=0; i<X.size()-1; i++)
      {
        double weight = Y[i];
        if ((weight != 0.0) && (weight == weight) /*NAN check*/
            && (weight != inf) && (weight != ninf))
        {
          double error = E[i];
          // Also check that the error is not a bad number
          if ((error == error) /*NAN check*/
              && (error != inf) && (error != ninf))
          {
              // --------- Single event per bin ----------
              // TOF = midpoint of the bin
              double tof = (X[i] + X[i+1]) / 2.0;
              // Error squared is carried in the event
              double errorSquared = E[i];
              errorSquared *= errorSquared;
              // Create and add the event
              el.addEventQuickly( WeightedEventNoTime(TOFCONV.convertUnits(tof), weight, errorSquared) );

          } // error is nont NAN or infinite
        } // weight is non-zero, not NAN, and non-infinite
      } // (each bin)

      // Set the X binning parameters
      el.setX( inSpec->ptrX() );
      // Manually set that this is sorted by TOF, since it is. This will make it "threadSafe" in other algos.
      el.setSortOrder( TOF_SORT );

      prog->report("Converting");
     // PARALLEL_END_INTERUPT_REGION
    }
   // PARALLEL_CHECK_INTERUPT_REGION
     NumericAxis *pAxis0 = new NumericAxis(2); 
     pAxis0->setUnit("TOF");
     outWS->replaceAxis(0,pAxis0);

    return outWS;
    // Set the output
    //setProperty("OutputWorkspace", outWS);
}

};



#endif
