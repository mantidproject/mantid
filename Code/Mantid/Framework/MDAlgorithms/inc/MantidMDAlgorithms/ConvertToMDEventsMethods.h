#ifndef H_CONVERT_TO_MDEVENTS_METHODS
#define H_CONVERT_TO_MDEVENTS_METHODS
//
#include "MantidMDAlgorithms/ConvertToMDEvents.h"
#include "MantidMDAlgorithms/ConvertToMDEventsCoordTransf.h"

namespace Mantid
{
namespace MDAlgorithms
{
/** The macrodefinitions for ConvertToMDEvents function, making the conversion from  into the MD events 
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

        File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

// service variable used for efficient filling of the MD event WS  -> should be moved to configuration;
#define SPLIT_LEVEL  2048



//-----------------------------------------------
// Method to process histohram workspace
template<Q_state Q, AnalMode MODE, CnvrtUnits CONV>
void ConvertToMDEvents::processQNDHWS()
{
    // counder for the number of events
    size_t n_added_events(0);
    // amount of work
    const size_t numSpec  = inWS2D->getNumberHistograms();
    // progress reporter
    pProg = std::auto_ptr<API::Progress>(new API::Progress(this,0.0,1.0,numSpec));

    // initiate the templated class which does the conversion of workspace data into MD WS coordinates;
    COORD_TRANSFORMER<Q,MODE,CONV,Histohram> trn(this); 
   

    // copy experiment info into target workspace
    API::ExperimentInfo_sptr ExperimentInfo(inWS2D->cloneExperimentInfo());
    // run index;
    uint16_t runIndex   = this->pWSWrapper->pWorkspace()->addExperimentInfo(ExperimentInfo);
    // number of dimesnions
    size_t n_dims       = this->pWSWrapper->nDimensions();

    
    const size_t specSize = this->inWS2D->blocksize();    
    size_t nValidSpectra  = det_loc.det_id.size();

    std::vector<coord_t>  Coord(n_dims);             // coordinates for single event
    // if any property dimension is outside of the data range requested, the job is done;
    if(!trn.calcGenericVariables(Coord,n_dims))return; 

    // take at least bufSize amout of data in one run for efficiency
    size_t buf_size     = ((specSize>SPLIT_LEVEL)?specSize:SPLIT_LEVEL);
    // allocate temporary buffer for MD Events data
    std::vector<coord_t>  allCoord(0); // MD events coordinates buffer
    allCoord.reserve(n_dims*buf_size);
 
    std::vector<float>    sig_err(2*buf_size);       // array for signal and error. 
    std::vector<uint16_t> run_index(buf_size);       // Buffer run index for each event 
    std::vector<uint32_t> det_ids(buf_size);         // Buffer of det Id-s for each event


  
    //External loop over the spectra:
    for (int64_t i = 0; i < int64_t(nValidSpectra); ++i)
    {
        size_t ic                 = det_loc.detIDMap[i];
        int32_t det_id            = det_loc.det_id[i];

        const MantidVec& X        = inWS2D->readX(ic);
        const MantidVec& Signal   = inWS2D->readY(ic);
        const MantidVec& Error    = inWS2D->readE(ic);


        if(!trn.calcYDepCoordinates(Coord,ic))continue;   // skip y outsize of the range;

        //=> START INTERNAL LOOP OVER THE "TIME"
        for (size_t j = 0; j < specSize; ++j)
        {
            // drop emtpy events
           if(Signal[j]<FLT_EPSILON)continue;

           if(!trn.calcMatrixCoord(X,i,j,Coord))continue; // skip ND outside the range
            //  ADD RESULTING EVENTS TO THE WORKSPACE
            float ErrSq = float(Error[j]*Error[j]);

            // coppy all data into data buffer for future transformation into events;
            sig_err[2*n_added_events+0]=float(Signal[j]);
            sig_err[2*n_added_events+1]=ErrSq;
            run_index[n_added_events]  = runIndex;
            det_ids[n_added_events]    = det_id;
            allCoord.insert(allCoord.end(),Coord.begin(),Coord.end());

            n_added_events++;
            if(n_added_events>=buf_size){
              pWSWrapper->addMDData(sig_err,run_index,det_ids,allCoord,n_added_events);
 
              n_added_events=0;
              pProg->report(i);
            }
       
        } // end spectra loop
      
       } // end detectors loop;

       if(n_added_events>0){
              pWSWrapper->addMDData(sig_err,run_index,det_ids,allCoord,n_added_events);
 
              n_added_events=0;
        }
 
        pWSWrapper->refreshCache();
        pProg->report();          

}

// Method to process event workspace
template<Q_state Q, AnalMode MODE, CnvrtUnits CONV>
void ConvertToMDEvents::processQNDEWS()
{
    DataObjects::EventWorkspace_const_sptr pEventWS  = boost::static_pointer_cast<const DataObjects::EventWorkspace>(inWS2D);

   // size_t lastNumBoxes = this->pWSWrapper->pWorkspace()->getBoxController()->getTotalNumMDBoxes();
    // counder for the number of events
    size_t n_added_events(0);
    // amount of work
    const size_t numSpec  = inWS2D->getNumberHistograms();
    size_t nValidSpectra  = det_loc.det_id.size();
    // progress reporter
    pProg = std::auto_ptr<API::Progress>(new API::Progress(this,0.0,1.0,numSpec));

    // initiate the templated class which does the conversion of workspace data into MD WS coordinates;
    COORD_TRANSFORMER<Q,MODE,CONV,Histohram> trn(this); 
   

    // copy experiment info into target workspace
    API::ExperimentInfo_sptr ExperimentInfo(inWS2D->cloneExperimentInfo());
    // run index;
    uint16_t runIndex   = this->pWSWrapper->pWorkspace()->addExperimentInfo(ExperimentInfo);
    // number of dimesnions
    size_t n_dims       = this->pWSWrapper->nDimensions();
   // coordinates for single event
    std::vector<coord_t>  Coord(n_dims);           
    // if any property dimension is outside of the data range requested, the job is done;
    if(!trn.calcGenericVariables(Coord,n_dims))return; 

    // take at least bufSize amout of data in one run for efficiency
    size_t buf_size     = SPLIT_LEVEL;
    // allocate temporary buffer for MD Events data
    std::vector<coord_t>  allCoord(0); // MD events coordinates buffer
    allCoord.reserve(n_dims*buf_size);
 
    std::vector<float>    sig_err(2*buf_size);       // array for signal and error. 
    std::vector<uint16_t> run_index(buf_size);       // Buffer run index for each event 
    std::vector<uint32_t> det_ids(buf_size);         // Buffer of det Id-s for each event


    for (size_t wi=0; wi < nValidSpectra; wi++)
    {
         size_t ic                 = det_loc.detIDMap[wi];
         int32_t det_id            = det_loc.det_id[wi];

         const DataObjects::EventList & el   = pEventWS->getEventList(ic);
         size_t numEvents       = (int64_t)el.getNumberEvents();

    
        const MantidVec& X        = el.dataX();
        const MantidVec& Signal   = el.dataY();
        const MantidVec& Error    = el.dataE();
         if(!trn.calcYDepCoordinates(Coord,ic))continue;   // skip y outsize of the range;

        //=> START INTERNAL LOOP OVER THE "TIME"
        for (size_t j = 0; j < Signal.size(); ++j)
        {
           // drop emtpy histohrams
           if(Signal[j]<FLT_EPSILON)continue;


           if(!trn.calcMatrixCoord(X,ic,j,Coord))continue; // skip ND outside the range
            //  ADD RESULTING EVENTS TO THE WORKSPACE
            float ErrSq = float(Error[j]*Error[j]);

            // coppy all data into data buffer for future transformation into events;
            sig_err[2*n_added_events+0]=float(Signal[j]);
            sig_err[2*n_added_events+1]=ErrSq;
            run_index[n_added_events]  = runIndex;
            det_ids[n_added_events]    = det_id;
            allCoord.insert(allCoord.end(),Coord.begin(),Coord.end());

            n_added_events++;
            if(n_added_events>=buf_size){
              pWSWrapper->addMDData(sig_err,run_index,det_ids,allCoord,n_added_events);
 
              n_added_events=0;
              pProg->report(wi);
          }
        } // end spectra loop
   
       } // end detectors loop;

       if(n_added_events>0){
              pWSWrapper->addMDData(sig_err,run_index,det_ids,allCoord,n_added_events);
 
              n_added_events=0;
        }
 
        pWSWrapper->refreshCache();
      


      //// Equivalent of: this->convertEventList(wi);
      //EventList & el = in_ws->getEventList(wi);

      //// We want to bind to the right templated function, so we have to know the type of TofEvent contained in the EventList.
      //boost::function<void ()> func;
      //switch (el.getEventType())
      //{
      //case TOF:
      //  func = boost::bind(&ConvertToDiffractionMDWorkspace::convertEventList<TofEvent>, &*this, static_cast<int>(wi));
      //  break;
      //case WEIGHTED:
      //  func = boost::bind(&ConvertToDiffractionMDWorkspace::convertEventList<WeightedEvent>, &*this, static_cast<int>(wi));
      //  break;
      //case WEIGHTED_NOTIME:
      //  func = boost::bind(&ConvertToDiffractionMDWorkspace::convertEventList<WeightedEventNoTime>, &*this, static_cast<int>(wi));
      //  break;
      //default:
      //  throw std::runtime_error("EventList had an unexpected data type!");
      //}

      //// Give this task to the scheduler
      //double cost = double(el.getNumberEvents());
      //ts->push( new FunctionTask( func, cost) );

      //// Keep a running total of how many events we've added
      //eventsAdded += el.getNumberEvents();
      //if (bc->shouldSplitBoxes(eventsAdded, lastNumBoxes))
      //{
      //  // Do all the adding tasks
      //  tp.joinAll();
      //  // Now do all the splitting tasks
      //  ws->splitAllIfNeeded(ts);
      //  tp.joinAll();

      //  // Count the new # of boxes.
      //  lastNumBoxes = ws->getBoxController()->getTotalNumMDBoxes();
     
      //  eventsAdded = 0;
      //}


   // tp.joinAll();
   // // Do a final splitting of everything
   // ws->splitAllIfNeeded(ts);
   // tp.joinAll();
   //
   // // Recount totals at the end.
   //// cputim.reset();
   // ws->refreshCache();
   //    //TODO: Centroid in parallel, maybe?
   // ws->getBox()->refreshCentroid(NULL);
   


}


    //      // This little dance makes the getting vector of events more general (since you can't overload by return type).
//      typename std::vector<T> * events_ptr;
//      getEventsFrom(el, events_ptr);
//      typename std::vector<T> & events = *events_ptr;
//
//      // Iterators to start/end
//      typename std::vector<T>::iterator it = events.begin();
//      typename std::vector<T>::iterator it_end = events.end();
//
//      for (; it != it_end; it++)
//      {
//      // Get the wavenumber in ang^-1 using the previously calculated constant.
//        double wavenumber = wavenumber_in_angstrom_times_tof_in_microsec / it->tof();
//
//
//        // Q vector = K_final - K_initial = wavenumber * (output_direction - input_direction)
//        coord_t center[3] = {Q_dir_x * wavenumber, Q_dir_y * wavenumber, Q_dir_z * wavenumber};
//
//   
//          // Push the MDLeanEvent with the same weight
//          out_events.push_back( MDE(float(it->weight()), float(it->errorSquared()), center) );
//
//      }


//template <class T>
//void ConvertToDiffractionMDWorkspace::convertEventList(int workspaceIndex)
//  {
//    EventList & el = in_ws->getEventList(workspaceIndex);
//    size_t numEvents = el.getNumberEvents();
//
//    // Get the position of the detector there.
//    std::set<detid_t>& detectors = el.getDetectorIDs();
//    if (detectors.size() > 0)
//    {
//      // The 3D MDEvents that will be added into the MDEventWorkspce
//      std::vector<MDE> out_events;
//      out_events.reserve( el.getNumberEvents() );
//
//     
//      // This little dance makes the getting vector of events more general (since you can't overload by return type).
//      typename std::vector<T> * events_ptr;
//      getEventsFrom(el, events_ptr);
//      typename std::vector<T> & events = *events_ptr;
//
//      // Iterators to start/end
//      typename std::vector<T>::iterator it = events.begin();
//      typename std::vector<T>::iterator it_end = events.end();
//
//      for (; it != it_end; it++)
//      {
//      // Get the wavenumber in ang^-1 using the previously calculated constant.
//        double wavenumber = wavenumber_in_angstrom_times_tof_in_microsec / it->tof();
//
//
//        // Q vector = K_final - K_initial = wavenumber * (output_direction - input_direction)
//        coord_t center[3] = {Q_dir_x * wavenumber, Q_dir_y * wavenumber, Q_dir_z * wavenumber};
//
//   
//          // Push the MDLeanEvent with the same weight
//          out_events.push_back( MDE(float(it->weight()), float(it->errorSquared()), center) );
//
//      }
//
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
//      // Add them to the MDEW
//      ws->addEvents(out_events);
//    }
//    
//  }




} // endNamespace MDAlgorithms
} // endNamespace Mantid

#endif
