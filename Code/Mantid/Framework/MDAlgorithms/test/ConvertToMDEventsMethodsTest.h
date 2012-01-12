#ifndef CONVERT2_MDEVENTS_METHODS_TEST_H_
#define CONVERT2_MDEVENTS_METHODS_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/ConvertToMDEventsUnitsConv.h"
#include "MantidMDAlgorithms/ConvertToMDEventsDetInfo.h"
#include "MantidMDAlgorithms/ConvertToMDEvents.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/Progress.h"


#include "MantidMDAlgorithms/ConvertToMDEventsHistoWS.h"
#include "MantidMDAlgorithms/ConvertToMDEventsEventWS.h"
#include "MantidMDEvents/MDBoxIterator.h"

// stuff for convertToEventWorkspace subalgorithm
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
//
#include "MantidMDAlgorithms/BinaryOperationMD.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/IMDBox.h"





using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::MDEvents;
using namespace Mantid::MDAlgorithms;



class ConvertToMDEventsMethodsTest : public CxxTest::TestSuite, public ConvertToMDEvents
{
   Mantid::API::MatrixWorkspace_sptr ws2D;
   Mantid::API::MatrixWorkspace_sptr ws_events;
   std::auto_ptr<API::Progress > pProg;   

   boost::shared_ptr<MDEvents::MDEventWSWrapper> pHistoMDWSWrapper;
   boost::shared_ptr<MDEvents::MDEventWSWrapper> pEventMDWSWrapper;
   PreprocessedDetectors det_loc;
   MDEvents::MDWSDescription TestWS;

public:
static ConvertToMDEventsMethodsTest *createSuite() {
    return new ConvertToMDEventsMethodsTest();    
}
static void destroySuite(ConvertToMDEventsMethodsTest  * suite) { delete suite; }    

void test_TwoTransfMethods()
{
  
    ConvertToMDEvensHistoWS<Q3D,Direct,ConvertNo> HistoConv;
    
    TestWS.Ei   = *(dynamic_cast<Kernel::PropertyWithValue<double>  *>(ws2D->run().getProperty("Ei")));
    TestWS.emode= MDAlgorithms::Direct;
    TestWS.dim_min.assign(4,-3);
    TestWS.dim_max.assign(4,3);
    TestWS.dim_names.assign(4,"Momentum");
    TestWS.dim_names[3]="DeltaE";
    TestWS.rotMatrix.assign(9,0);
    TestWS.rotMatrix[0]=1;
    TestWS.rotMatrix[4]=1;
    TestWS.rotMatrix[8]=1;

     pHistoMDWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());
     pHistoMDWSWrapper->createEmptyMDWS(TestWS);

    Mantid::API::BoxController_sptr bc=pHistoMDWSWrapper->pWorkspace()->getBoxController();
    bc->setSplitThreshold(5);
    bc->setMaxDepth(100);
    bc->setSplitInto(5);

    TS_ASSERT_THROWS_NOTHING(HistoConv.setUPConversion(ws2D,det_loc,TestWS,pHistoMDWSWrapper));

    pProg =  std::auto_ptr<API::Progress >(new API::Progress(dynamic_cast<ConvertToMDEvents *>(this),0.0,1.0,4));
    TS_ASSERT_THROWS_NOTHING(HistoConv.runConversion(pProg.get()));

    TS_ASSERT_EQUALS(50,pHistoMDWSWrapper->pWorkspace()->getNPoints());

}
void test_buildFromEWS()
{
    // create empty target ws
     pEventMDWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());
     pEventMDWSWrapper->createEmptyMDWS(TestWS);
     // set up conversion just to deliver proper pointers to tof convertor
     ConvertToMDEvensHistoWS<Q3D,Direct,ConvByTOF> tmp;
     tmp.setUPConversion(ws2D,det_loc,TestWS,pEventMDWSWrapper);

     // provide arguments for convertor
     DataObjects::Workspace2D_const_sptr inWS = boost::static_pointer_cast<const DataObjects::Workspace2D>(ws2D);
     EventWorkspace_sptr outWS = convertToEvents(inWS,tmp);

     ws_events =boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(outWS);
     if (!ws_events){
          throw std::runtime_error("Error in ConvertToEventWorkspace. Cannot proceed.");
     }



     ConvertToMDEvensEventWS<Q3D,Direct> TOFConv;

  
     Mantid::API::BoxController_sptr bc=pEventMDWSWrapper->pWorkspace()->getBoxController();
     bc->setSplitThreshold(5);
     bc->setMaxDepth(100);
     bc->setSplitInto(5);

     TS_ASSERT_THROWS_NOTHING(TOFConv.setUPConversion(ws_events,det_loc,TestWS,pEventMDWSWrapper));

     pProg =  std::auto_ptr<API::Progress >(new API::Progress(dynamic_cast<ConvertToMDEvents *>(this),0.0,1.0,4));
     TS_ASSERT_THROWS_NOTHING(TOFConv.runConversion(pProg.get()));
     TS_ASSERT_EQUALS(50,pEventMDWSWrapper->pWorkspace()->getNPoints());


}

