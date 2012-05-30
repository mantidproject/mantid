#ifndef H_CONVERT_TO_MDEVENTS_EVENTWS
#define H_CONVERT_TO_MDEVENTS_EVENTWS
//
#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Algorithm.h" 
#include <vector>

#include "MantidDataObjects/EventWorkspace.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/PhysicalConstants.h"

#include "MantidMDEvents/MDEventWSWrapper.h"
#include "MantidMDEvents/MDEvent.h"

#include "MantidMDEvents/ConvToMDPreprocDet.h"
#include "MantidMDEvents/ConvToMDEventsBase.h"
// coordinate transformation
#include "MantidMDEvents/MDTransfFactory.h"

namespace Mantid
{
namespace MDEvents
{
/** The class specializes ConvToMDEventsBase for the case when the conversion occurs from Events WS to the MD events WS
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

class ConvToMDEventsEvents: public ConvToMDEventsBase
{
 public:
    size_t  initialize(Mantid::API::MatrixWorkspace_sptr pWS2D,
                          const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper);
    void runConversion(API::Progress *pProg);

private:
   // function runs the conversion on 
   virtual size_t conversionChunk(size_t workspaceIndex);
   // the pointer to the source event workspace as event ws does not work through the public Matrix WS interface
    DataObjects::EventWorkspace_sptr pEventWS;
   /**function converts particular type of events into MD space and add these events to the workspace itself 
    */
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
        if(!pQConverter->calcYDepCoordinates(locCoord,detNum))return 0;   // skip if any y outsize of the range of interest;
        UnitConversion.updateConversion(detNum);
//
        // allocate temporary buffers for MD Events data
         // MD events coordinates buffer
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
     

       size_t ic(0);
       it = events.begin();
       for (; it != it_end; it++)
       {
         double val=UnitConversion.convertUnits(it->tof());         
         if(!pQConverter->calcMatrixCoord(val,locCoord))continue; // skip ND outside the range


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
       pWSWrapper->addMDData(sig_err,run_index,det_ids,allCoord,n_added_events);
       return n_added_events;
   }

};

} // endNamespace MDEvents
} // endNamespace Mantid

#endif
