#include "MantidMDEvents/ConvToMDEventsEvents.h"
//

namespace Mantid
{
namespace MDEvents
{

size_t  ConvToMDEventsEvents::initialize(Mantid::API::MatrixWorkspace_sptr pWS2D, ConvToMDPreprocDet &detLoc,
                          const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper)
{
    size_t numSpec=ConvToMDEventsBase::initialize(pWS2D,detLoc,WSD,inWSWrapper);

    
    pEventWS  = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(pWS2D);
    if(!pEventWS.get()){
           throw(std::logic_error(" ConvertToMDEvensEventWS should work with defined event workspace"));
    }

    pQConverter->initialize(*this);

    return numSpec;
}

void ConvToMDEventsEvents::runConversion(API::Progress *pProg)
{
       // Get the box controller
        Mantid::API::BoxController_sptr bc = pWSWrapper->pWorkspace()->getBoxController();
        size_t lastNumBoxes = bc->getTotalNumMDBoxes();
        
        // preprocessed detectors insure that each detector has its own spectra
        size_t nValidSpectra  = this->pDetLoc->nDetectors();

       // if any property dimension is outside of the data range requested, the job is done;
        if(!pQConverter->calcGenericVariables(Coord,this->n_dims))return; 
       
        size_t eventsAdded  = 0;
        for (size_t wi=0; wi <nValidSpectra; wi++)
        {     
           size_t iSpec         = this->pDetLoc->getDetSpectra(wi);       
           UnitConversion.updateConversion(wi);

           eventsAdded         += this->conversionChunk(iSpec);
      // Give this task to the scheduler
         //%double cost = double(el.getNumberEvents());
         //ts->push( new FunctionTask( func, cost) );

          // Keep a running total of how many events we've added
           if (bc->shouldSplitBoxes(eventsAdded, lastNumBoxes)){
            // Do all the adding tasks
            //   tp.joinAll();    
            // Now do all the splitting tasks
              //ws->splitAllIfNeeded(ts);
               pWSWrapper->pWorkspace()->splitAllIfNeeded(NULL);
             //if (ts->size() > 0)       tp.joinAll();

            // Count the new # of boxes.
              lastNumBoxes = pWSWrapper->pWorkspace()->getBoxController()->getTotalNumMDBoxes();
              pProg->report(wi);
           }
   
       }
    //tp.joinAll();
    // Do a final splitting of everything
    //ws->splitAllIfNeeded(ts);
    //tp.joinAll();
    pWSWrapper->pWorkspace()->splitAllIfNeeded(NULL);
    // Recount totals at the end.
    pWSWrapper->pWorkspace()->refreshCache(); 
    pWSWrapper->refreshCentroid();
    pProg->report();
}

//
size_t ConvToMDEventsEvents::conversionChunk(size_t workspaceIndex)
{       

       switch (this->pEventWS->getEventList(workspaceIndex).getEventType())
        {
        case Mantid::API::TOF:
            return this->convertEventList<Mantid::DataObjects::TofEvent>(workspaceIndex);
         case Mantid::API::WEIGHTED:
          return  this->convertEventList<Mantid::DataObjects::WeightedEvent>(workspaceIndex);
        case Mantid::API::WEIGHTED_NOTIME:
          return this->convertEventList<Mantid::DataObjects::WeightedEventNoTime>(workspaceIndex);
        default:
           throw std::runtime_error("EventList had an unexpected data type!");
        }
}
  

} // endNamespace MDEvents
} // endNamespace Mantid


