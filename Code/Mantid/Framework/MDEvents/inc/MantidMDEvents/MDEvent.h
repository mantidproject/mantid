#ifndef MDEVENT_H_
#define MDEVENT_H_

#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDLeanEvent.h"
#include "MantidNexusCPP/NeXusFile.hpp"
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







    //---------------------------------------------------------------------------------------------
    /** When first creating a NXS file containing the data, the proper
     * data block(s) need to be created.
     *
     * @param file :: open NXS file.
     * @param chunkSize :: chunk size to use when creating the data set (in number of events).
     */
    static void prepareNexusData(::NeXus::File * file, const uint64_t chunkSize)
    {
      std::vector<int> dims(2,0);
      dims[0] = NX_UNLIMITED;
      // One point per dimension, plus signal, plus error, plus runIndex, plus detectorID = nd+4
      dims[1] = (nd)+4;

      // Now the chunk size.
      std::vector<int> chunk(dims);
      chunk[0] = int(chunkSize);

      // Make and open the data
#ifdef COORDT_IS_FLOAT
      file->makeCompData("event_data", ::NeXus::FLOAT32, dims, ::NeXus::NONE, chunk, true);
#else
      file->makeCompData("event_data", ::NeXus::FLOAT64, dims, ::NeXus::NONE, chunk, true);
#endif

      // A little bit of description for humans to read later
      file->putAttr("description", "signal, errorSquared, runIndex, detectorId, center (each dim.)");
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
      coord_t * data = new coord_t[numEvents*(nd+4)];

      //TODO: WARNING NEXUS NEEDS TO BE UPDATED TO USE 64-bit ints on Windows.
      std::vector<int> start(2,0);
      start[0] = int(startIndex);

      totalSignal = 0;
      totalErrorSquared = 0;

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
        totalSignal += signal_t(signal);
        totalErrorSquared += signal_t(errorSquared);
      }

      // Specify the dimensions
      std::vector<int> dims;
      dims.push_back(int(numEvents));
      dims.push_back(int(nd+2));

      try
      {
        file->putSlab(data, start, dims);
      }
      catch (std::exception &)
      {
        delete [] data;
        throw;
      }

      delete [] data;
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

#ifdef COORDT_IS_FLOAT
      if (file->getInfo().type != ::NeXus::FLOAT32)
      {
        // TODO: Handle old files that are recorded in DOUBLEs to load as FLOATS
        throw std::runtime_error("loadVectorFromNexusSlab(): cannot load legacy file that is in doubles yet.");
      }
#else
#endif
      // Allocate the data
      coord_t * data = new coord_t[numEvents*(nd+4)];

      // Start/size descriptors
      std::vector<int> start(2,0);
      start[0] = int(indexStart); //TODO: What if # events > size of int32???

      std::vector<int> size(2,0);
      size[0] = int(numEvents);
      size[1] = nd+4;

      // Get the slab into the allocated data
      file->getSlab(data, start, size);

      // Reserve the amount of space needed. Significant speed up (~30% thanks to this)
      events.reserve( events.size() + numEvents);
      for (size_t i=0; i<numEvents; i++)
      {
        // Index into the data array
        size_t ii = i*(nd+4);

        // Point directly into the data block for the centers.
        // WARNING: coord_t type must be same as double for this to work!
        coord_t * centers = data + ii+4;

        // Create the event with signal, error squared,
        // the runIndex, the detectorID, and the centers
        events.push_back( MDEvent<nd>( float(data[ii]), float(data[ii + 1]),
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
