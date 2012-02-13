#ifndef MANTID_MDEVENTS_MDLEANEVENT_H_
#define MANTID_MDEVENTS_MDLEANEVENT_H_
    
#include "MantidKernel/System.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <numeric>
#include <cmath>

namespace Mantid
{
namespace MDEvents
{


  /** Templated class holding data about a neutron detection event
   * in N-dimensions (for example, Qx, Qy, Qz, E).
   *
   *   Each neutron has a signal (a float, can be != 1) and an error. This
   * is the same principle as the WeightedEvent in EventWorkspace's
   *
   * This class is meant to be as small in memory as possible, since there
   * will be (many) billions of it.
   * No virtual methods! This adds a pointer to a vtable = 8 bytes of memory
   * per event (plus the overhead of vtable lookups for calls)
   *
   * @tparam nd :: the number of dimensions that each MDLeanEvent will be tracking.
   *               an int > 0.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 3, 2010
   *
   * */
  template<size_t nd>
  class DLLExport MDLeanEvent
  {
  protected:
    /** The signal (aka weight) from the neutron event.
     * Will be exactly 1.0 unless modified at some point.
     */
    float signal;

    /** The square of the error carried in this event.
     * Will be 1.0 unless modified by arithmetic.
     * The square is used for more efficient calculations.
     */
    float errorSquared;

    /** The N-dimensional coordinates of the center of the event.
     * A simple fixed-sized array of (floats or doubles).
     */
    coord_t center[nd];

  public:
    /* Will be keeping functions inline for (possible?) performance improvements */

    //---------------------------------------------------------------------------------------------
    /** Empty constructor */
    MDLeanEvent() :
      signal(1.0), errorSquared(1.0)
    {
    }


    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error
     *
     * @param signal :: signal (aka weight)
     * @param errorSquared :: square of the error on the weight
     * */
    MDLeanEvent(const double signal, const double errorSquared) :
      signal(float(signal)), errorSquared(float(errorSquared))
    {
    }

    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error
     *
     * @param signal :: signal (aka weight)
     * @param errorSquared :: square of the error on the weight
     * */
    MDLeanEvent(const float signal, const float errorSquared) :
      signal(signal), errorSquared(errorSquared)
    {
    }

    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error and an array of centers
     *
     * @param signal :: signal (aka weight)
     * @param errorSquared :: square of the error on the weight
     * @param centers :: pointer to a nd-sized array of values to set for all coordinates.
     * */
    MDLeanEvent(const float signal, const float errorSquared, const coord_t * centers) :
      signal(signal), errorSquared(errorSquared)
    {
      for (size_t i=0; i<nd; i++)
        center[i] = centers[i];
    }

    //---------------------------------------------------------------------------------------------
    /** Copy constructor
     * @param rhs :: mdevent to copy
     * */
    MDLeanEvent(const MDLeanEvent &rhs) :
      signal(rhs.signal), errorSquared(rhs.errorSquared)
    {
      for (size_t i=0; i<nd; i++)
        center[i] = rhs.center[i];
    }


