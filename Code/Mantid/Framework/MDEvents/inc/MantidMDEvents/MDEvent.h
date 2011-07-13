#ifndef MDEVENT_H_
#define MDEVENT_H_

#include "MantidKernel/System.h"
#include "MantidNexus/NeXusFile.hpp"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <numeric>
#include <cmath>

namespace Mantid
{
namespace MDEvents
{


  /** Macro TMDE to make declaring template functions
   * faster. Put this macro before function declarations.
   * Use:
   * TMDE(void ClassName)::methodName()
   * {
   *    // function body here
   * }
   *
   * @tparam MDE :: the MDEvent type; at first, this will always be MDEvent<nd>
   * @tparam nd :: the number of dimensions in the center coords. Passing this
   *               as a template argument should speed up some code.
   */
  #define TMDE(decl) template <typename MDE, size_t nd> decl<MDE, nd>

  /** Macro to make declaring template classes faster.
   * Use:
   * TMDE_CLASS
   * class ClassName : ...
   */
  #define TMDE_CLASS template <typename MDE, size_t nd>



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
   * @tparam nd :: the number of dimensions that each MDEvent will be tracking.
   *               an int > 0.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 3, 2010
   *
   * */
  template<size_t nd>
  class DLLExport MDEvent
  {
  private:
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
    MDEvent() :
      signal(1.0), errorSquared(1.0)
    {
    }


    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error
     *
     * @param signal :: signal
     * @param errorSquared :: errorSquared
     * */
    MDEvent(const double signal, const double errorSquared) :
      signal(float(signal)), errorSquared(float(errorSquared))
    {
    }

    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error
     *
     * @param signal :: signal
     * @param errorSquared :: errorSquared
     * */
    MDEvent(const float signal, const float errorSquared) :
      signal(signal), errorSquared(errorSquared)
    {
    }

    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error and an array of centers
     *
     * @param signal :: signal
     * @param errorSquared :: errorSquared
     * @param centers :: pointer to a nd-sized array of values to set for all coordinates.
     * */
    MDEvent(const float signal, const float errorSquared, const coord_t * centers) :
      signal(signal), errorSquared(errorSquared)
    {
      for (size_t i=0; i<nd; i++)
        center[i] = centers[i];
    }

    //---------------------------------------------------------------------------------------------
    /** Copy constructor
     * @param rhs :: mdevent to copy
     * */
    MDEvent(const MDEvent &rhs) :
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
     * @param newerrorSquaredl :: the error squared value  */
    void setErrorSquared(const float newerrorSquared)
    {
      errorSquared = newerrorSquared;
    }


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
     */
    static void prepareNexusData(::NeXus::File * file, size_t numPoints)
    {
      std::vector<int> dims(2,0);
      dims[0] = int(numPoints);
      dims[1] = (nd)+2;
      // Make and open the data
      file->makeData("event_data", ::NeXus::FLOAT64, dims, true);
      // A little bit of description for humans to read later
      file->putAttr("description", "signal, errorsquared, center (each dim.)");
//      dims[1]=2;
//      file->makeData("event_signal_errorsquared", ::NeXus::FLOAT32, dims, 0);
    }

