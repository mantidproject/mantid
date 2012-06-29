#include "MantidMDEvents/ConvToMDEventsWS.h"
#include "MantidMDEvents/UnitsConversionHelper.h"
//

namespace Mantid
{
namespace MDEvents
{
/**function converts particular list of events of type T into MD workspace space and adds these events to the workspace itself  */
template <class T>
size_t ConvToMDEventsWS::convertEventList(size_t workspaceIndex)
{

         const Mantid::DataObjects::EventList & el = this->pEventWS->getEventList(workspaceIndex);

         // create local unit conversion class
         UnitsConversionHelper localUnitConv(UnitConversion);

         size_t numEvents     = el.getNumberEvents();      
         uint32_t detID       = this->pDetLoc->getDetID(workspaceIndex);
         uint16_t runIndexLoc = this->runIndex;

         std::vector<coord_t>locCoord(this->Coord);
         // set up unit conversion and calculate up all coordinates, which depend on spectra index only
        if(!pQConverter->calcYDepCoordinates(locCoord,workspaceIndex))return 0;   // skip if any y outsize of the range of interest;
        localUnitConv.updateConversion(workspaceIndex);
//
        // allocate temporary buffers for MD Events data
         // MD events coordinates buffer
         std::vector<coord_t>  allCoord;
         std::vector<float>    sig_err;       // array for signal and error. 
         std::vector<uint16_t> run_index;     // Buffer for run index for each event 
         std::vector<uint32_t> det_ids;       // Buffer of det Id-s for each event

         allCoord.reserve(this->n_dims*numEvents);     sig_err.reserve(2*numEvents);
         run_index.reserve(numEvents);                 det_ids.reserve(numEvents);
    
      // This little dance makes the getting vector of events more general (since you can't overload by return type).
        typename std::vector<T>const * events_ptr;
        getEventsFrom(el, events_ptr);
        const typename std::vector<T> & events = *events_ptr;



        // Iterators to start/end
       typename std::vector<T>::const_iterator it = events.begin();
       typename std::vector<T>::const_iterator it_end = events.end();
     
    
       it = events.begin();
       for (; it != it_end; it++)
       {
         double val=localUnitConv.convertUnits(it->tof());         
         if(!pQConverter->calcMatrixCoord(val,locCoord))continue; // skip ND outside the range


         sig_err.push_back(float(it->weight()));
         sig_err.push_back(float(it->errorSquared()));
         run_index.push_back(runIndexLoc);
         det_ids.push_back(detID);
         allCoord.insert(allCoord.end(),locCoord.begin(),locCoord.end());
       }

      // Add them to the MDEW
       size_t n_added_events = run_index.size();
       pWSWrapper->addMDData(sig_err,run_index,det_ids,allCoord,n_added_events);
       return n_added_events;
}

/** The method runs conversion for a single event list, corresponding to a particular workspace index */
size_t ConvToMDEventsWS::conversionChunk(size_t workspaceIndex)
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


/** method sets up all internal variables necessary to convert from Event Workspace to MDEvent workspace 
 @parameter WSD         -- the class describing the target MD workspace, sorurce Event workspace and the transformations, necessary to perform on these workspaces
 @parameter inWSWrapper -- the class wrapping the target MD workspace
*/
size_t  ConvToMDEventsWS::initialize(const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper)
{
    size_t numSpec=ConvToMDBase::initialize(WSD,inWSWrapper);

    
    pEventWS  = boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(inWS2D);
    if(!pEventWS.get()){
           throw(std::logic_error(" ConvertToMDEvensEventWS should work with defined event workspace"));
    }
 
    return numSpec;
}

void ConvToMDEventsWS::runConversion(API::Progress *pProg)
{
       // Get the box controller
        Mantid::API::BoxController_sptr bc = pWSWrapper->pWorkspace()->getBoxController();
        size_t lastNumBoxes = bc->getTotalNumMDBoxes();
        size_t nEventsInWS  = pWSWrapper->pWorkspace()->getNPoints();
         // Is the access to input events thread-safe?
       // bool MultiThreadedAdding = pEventWS->threadSafe();
        // preprocessed detectors insure that each detector has its own spectra
        size_t nValidSpectra  = this->pDetLoc->nDetectors();

        // Create the thread pool that will run all of these.
        Kernel::ThreadScheduler * ts = new Kernel::ThreadSchedulerFIFO();
        // initiate thread pool with number of machine's cores (0 in tp constructor)
        //Kernel::ThreadScheduler * ts = NULL;       
        pProg->resetNumSteps(nValidSpectra,0,1);
        Kernel::ThreadPool tp(ts, 0, new API::Progress(*pProg));
        

       // if any property dimension is outside of the data range requested, the job is done;
        if(!pQConverter->calcGenericVariables(Coord,this->n_dims))return; 
       
        size_t eventsAdded  = 0;
        for (size_t wi=0; wi <nValidSpectra; wi++)
        {     

           size_t nConverted   =  this->conversionChunk(wi);
           eventsAdded         += nConverted;
           nEventsInWS         += nConverted;
      // Give this task to the scheduler
         //%double cost = double(el.getNumberEvents());
         //ts->push( new FunctionTask( func, cost) );

          // Keep a running total of how many events we've added
           if (bc->shouldSplitBoxes(nEventsInWS,eventsAdded, lastNumBoxes)){
              // Do all the adding tasks
              tp.joinAll();    
              // Now do all the splitting tasks
              pWSWrapper->pWorkspace()->splitAllIfNeeded(ts);
              if (ts->size() > 0)  tp.joinAll();

            // Count the new # of boxes.
              lastNumBoxes = pWSWrapper->pWorkspace()->getBoxController()->getTotalNumMDBoxes();
              pProg->report(wi);
           }
   
       }
    tp.joinAll();
    // Do a final splitting of everything 
    pWSWrapper->pWorkspace()->splitAllIfNeeded(ts);
    tp.joinAll();
    // Recount totals at the end.
    pWSWrapper->pWorkspace()->refreshCache(); 
    pWSWrapper->refreshCentroid();
    pProg->report();
}

  

} // endNamespace MDEvents
} // endNamespace Mantid


