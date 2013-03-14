#ifndef MDEVENT_H_
#define MDEVENT_H_

#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/System.h"
#include "MantidAPI/BoxController.h"
#include "MantidMDEvents/MDLeanEvent.h"
#include <cmath>
#include <numeric>

namespace Mantid
{
namespace MDEvents
{

// To ensure the structure is as small as possible
#pragma pack(push, 2)

  /** Templated class holding data about a neutron detection event
   * in N-dimensions (for example, Qx, Qy, Qz, E).
   *
   * This is an extension to MDLeanEvent which adds:
   *   - 16-bit int for the run_index (index into the vector of ExperimentInfo of the workspace)
   *   - 32-bit int for the detector_id.
   *
   * @tparam nd :: the number of dimensions that each MDEvent will be tracking.
   *               an int > 0.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 3, 2010
   *
   * */
  template<size_t nd>
  class DLLExport MDEvent : public MDLeanEvent<nd>
  {
  protected:

    /** 0-based index of which run this event belongs to.
     * This refers to the particular ExperimentInfo that is stored in the MDEventWorkspace;
     * and this can be used to find the run number, goniometer settings, etc. as needed
     */
    uint16_t runIndex;

    /** Detector ID of the pixel that measured this event. */
    int32_t detectorId;

  public:

    // Enum to flag this templated type as a full md event type.
    enum{is_full_mdevent=true};

    //---------------------------------------------------------------------------------------------
    /** Empty constructor */
    MDEvent()
    : MDLeanEvent<nd>(),
      runIndex(0), detectorId(0)
    {
    }


    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error
     *
     * @param signal :: signal
     * @param errorSquared :: square of the error on the weight
     * */
    MDEvent(const float signal, const float errorSquared)
    : MDLeanEvent<nd>(signal, errorSquared),
      runIndex(0), detectorId(0)
    {
    }

    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error
     *
     * @param signal :: signal
     * @param errorSquared :: square of the error on the weight
     * */
    MDEvent(const double signal, const double errorSquared)
    : MDLeanEvent<nd>(signal, errorSquared),
      runIndex(0), detectorId(0)
    {
    }

    //---------------------------------------------------------------------------------------------
    /** Constructor with signal, error, runIndex and detectorId
     *
     * @param signal :: signal (aka weight)
     * @param errorSquared :: square of the error on the weight
     * @param runIndex :: 0-based index of which run in the containing MDEventWorkspace
     * @param detectorId :: ID of the detector that measured this event.
     */
    MDEvent(const double signal, const double errorSquared, const uint16_t runIndex, const int32_t detectorId)
    : MDLeanEvent<nd>(signal, errorSquared),
      runIndex(runIndex), detectorId(detectorId)
    {
    }

    //---------------------------------------------------------------------------------------------
    /** Constructor with signal, error, runIndex and detectorId
     *
     * @param signal :: signal (aka weight)
     * @param errorSquared :: square of the error on the weight
     * @param runIndex :: 0-based index of which run in the containing MDEventWorkspace
     * @param detectorId :: ID of the detector that measured this event.
     */
    MDEvent(const float signal, const float errorSquared, const uint16_t runIndex, const int32_t detectorId)
    : MDLeanEvent<nd>(signal, errorSquared),
      runIndex(runIndex), detectorId(detectorId)
    {
    }

    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error and an array of centers
     *
     * @param signal :: signal (aka weight)
     * @param errorSquared :: square of the error on the weight
     * @param centers :: pointer to a nd-sized array of values to set for all coordinates.
     * */
    MDEvent(const float signal, const float errorSquared, const coord_t * centers)
    : MDLeanEvent<nd>(signal, errorSquared, centers),
      runIndex(0), detectorId(0)
    {
    }

    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error and an array of centers, and the runIndex and detectorID
     *
     * @param signal :: signal (aka weight)
     * @param errorSquared :: square of the error on the weight
     * @param runIndex :: 0-based index of which run in the containing MDEventWorkspace
     * @param detectorId :: ID of the detector that measured this event.
     * @param centers :: pointer to a nd-sized array of values to set for all coordinates.
     * */
    MDEvent(const float signal, const float errorSquared, const uint16_t runIndex, const int32_t detectorId, const coord_t * centers)
    : MDLeanEvent<nd>(signal, errorSquared, centers),
      runIndex(runIndex), detectorId(detectorId)
    {
    }

#ifdef COORDT_IS_FLOAT
    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error and an array of centers, and the runIndex and detectorID
     *
     * @param signal :: signal (aka weight)
     * @param errorSquared :: square of the error on the weight
     * @param runIndex :: 0-based index of which run in the containing MDEventWorkspace
     * @param detectorId :: ID of the detector that measured this event.
     * @param centers :: pointer to a nd-sized array of values to set for all coordinates.
     * */
    MDEvent(const float signal, const float errorSquared, const uint16_t runIndex, const int32_t detectorId, const double * centers)
    : MDLeanEvent<nd>(signal, errorSquared, centers),
      runIndex(runIndex), detectorId(detectorId)
    {
    }
#endif