    //---------------------------------------------------------------------------------------------
    /** Open the NXS data blocks for loading.
     *
     * @param file :: open NXS file.
     */
    static void openNexusData(::NeXus::File * file)
    {
      // Open the data
      file->openData("event_data");
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
     * This will be re-implemented by any other MDEvent-like type.
     *
     * @param events :: reference to the vector of events to save.
     * @param file :: open NXS file.
     * @param start :: point
     * */
    static void saveVectorToNexusSlab(const std::vector<MDEvent<nd> > & events, ::NeXus::File * file, std::vector<int> & start)
    {
      size_t numEvents = events.size();
      std::vector<double> data;
      data.reserve(numEvents*(nd+2));

      typename std::vector<MDEvent<nd> >::const_iterator it = events.begin();
      typename std::vector<MDEvent<nd> >::const_iterator it_end = events.end();
      for (; it != it_end; ++it)
      {
        const MDEvent<nd> & event = *it;
        data.push_back( event.signal );
        data.push_back( event.errorSquared );
        for(size_t d=0; d<nd; d++)
          data.push_back( event.center[d] );
      }

      // Specify the dimensions
      std::vector<int> dims;
      dims.push_back(int(numEvents));
      dims.push_back(int(nd+2));

      file->putSlab(data, start, dims);
    }

    //---------------------------------------------------------------------------------------------
    /** Static method to load part of a HDF block into a vector of MDEvents.
     * The data block MUST be already open.
     *
     * This will be re-implemented by any other MDEvent-like type.
     *
     * @param events :: reference to the vector of events to load. This is NOT cleared by the method before loading.
     * @param file :: open NXS file.
     * @param indexStart :: index (in events) in the data field to start at
     * @param numEvents :: number of events to load.
     * */
    static void loadVectorFromNexusSlab(std::vector<MDEvent<nd> > & events, ::NeXus::File * file, size_t indexStart, size_t numEvents)
    {
      if (numEvents == 0)
        return;

      // Allocate the data
      double * data = new double[numEvents*(nd+2)];

      // Start/size descriptors
      std::vector<int> start(2,0);
      start[0] = int(indexStart); //TODO: What if # events > size of int32???

      std::vector<int> size(2,0);
      size[0] = int(numEvents);
      size[1] = nd+2;

      // Get the slab into the allocated data
      file->getSlab(data, start, size);

      for (size_t i=0; i<numEvents; i++)
      {
        // Point directly into the data block for the centers.
        // WARNING: coord_t type must be same as double for this to work!
        coord_t * centers = data + i*(nd+2)+2;

        // Create the event with signal, error squared, and the centers
        events.push_back( MDEvent<nd>( float(data[i*(nd+2)]), float(data[i*(nd+2) + 1]), centers) );
      }

      // Release the memory (all has been COPIED into MDEvent's)
      delete [] data;
    }

//
//    static void saveVectorToNexusSlab(const std::vector<MDEvent<nd> > & events, ::NeXus::File * file, std::vector<int> & start)
//    {
//      size_t numEvents = events.size();
//      std::vector<coord_t> centers;
//      centers.reserve(numEvents*nd);
//      std::vector<float> signal_error;
//      signal_error.reserve(numEvents*2);
//
//      typename std::vector<MDEvent<nd> >::const_iterator it = events.begin();
//      typename std::vector<MDEvent<nd> >::const_iterator it_end = events.end();
//      for (; it != it_end; ++it)
//      {
//        const MDEvent<nd> & event = *it;
//        for(size_t d=0; d<nd; d++)
//          centers.push_back( event.center[d] );
//        signal_error.push_back( event.signal );
//        signal_error.push_back( event.errorSquared );
//      }
//
//      // Specify the dimensions
//      std::vector<int> dims;
//      dims.push_back(int(numEvents));
//      dims.push_back(int(nd));
//
//      file->openData("event_center");
//      file->putSlab(centers, start, dims);
//      file->closeData();
//
//      dims[1] = 2;
//      file->openData("event_signal_errorsquared");
//      file->putSlab(signal_error, start, dims);
//      file->closeData();
//    }







    //---------------------------------------------------------------------------------------------
    /** Static method to save a vector of MDEvents of this type to a nexus file
     * open to the right group.
     * This will be re-implemented by any other MDEvent-like type
     *
     * @param events :: reference to the vector of events to save.
     * @param file :: open NXS file. */
    static void saveVectorToNexus(const std::vector<MDEvent<nd> > & events, ::NeXus::File * file)
    {
      //TODO: Use bare C arrays maybe?
      size_t numEvents = events.size();
      std::vector<coord_t> centers;
      centers.reserve(numEvents*nd);
      std::vector<float> signal_error;
      signal_error.reserve(numEvents*2);

      typename std::vector<MDEvent<nd> >::const_iterator it = events.begin();
      typename std::vector<MDEvent<nd> >::const_iterator it_end = events.end();
      for (; it != it_end; ++it)
      {
        const MDEvent<nd> & event = *it;
        for(size_t d=0; d<nd; d++)
          centers.push_back( event.center[d] );
        signal_error.push_back( event.signal );
        signal_error.push_back( event.errorSquared );
      }

      // Specify the dimensions
      std::vector<int> dims;
      dims.push_back(int(numEvents));
      dims.push_back(int(nd));

      file->writeData("center", centers, dims);

      dims[1] = 2;
      file->writeData("signal_errorsquared", signal_error, dims);
    }




    //---------------------------------------------------------------------------------------------
    /** Static method to load a vector of MDEvents of this type from a nexus file
     * open to the right group.
     * This will be re-implemented by any other MDEvent-like type
     *
     * @param events :: reference to the vector of events to load.
     * @param file :: open NXS file. */
    static void loadVectorFromNexus(std::vector<MDEvent<nd> > & events, ::NeXus::File * file)
    {
      // Load both data vectors
      std::vector<coord_t> centers;
      std::vector<float> signal_error;

      file->openData("center");
      file->getData(centers);
      file->closeData();

      file->openData("signal_errorsquared");
      file->getData(signal_error);
      file->closeData();

      if (centers.size()/nd != signal_error.size()/2)
        throw std::runtime_error("Error loading MDEvent data from NXS file. The signal_error and center are of incompatible sizes");

      size_t numEvents = signal_error.size()/2;
      events.clear();
      events.reserve(numEvents);

      size_t center_index = 0;
      size_t signal_index = 0;
      for (size_t i=0; i<numEvents; i++)
      {
        // Get the centers
        coord_t event_center[nd];
        // TODO: memcpy might be faster?
        for(size_t d=0; d<nd; d++)
        {
          event_center[d] = centers[center_index];
          center_index++;
        }
        // Get the signal/error
        float signal = signal_error[signal_index++];
        float errorSquared = signal_error[signal_index++];
        // Make the event and push it into the vector
        events.push_back( MDEvent(signal, errorSquared, event_center) );
      }
    }


  };



}//namespace MDEvents

}//namespace Mantid


#endif /* MDEVENT_H_ */