    //---------------------------------------------------------------------------------------------
    /** @return the n-th coordinate axis value.
     * @param n :: index (0-based) of the dimension you want.
     * */
    coord_t getCenter(const size_t n) const
    {
      return center[n];
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the array of coordinates
     * @return pointer to the fixed-size array.
     * */
    const coord_t * getCenter() const
    {
      return center;
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the array of coordinates, as a pointer to a non-const
     * array.
     * @return pointer to the fixed-size array.
     * */
    coord_t * getCenterNonConst()
    {
      return center;
    }

    //---------------------------------------------------------------------------------------------
    /** Sets the n-th coordinate axis value.
     * @param n :: index (0-based) of the dimension you want to set
     * @param value :: value to set.
     * */
    void setCenter(const size_t n, const coord_t value)
    {
      center[n] = value;
    }

    //---------------------------------------------------------------------------------------------
    /** Sets all the coordinates.
     *
     * @param centers :: pointer to a nd-sized array of the values to set.
     * */
    void setCoords(const coord_t * centers)
    {
      for (size_t i=0; i<nd; i++)
        center[i] = centers[i];
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the number of dimensions in the event.
     * */
    size_t getNumDims() const
    {
      return nd;
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the signal (weight) of this event.
     * */
    float getSignal() const
    {
      return signal;
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the error (squared) of this event.
     * */
    float getErrorSquared() const
    {
      return errorSquared;
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the error (not squared) of this event.
     *
     * Performance note: This calls sqrt(), which is a slow function. Use getErrorSquared() if possible.
     * @return the error (not squared) of this event.
     * */
    float getError() const
    {
      return float(sqrt(errorSquared));
    }

    //---------------------------------------------------------------------------------------------
    /** Set the signal of the event
     * @param newSignal :: the signal value  */
    void setSignal(const float newSignal)
    {
      signal = newSignal;
    }

    //---------------------------------------------------------------------------------------------
    /** Set the squared error  of the event
     * @param newerrorSquared :: the error squared value  */
    void setErrorSquared(const float newerrorSquared)
    {
      errorSquared = newerrorSquared;
    }


    //---------------------------------------------------------------------------------------------
    /** @returns a string identifying the type of event this is. */
    static std::string getTypeName()
    {
      return "MDLeanEvent";
    }



    //---------------------------------------------------------------------------------------------
    /** @return the run index of this event in the containing MDEventWorkspace.
     *          Always 0: this information is not present in a MDLeanEvent. */
    uint16_t getRunIndex() const
    {
      return 0;
    }

    //---------------------------------------------------------------------------------------------
    /** @return the detectorId of this event.
    *           Always 0: this information is not present in a MDLeanEvent. */
    int32_t getDetectorID() const
    {
      return 0;
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
      dims[1] = (nd)+2; // One point per dimension, plus signal, plus error = nd+2

      // Now the chunk size.
      std::vector<int> chunk(dims);
      chunk[0] = int(chunkSize);

      // Make and open the data
      file->makeCompData("event_data", ::NeXus::FLOAT64, dims, ::NeXus::NONE, chunk, true);

      // A little bit of description for humans to read later
      file->putAttr("description", "signal, errorsquared, center (each dim.)");
    }

    //---------------------------------------------------------------------------------------------
    /** Open the NXS data blocks for loading.
     * The data should have been created before.
     *
     * @param file :: open NXS file.
     * @return the number of events currently in the data field.
     */
    static uint64_t openNexusData(::NeXus::File * file)
    {
      // Open the data
      file->openData("event_data");
      // Return the size of dimension 0 = the number of events in the field
      return uint64_t(file->getInfo().dims[0]);
    }

    //---------------------------------------------------------------------------------------------
    /** Do any final clean up of NXS event data blocks
     *
     * @param file :: open NXS file.
     */
    static void closeNexusData(::NeXus::File * file)
    {
      file->closeData();
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
    static void saveVectorToNexusSlab(const std::vector<MDLeanEvent<nd> > & events, ::NeXus::File * file, const uint64_t startIndex,
        signal_t & totalSignal, signal_t & totalErrorSquared)
    {
      size_t numEvents = events.size();
      std::vector<double> data;
      data.reserve(numEvents*(nd+2));
      std::vector<int> start(2,0);
      //TODO: WARNING NEXUS NEEDS TO BE UPDATED TO USE 64-bit ints on Windows.
      start[0] = int(startIndex);

      totalSignal = 0;
      totalErrorSquared = 0;

      typename std::vector<MDLeanEvent<nd> >::const_iterator it = events.begin();
      typename std::vector<MDLeanEvent<nd> >::const_iterator it_end = events.end();
      for (; it != it_end; ++it)
      {
        const MDLeanEvent<nd> & event = *it;
        signal_t signal = event.signal;
        signal_t errorSquared = event.errorSquared;
        data.push_back( signal );
        data.push_back( errorSquared );
        for(size_t d=0; d<nd; d++)
          data.push_back( event.center[d] );
        // Track the total signal
        totalSignal += signal;
        totalErrorSquared += errorSquared;
      }

      // Specify the dimensions
      std::vector<int> dims;
      dims.push_back(int(numEvents));
      dims.push_back(int(nd+2));

      file->putSlab(data, start, dims);
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
    static void loadVectorFromNexusSlab(std::vector<MDLeanEvent<nd> > & events, ::NeXus::File * file, uint64_t indexStart, uint64_t numEvents)
    {
      if (numEvents == 0)
        return;

#ifdef COORDT_IS_FLOAT
      if (file->getInfo().type != ::NeXus::FLOAT32)
      {
        // TODO: Handle old files that are recorded in DOUBLEs to load as FLOATS
        throw std::runtime_error("loadVectorFromNexusSlab(): cannot load legacy file that is in floats yet.");
      }
#else
#endif
      // Allocate the data
      coord_t * data = new coord_t[numEvents*(nd+2)];

      // Start/size descriptors
      std::vector<int> start(2,0);
      start[0] = int(indexStart); //TODO: What if # events > size of int32???

      std::vector<int> size(2,0);
      size[0] = int(numEvents);
      size[1] = nd+2;

      // Get the slab into the allocated data
      file->getSlab(data, start, size);

      // Reserve the amount of space needed. Significant speed up (~30% thanks to this)
      events.reserve( events.size() + numEvents);
      for (size_t i=0; i<numEvents; i++)
      {
        // Index into the data array
        size_t ii = i*(nd+2);

        // Point directly into the data block for the centers.
        // WARNING: coord_t type must be same as double for this to work!
        coord_t * centers = data + ii+2;

        // Create the event with signal, error squared, and the centers
        events.push_back( MDLeanEvent<nd>( float(data[ii]), float(data[ii + 1]), centers) );
      }

      // Release the memory (all has been COPIED into MDLeanEvent's)
      delete [] data;
    }

  };


} // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_MDLEANEVENT_H_ */