void test_compareTwoBuilds()
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
    // Get all the MDBoxes contained
    //IMDBox<MDE,nd> * parentBox = ws->getBox();
    //std::vector<IMDBox<MDE,nd> *> boxes;
    //parentBox->getBoxes(boxes, 1000, true);

    IMDBox<MDEvent<4> ,4> * parentBox = pMatrWs->getBox();
    std::vector<IMDBox<MDEvent<4>,4> *> boxesM;
    parentBox->getBoxes(boxesM, 1000, true);

                            parentBox = pEvntWs->getBox();
    std::vector<IMDBox<MDEvent<4>,4> *> boxesE;
    parentBox->getBoxes(boxesE, 1000, true);


    for (size_t i=0; i<boxesM.size(); i++){
        MDBox<MDEvent<4>,4> * boxM = dynamic_cast<MDBox<MDEvent<4>,4> *>(boxesM[i]);
        if (boxM){
            MDBox<MDEvent<4>,4> * boxE = dynamic_cast<MDBox<MDEvent<4>,4> *>(boxesE[i]);

          std::vector<MDEvent<4> > & eventsM = boxM->getEvents();
          std::vector<MDEvent<4> > & eventsE = boxE->getEvents();
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
            itE++;
          }
          boxE->releaseEvents();
          boxM->releaseEvents();
       }
    }

}

// constructor:
ConvertToMDEventsMethodsTest ():TestWS(4){    

   std::vector<double> L2(5,5);
   std::vector<double> polar(5,(30./180.)*3.1415926);
   polar[0]=0;
   std::vector<double> azimutal(5,0);
   azimutal[1]=(45./180.)*3.1415936;
   azimutal[2]=(90./180.)*3.1415936;
   azimutal[3]=(135./180.)*3.1415936;
   azimutal[4]=(180./180.)*3.1415936;

   int numBins=10;
   ws2D =WorkspaceCreationHelper::createProcessedInelasticWS(L2, polar, azimutal,numBins,-1,3,3);

   Kernel::UnitFactory::Instance().create("TOF");
   Kernel::UnitFactory::Instance().create("Energy");
   Kernel::UnitFactory::Instance().create("DeltaE");
   Kernel::UnitFactory::Instance().create("Momentum");

   // set up workpspaces and preprocess detectors
    pProg =  std::auto_ptr<API::Progress >(new API::Progress(dynamic_cast<ConvertToMDEvents *>(this),0.0,1.0,4));

    processDetectorsPositions(ws2D,det_loc,ConvertToMDEvents::getLogger(),pProg.get());
    //pConvMethods = std::auto_ptr<ConvertToMDEventsCoordTestHelper>(new ConvertToMDEventsCoordTestHelper());
    //pConvMethods->setUPConversion(ws2D,det_loc);
  
}
// function repeats convert to events algorithm which for some myteruious reasons do not work here
EventWorkspace_sptr convertToEvents(DataObjects::Workspace2D_const_sptr inWS,const IConvertToMDEventsMethods &conv,bool GenerateMultipleEvents=false,int MaxEventsPerBin=10){
  
    // set up conversion to Time of flight
    UNITS_CONVERSION<ConvByTOF,Centered> TOFCONV;
    TOFCONV.setUpConversion(&conv,"TOF");

    //Create the event workspace
    EventWorkspace_sptr  outWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", inWS->getNumberHistograms(), inWS->blocksize()+1, inWS->blocksize()));

    //Copy geometry, etc. over.
    API::WorkspaceFactory::Instance().initializeFromParent(inWS, outWS, false);

    // Cached values for later checks
    double inf = std::numeric_limits<double>::infinity();
    double ninf = -inf;

    Progress prog(this, 0.0, 1.0, inWS->getNumberHistograms());
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
            if (GenerateMultipleEvents)
            {
              // --------- Multiple events per bin ----------
              double errorSquared = error * error;
              // Find how many events to fake
              double val = weight / E[i];
              val *= val;
              // Convert to int with slight rounding up. This is to avoid rounding errors
              int numEvents = int(val + 0.2);
              if (numEvents < 1) numEvents = 1;
              if (numEvents > MaxEventsPerBin) numEvents = MaxEventsPerBin;
              // Scale the weight and error for each
              weight /= numEvents;
              errorSquared /= numEvents;

              // Spread the TOF. e.g. 2 events = 0.25, 0.75.
              double tofStep = (X[i+1] - X[i]) / (numEvents);
              for (size_t j=0; j<size_t(numEvents); j++)
              {
                double tof = X[i] + tofStep * (0.5 + double(j));
                // Create and add the event

                el.addEventQuickly( WeightedEventNoTime(TOFCONV.getXConverted(tof), weight, errorSquared) );
              }
            }
            else
            {
              // --------- Single event per bin ----------
              // TOF = midpoint of the bin
              double tof = (X[i] + X[i+1]) / 2.0;
              // Error squared is carried in the event
              double errorSquared = E[i];
              errorSquared *= errorSquared;
              // Create and add the event
              el.addEventQuickly( WeightedEventNoTime(TOFCONV.getXConverted(tof), weight, errorSquared) );
            }
          } // error is nont NAN or infinite
        } // weight is non-zero, not NAN, and non-infinite
      } // (each bin)

      // Set the X binning parameters
      el.setX( inSpec->ptrX() );
      // Manually set that this is sorted by TOF, since it is. This will make it "threadSafe" in other algos.
      el.setSortOrder( TOF_SORT );

      prog.report("Converting");
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