    //---------------------------------------------------------------------------------------------
    /** @return the run index of this event in the containing MDEventWorkspace */
    uint16_t getRunIndex() const
    { return runIndex;  }

    /** Sets the runIndex of this event
     * @param index :: new runIndex value. */
    void setRunIndex(uint16_t index)
    { runIndex = index; }

    //---------------------------------------------------------------------------------------------
    /** @return the detectorId of this event. */
    int32_t getDetectorID() const
    { return detectorId;  }

    /** Sets the detectorId of this event
     * @param id :: new runIndex value. */
    void setDetectorId(int32_t id)
    { detectorId = id; }

    //---------------------------------------------------------------------------------------------
    /** @returns a string identifying the type of event this is. */
    static std::string getTypeName()
    {
      return "MDEvent";
    }


    /* static method used to convert vector of lean events into vector of their coordinates & signal and error 
     @param events    -- vector of events
     @return coord    -- vector of events coordinates, their signal and error casted to coord_t type
     @return ncols    -- the number of colunts  in the data (it is nd+4 here but may be different for other data types) 
     @return totalSignal -- total signal in the vector of events
     @return totalErr   -- total error corresponting to the vector of events
    */
    static inline void eventsToData(const std::vector<MDEvent<nd> > & events,std::vector<coord_t> &coord,size_t &ncols,double &totalSignal,double &totalErrSq )
    {
      ncols = nd+4;
      size_t nEvents=events.size()/nd;
      coord.resize(nEvents+ncols);


      totalSignal = 0;
      totalErrSq = 0;

      size_t index(0);
      typename std::vector<MDEvent<nd> >::const_iterator it = events.begin();
      typename std::vector<MDEvent<nd> >::const_iterator it_end = events.end();
      for (; it != it_end; ++it)
      {
        const MDEvent<nd> & event = *it;
        float signal = event.signal;
        float errorSquared = event.errorSquared;
        data[index++] = static_cast<coord_t>(signal);
        data[index++] = static_cast<coord_t>(errorSquared);
     // Additional stuff for MDEvent
        data[index++] = static_cast<coord_t>(event.runIndex);
        data[index++] = static_cast<coord_t>(event.detectorId);
        for(size_t d=0; d<nd; d++)
          data[index++] = event.center[d];
        // Track the total signal
        totalSignal += signal_t(signal);
        totalErrorSquared += signal_t(errorSquared);
      }

    }
    /* static method used to convert vector of data into vector of lean events 
     @return coord    -- vector of events coordinates, their signal and error casted to coord_t type
     @param events    -- vector of events
    */
    static inline void dataToEvents(const std::vector<coord_t> &coord, std::vector<MDLeanEvent<nd> > & events)
    {
    // Number of columns = number of dimensions + 2 (signal/error)
      size_t numColumns = nd+4;
      size_t numEvents = events.size()/numColumns;
      if(numEvents*numColumns!=events.size())
          throw(std::invalid_argument("wrong input array of data to convert to lean events "));

         // Reserve the amount of space needed. Significant speed up (~30% thanks to this)
      events.reserve(numEvents);
      for (size_t i=0; i<numEvents; i++)
      {
        // Index into the data array
        size_t ii = i*numColumns;

        // Point directly into the data block for the centers.
        coord_t * centers = &(data[ii+2]);

        // Create the event with signal, error squared, and the centers
        events.push_back( MDLeanEvent<nd>(coord_t(data[ii]), coord_t(data[ii + 1]), 
                                          uint16_t(data[ii + 2]), int32_t(data[ii+3]), centers) );
      }
    }



