#ifndef H_CONVERT_TO_MDEVENTS_EVENTWS
#define H_CONVERT_TO_MDEVENTS_EVENTWS
/**TODO: FOR DEPRICATION */ 
//
#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Algorithm.h" 
#include <vector>

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/PhysicalConstants.h"


#include "MantidMDEvents/MDEventWSWrapper.h"
#include "MantidMDEvents/MDEvent.h"

#include "MantidMDAlgorithms/ConvertToMDEventsWSInterface.h"
#include "MantidMDEvents/ConvToMDPreprocDet.h"
// coordinate transformation
#include "MantidMDAlgorithms/ConvertToMDEventsTransfInterface.h"
#include "MantidMDAlgorithms/ConvertToMDEventsTransfNoQ.h"
#include "MantidMDAlgorithms/ConvertToMDEventsTransfModQ.h"
#include "MantidMDAlgorithms/ConvertToMDEventsTransfQ3D.h"


namespace Mantid
{
namespace MDAlgorithms
{
/** The macrodefinitions for ConvertToMDEvents class, making the conversion from Events WS to the MD events WS
   *
   * @date 11-10-2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

        This file is part of Mantid.

        Mantid is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 3 of the License, or
        (at your option) any later version.

        Mantid is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.

        File/ change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

// Class to process event workspace by direct conversion:
template<ConvertToMD::QMode Q, ConvertToMD::AnalMode MODE, ConvertToMD::CnvrtUnits CONV,ConvertToMD::SampleType Sample>
class ConvertToMDEventsWS<ConvertToMD::EventWSType,Q,MODE,CONV,Sample>: public ConvertToMDEventsWSBase 
{
    /// shalow class which is invoked from processQND procedure and describes the transformation from workspace coordinates to target coordinates
    /// presumably will be completely inlined
     // the instantiation of the class which does the transformation itself
     CoordTransformer<Q,MODE,CONV,ConvertToMD::Centered,Sample> trn; 
     // the pointer to underlying event workspace
     DataObjects::EventWorkspace *pEventWS;
     // vector to keep generic part of event coordinates
     std::vector<coord_t> Coord;
 public:
    size_t  setUPConversion(const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper)
    {
        size_t numSpec=ConvertToMDEventsWSBase::setUPConversion(WSD,inWSWrapper);

        // initiate the templated class which does the conversion of workspace data into MD WS coordinates;
        trn.setUpTransf(this); 
        // allocate space for single MDEvent coordinates with common coordinates which would propagate everywhere
        Coord.resize(this->n_dims);

        // hack
        pEventWS  = dynamic_cast<DataObjects::EventWorkspace *>(const_cast<API::MatrixWorkspace *>(inWS2D.get()));
        if(!pEventWS){
           throw(std::logic_error(" ConvertToMDEvensEventWS should work with defined event workspace"));
         }

        return numSpec;
    }

    void runConversion(API::Progress *pProg)
    {
         // Get the box controller
        Mantid::API::BoxController_sptr bc = pWSWrapper->pWorkspace()->getBoxController();
        size_t nEventsInWS                 = pWSWrapper->pWorkspace()->getNPoints();
        size_t lastNumBoxes                = bc->getTotalNumMDBoxes();

        
        // preprocessed detectors insure that each detector has its own spectra
        size_t nValidSpectra  = this->pDetLoc->nDetectors();

       // if any property dimension is outside of the data range requested, the job is done;
        if(!trn.calcGenericVariables(Coord,this->n_dims))return; 

       
        size_t n_added_events  = 0;
        for (size_t wi=0; wi <nValidSpectra; wi++)
        {     
           size_t iSpec         = this->pDetLoc->getDetSpectra(wi);       
           size_t  eventsAdded  = this->conversionChunk(iSpec);
           n_added_events+=eventsAdded;
           nEventsInWS   +=eventsAdded;

      // Give this task to the scheduler
         //%double cost = double(el.getNumberEvents());
         //ts->push( new FunctionTask( func, cost) );

          // Keep a running total of how many events we've added
          if (bc->shouldSplitBoxes(n_added_events,nEventsInWS,lastNumBoxes))
          {
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
    private:
     //
      virtual size_t conversionChunk(size_t workspaceIndex)
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
      };
  
   template <class T>
   size_t convertEventList(size_t workspaceIndex)
   {

         Mantid::DataObjects::EventList & el = this->pEventWS->getEventList(workspaceIndex);
         size_t numEvents     = el.getNumberEvents();    
         size_t  detNum       = this->pDetLoc->getWSDet(workspaceIndex);
         uint32_t detID       = this->pDetLoc->getDetID(detNum);
         uint16_t runIndexLoc = this->runIndex;

         std::vector<coord_t>locCoord(this->Coord);
         // set up unit conversion and calculate up all coordinates, which depend on spectra index only
         if(!trn.calcYDepCoordinates(locCoord,detNum))return 0;   // skip if any y outsize of the range of interest;
//
//       // allocate temporary buffers for MD Events data
//       // MD events coordinates buffer
         std::vector<coord_t>  allCoord;
         std::vector<float>    sig_err;       // array for signal and error. 
         std::vector<uint16_t> run_index;       // Buffer run index for each event 
         std::vector<uint32_t> det_ids;         // Buffer of det Id-s for each event

         allCoord.reserve(this->n_dims*numEvents);     sig_err.reserve(2*numEvents);
         run_index.reserve(numEvents);                 det_ids.reserve(numEvents);
    
      // This little dance makes the getting vector of events more general (since you can't overload by return type).
        typename std::vector<T> * events_ptr;
        getEventsFrom(el, events_ptr);
        typename std::vector<T> & events = *events_ptr;

        // Iterators to start/end
       typename std::vector<T>::iterator it = events.begin();
       typename std::vector<T>::iterator it_end = events.end();

       for (; it != it_end; it++)
       {
         double tof=it->tof();
         if(!trn.convertAndCalcMatrixCoord(tof,locCoord))continue; // skip ND outside the range

         sig_err.push_back(float(it->weight()));
         sig_err.push_back(float(it->errorSquared()));
         run_index.push_back(runIndexLoc);
         det_ids.push_back(detID);
         allCoord.insert(allCoord.end(),locCoord.begin(),locCoord.end());
       }

//      // Clear out the EventList to save memory
//      if (ClearInputWorkspace)
//      {
//        // Track how much memory you cleared
//        size_t memoryCleared = el.getMemorySize();
//        // Clear it now
//        el.clear();
//        // For Linux with tcmalloc, make sure memory goes back, if you've cleared 200 Megs
//        MemoryManager::Instance().releaseFreeMemoryIfAccumulated(memoryCleared, (size_t)2e8);
//      }
//
      // Add them to the MDEW
       size_t n_added_events = run_index.size();
#ifndef _DEBUG_EXCLUDE_ADD_TO_WORKSPACE
       pWSWrapper->addMDData(sig_err,run_index,det_ids,allCoord,n_added_events);
#endif
       return n_added_events;
   }

};

} // endNamespace MDAlgorithms
} // endNamespace Mantid

#endif
