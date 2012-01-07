#ifndef H_CONVERT_TO_MDEVENTS_METHODS
#define H_CONVERT_TO_MDEVENTS_METHODS
//
#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Algorithm.h" 

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidMDEvents/MDEventWSWrapper.h"
#include "MantidMDEvents/MDEvent.h"

#include "MantidMDAlgorithms/ConvertToMDEventsDetInfo.h"
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

        File/ change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

// service variable used for efficient filling of the MD event WS  -> should be moved to configuration;
#define SPLIT_LEVEL  2048
/** class describes the inteface to the methods, which perfoen the conversion from usual workspaces to MDEventWorkspace */
class IConvertToMDEventMethods
{
public:
    virtual void setUPConversion(Mantid::API::MatrixWorkspace_sptr inWS2D,preprocessed_detectors &det_loc,MDWSDescription &TWS,boost::shared_ptr<MDEvents::MDEventWSWrapper> pWSWrapper)=0;
    virtual void runConversion()=0;
    virtual void conversionChunk()=0;
    virtual ~IConvertToMDEventMethods(){};
};

 class ConvertToMDEvents;
 /// the class describes the properties of target MD workspace, which should be obtained as the result of this algorithm. 
  struct MDWSDescription
  {
  public:
      /// constructor
      MDWSDescription():n_activated_dimensions(0),emode(-1){};
    /// the variable which describes the number of the dimensions, in the target workspace. 
    /// Calculated from number of input properties and the operations, performed on input workspace;
    size_t n_activated_dimensions;
    ///
    int emode;
    /// minimal and maximal values for the workspace dimensions:
    std::vector<double>      dim_min,dim_max;
    /// the names for the target workspace dimensions and properties of input MD workspace
    std::vector<std::string> dim_names;
    /// the units of target workspace dimensions and properties of input MD workspace dimensions
    std::vector<std::string> dim_units;
    /// the matrix to transform momentums of the workspace into notional target coordinate system
    std::vector<double> rotMatrix;  // should it be the Quat?
    /// helper function checks if min values are less them max values and are consistent between each other 
    void checkMinMaxNdimConsistent(Mantid::Kernel::Logger& log)const;
 
  }; 

  /// known sates for algorithms, caluclating Q-values
  enum Q_state{
       NoQ,     //< no Q transformatiom, just copying values along X axis (may be with units transformation)
       modQ,    //< calculate mod Q
       Q3D,      //< calculate 3 component of Q in fractional coordinate system.
       NQStates  // number of various recognized Q-analysis modes used to terminate Q-state algorithms metalooop.
   };
  /**  known analysis modes, arranged according to emodes 
    *  It is importent to assign enums proper numbers, as direct correspondence between enums and their emodes 
    *  used by the external units conversion algorithms and this algorithm, so the agreement should be the stame     */
  enum AnalMode{  
      Elastic = 0,  //< int emode = 0; Elastic analysis
      Direct  = 1,  //< emode=1; Direct inelastic analysis mode
      Indir   = 2,  //< emode=2; InDirect inelastic analysis mode
      ANY_Mode      //< couples with NoQ, means just copying existing data (may be douing units conversion), also used to terminate AnalMode algorithms metaloop
  };
  /** enum describes if there is need to convert workspace units and different unit conversion modes 
   * this modes are identified by algorithm from workpace parameters and user input.   */
  enum CnvrtUnits   // here the numbers are specified to enable proper metaloop on conversion
  {
      ConvertNo,   //< no, input workspace has the same units as output workspace or in units used by Q-dE algorithms naturally
      ConvFast , //< the input workspace has different units from the requested and fast conversion is possible
      ConvByTOF,   //< conversion possible via TOF
      ConvFromTOF,  //< Input workspace units are the TOF 
      NConvUintsStates // number of various recognized unit conversion modes used to terminate CnvrtUnits algorithms metalooop.
  };
  enum InputWSType  // Algorithm recognizes 2 input workspace types with different interface. 
  {
      Workspace2DType, //< 2D matirix workspace
      EventWSType,     //< Event worskapce
      NInWSTypes
  };
// way to treat the X-coorinate in the workspace:
    enum XCoordType
    {
        Histohram, // typical for Matrix workspace -- deploys central average 0.5(X[i]+X[i+1]); other types of averaging are possible if needed 
        Axis       // typical for events
    };

//-----------------------------------------------
// Method to process histohram workspace
template<Q_state Q, AnalMode MODE, CnvrtUnits CONV>
class processHistoWS: public IConvertToMDEventMethods 
{
/// shalow class which is invoked from processQND procedure and describes the transformation from workspace coordinates to target coordinates
    /// presumably will be completely inlined
     template<Q_state Q, AnalMode MODE, CnvrtUnits CONV,XCoordType XTYPE> 
     friend struct COORD_TRANSFORMER;
public:
    void setUPConversion(Mantid::API::MatrixWorkspace_sptr inWS2D,preprocessed_detectors &det_loc,MDWSDescription &TWS,boost::shared_ptr<MDEvents::MDEventWSWrapper> pWSWrapper)
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
         //size_t numEvents       = el.getNumberEvents();

    
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