    //---------------------------------------------------------------------------------------------
    /** When first creating a NXS file containing the data, the proper
     * data block(s) need to be created.
     *
     * @param file :: open NXS file.
     * @param chunkSize :: chunk size to use when creating the data set (in number of events).
     */
    static void prepareNexusData(::NeXus::File * file, const uint64_t chunkSize)
    {
       API::BoxController::prepareEventNexusData(file,chunkSize,nd+4,"signal, errorSquared, runIndex, detectorId, center (each dim.)");
    }


    //---------------------------------------------------------------------------------------------
    /** Static method to save a vector of MDEvents of this type to a nexus file
     * open to the right group.
     * This method plops the events as a slab at a particular point in an already created array.
     * The data block MUST be already open.
     *
     * This will be re-implemented by any other MDLeanEvent-like type.
     *
     * @param events :: reference to the vector of events to save.
     * @param file :: open NXS file.
     * @param startIndex :: index in the array to start saving to
     * @param[out] totalSignal :: returns the integrated signal of all events
     * @param[out] totalErrorSquared :: returns the integrated squared error of all events
     * */
    static void saveVectorToNexusSlab(const std::vector<MDEvent<nd> > & events, ::NeXus::File * file, const uint64_t startIndex,
        signal_t & totalSignal, signal_t & totalErrorSquared)
    {
      size_t numEvents = events.size();
      if(numEvents==0)return;
      size_t numColumns = nd+4;
      coord_t * data = new coord_t[numEvents*numColumns];

      double SignalSum(0);
      double ErrorSqSum(0);

      size_t index = 0;
      typename std::vector<MDEvent<nd> >::const_iterator it = events.begin();
      typename std::vector<MDEvent<nd> >::const_iterator it_end = events.end();
      for (; it != it_end; ++it)
      {
        const MDEvent<nd> & event = *it;
        float signal = event.signal;
        float errorSquared = event.errorSquared;
        data[index++] = static_cast<coord_t>(signal);
        data[index++] = static_cast<coord_t>(errorSquared);
        // Additional stuff for MDEvent
        data[index++] = static_cast<coord_t>(event.runIndex);
        data[index++] = static_cast<coord_t>(event.detectorId);
        for(size_t d=0; d<nd; d++)
          data[index++] = event.center[d];
        // Track the total signal
        SignalSum += double(signal);
        ErrorSqSum += double(errorSquared);
      }
      totalSignal = signal_t(SignalSum);
      totalErrorSquared = signal_t(ErrorSqSum);

      MDLeanEvent<nd>::putDataInNexus(file, data, startIndex, numEvents, numColumns);
     }

    //---------------------------------------------------------------------------------------------
    /** Static method to load part of a HDF block into a vector of MDEvents.
     * The data block MUST be already open, using e.g. openNexusData()
     *
     * This will be re-implemented by any other MDLeanEvent-like type.
     *
     * @param events :: reference to the vector of events to load. This is NOT cleared by the method before loading.
     * @param file :: open NXS file.
     * @param indexStart :: index (in events) in the data field to start at
     * @param numEvents :: number of events to load.
     * */
    static void loadVectorFromNexusSlab(std::vector<MDEvent<nd> > & events, ::NeXus::File * file, uint64_t indexStart, uint64_t numEvents)
    {
      if (numEvents == 0)
        return;

      // Number of columns = number of dimensions + 4 (signal/error/detId/runIndex)
      size_t numColumns = nd+4;
      // Load the data
      coord_t * data = MDLeanEvent<nd>::getDataFromNexus(file, indexStart, numEvents, numColumns);

      // Reserve the amount of space needed. Significant speed up (~30% thanks to this)
      events.reserve( events.size() + numEvents);
      for (size_t i=0; i<numEvents; i++)
      {
        // Index into the data array
        size_t ii = i*numColumns;

        // Point directly into the data block for the centers.
        coord_t * centers = data + ii+4;

        // Create the event with signal, error squared,
        // the runIndex, the detectorID, and the centers
        events.push_back( MDEvent<nd>(float(data[ii]), float(data[ii + 1]),
            uint16_t(data[ii + 2]), int32_t(data[ii+3]), centers) );
      }

      // Release the memory (all has been COPIED into MDLeanEvent's)
      delete [] data;
    }

  };

// Return to normal packing
#pragma pack(pop)

}//namespace MDEvents

}//namespace Mantid


#endif /* MDEVENT_H_ */
