#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDLeanEvent.h"
#include "MantidAPI/ImplicitFunction.h"
#include "MantidNexus/NeXusFile.hpp"
#include "MantidKernel/DiskMRU.h"

using Mantid::Kernel::DiskMRU;

namespace Mantid
{
namespace MDEvents
{

//-----------------------------------------------------------------------------------------------
  /** Empty constructor */
  TMDE(MDBox)::MDBox()
   : IMDBox<MDE, nd>(),
     m_dataBusy(false), m_dataConstAccess(true),
     m_fileIndexStart(0), m_fileNumEvents(0),
     m_onDisk(false), m_inMemory(false)
  {
  }

  //-----------------------------------------------------------------------------------------------
  /** Constructor
   * @param controller :: BoxController that controls how boxes split
   * @param depth :: splitting depth of the new box.
   */
  TMDE(MDBox)::MDBox(BoxController_sptr controller, const size_t depth)
    : IMDBox<MDE, nd>(),
      m_dataBusy(false), m_dataConstAccess(true),
      m_fileIndexStart(0), m_fileNumEvents(0),
      m_onDisk(false), m_inMemory(false)

  {
    if (controller->getNDims() != nd)
      throw std::invalid_argument("MDBox::ctor(): controller passed has the wrong number of dimensions.");
    this->m_BoxController = controller;
    this->m_depth = depth;
    // Give it a fresh ID from the controller.
    this->setId( controller->getNextId() );
  }

  //-----------------------------------------------------------------------------------------------
  /** Constructor
   * @param controller :: BoxController that controls how boxes split
   * @param depth :: splitting depth of the new box.
   * @param extents :: vector defining the extents
   */
  TMDE(MDBox)::MDBox(BoxController_sptr controller, const size_t depth, const std::vector<Mantid::Geometry::MDDimensionExtents> & extentsVector)
      : IMDBox<MDE, nd>(extentsVector),
        m_dataBusy(false), m_dataConstAccess(true),
        m_fileIndexStart(0), m_fileNumEvents(0),
        m_onDisk(false), m_inMemory(false)
  {
    if (controller->getNDims() != nd)
      throw std::invalid_argument("MDBox::ctor(): controller passed has the wrong number of dimensions.");
    this->m_BoxController = controller;
    this->m_depth = depth;
    // Give it a fresh ID from the controller.
    this->setId( controller->getNextId() );
  }


  //-----------------------------------------------------------------------------------------------
  /** Copy constructor */
  TMDE(MDBox)::MDBox(const MDBox<MDE,nd> & other)
   : IMDBox<MDE, nd>(other),
     data(other.data),
     m_dataBusy(other.m_dataBusy), m_dataConstAccess(other.m_dataConstAccess),
     m_fileIndexStart(other.m_fileIndexStart), m_fileNumEvents(other.m_fileNumEvents),
     m_onDisk(other.m_onDisk), m_inMemory(other.m_inMemory)
  {
  }


  //-----------------------------------------------------------------------------------------------
  /** Clear any points contained. */
  TMDE(
  void MDBox)::clear()
  {
    // Make sure the object is not in any of the disk MRUs
    this->m_BoxController->getDiskMRU().objectDeleted(this);
    // Clear all contents
    this->m_signal = 0.0;
    this->m_errorSquared = 0.0;
    m_fileNumEvents = 0;
    data.clear();
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this box */
  TMDE(
  size_t MDBox)::getNumDims() const
  {
    return nd;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of un-split MDBoxes in this box (including all children)
   * @return :: 1 always since this is just a MD Box*/
  TMDE(
  size_t MDBox)::getNumMDBoxes() const
  {
    return 1;
  }

  //-----------------------------------------------------------------------------------------------
  /// Fill a vector with all the boxes up to a certain depth
  TMDE(
  void MDBox)::getBoxes(std::vector<IMDBox<MDE,nd> *> & boxes, size_t /*maxDepth*/, bool /*leafOnly*/)
  {
    boxes.push_back(this);
  }

  //-----------------------------------------------------------------------------------------------
  /// Fill a vector with all the boxes up to a certain depth
  TMDE(
  void MDBox)::getBoxes(std::vector<IMDBox<MDE,nd> *> & boxes, size_t /*maxDepth*/, bool /*leafOnly*/, Mantid::Geometry::MDImplicitFunction * /*function*/)
  {
    boxes.push_back(this);
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this box */
  TMDE(size_t MDBox)::getNPoints() const
  {
    if (m_onDisk)
    {
      if (m_inMemory)
        return data.size();
      else
        return m_fileNumEvents + data.size();
    }
    else
      return data.size();
  }


  //-----------------------------------------------------------------------------------------------
  /** Set the start/end point in the file where the events are located
   * @param start :: start point,
   * @param numEvents :: number of events in the file   */
  TMDE(
  void MDBox)::setFileIndex(uint64_t start, uint64_t numEvents)
  {
    m_fileIndexStart = start;
    m_fileNumEvents = numEvents;
  }

  //-----------------------------------------------------------------------------------------------
  /** Private method to load the events from a disk cache into the "data" member vector.
   * If there were events in data[] because of the use of addEvents, they are APPENDED
   * to the ones from disk.
   *
   */
  TMDE(
  inline void MDBox)::loadEvents() const
  {
    // Is the data in memory right now (cached copy)?
    if (!m_inMemory)
    {
      // Perform the data loading
      ::NeXus::File * file = this->m_BoxController->getFile();
      if (file)
      {
        this->m_BoxController->fileMutex.lock();
        // Note that this APPENDS any events to the existing event list
        //  (in the event that addEvent() was called for a box that was on disk)
        MDE::loadVectorFromNexusSlab(data, file, m_fileIndexStart, m_fileNumEvents);
        this->m_BoxController->fileMutex.unlock();
        m_inMemory = true;
      }
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns a reference to the events vector contained within.
   * VERY IMPORTANT: call MDBox::releaseEvents() when you are done accessing that data.
   */
  TMDE(
  std::vector< MDE > & MDBox)::getEvents()
  {
    if (m_onDisk)
    {
      // Load and concatenate the events if needed
      this->loadEvents();
      // After loading, or each time you request it:
      // Touch the MRU to say you just used it.
      this->m_BoxController->getDiskMRU().loading(this);
      // The data vector is busy - can't release the memory yet
      this->m_dataBusy = true;
      // This access to data was NOT const, so it might have changed.
      this->m_dataConstAccess = false;
    }
    // else: do nothing if the events are already in memory.
    return data;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns a const reference to the events vector contained within.
   * VERY IMPORTANT: call MDBox::releaseEvents() when you are done accessing that data.
   */
  TMDE(
  const std::vector<MDE> & MDBox)::getConstEvents() const
  {
    if (m_onDisk)
    {
      // Load and concatenate the events if needed
      this->loadEvents();
      // After loading, or each time you request it:
      // Touch the MRU to say you just used it.
      this->m_BoxController->getDiskMRU().loading(this);
      // The data vector is busy - can't release the memory yet
      this->m_dataBusy = true;
      // This access to data was const
      this->m_dataConstAccess = true;
    }
    // else: do nothing if the events are already in memory.
    return data;
  }

  //-----------------------------------------------------------------------------------------------
  /** For file-backed MDBoxes, this marks that the data vector is
   * no longer "busy", and so it is safe for the MRU to cache it
   * back to disk if needed.
   */
  TMDE(
  void MDBox)::releaseEvents() const
  {
    if (m_onDisk)
    {
      // Data vector is no longer busy.
      this->m_dataBusy = false;
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Call to save the data (if needed) and release the memory used.
   * Called from the DiskMRU.
   */
  TMDE(
  void MDBox)::save() const
  {
    // When addEvent() was called on cached data, so the data[] vector has something despite the events being on disk
    bool addedEventsOnCached = (!m_inMemory && (data.size() != 0));

    // Only save to disk when the access was non-const;
    //  or when you added events to a cached data
    //  or when you added events, FOLLOWED by getting the events (which merged the vector together)
    if (!m_dataConstAccess || addedEventsOnCached || (m_fileNumEvents != data.size()) )
    {
      // This will load and append events ONLY if needed.
      if (addedEventsOnCached)
        this->loadEvents();

      // This is the new size of the event list, possibly appended (if used AddEvent) or changed otherwise (non-const access)
      size_t newNumEvents = data.size();
      DiskMRU & mru = this->m_BoxController->getDiskMRU();
      if (newNumEvents != m_fileNumEvents)
      {
        // Event list changed size. The MRU can tell us where it best fits now.
        m_fileIndexStart = mru.relocate(m_fileIndexStart, m_fileNumEvents, newNumEvents);
        m_fileNumEvents = newNumEvents;
        if (newNumEvents > 0)
        {
          // Save it where the MRU told us to
          this->saveNexus( this->m_BoxController->getFile() );
        }
      }
      else
      {
        // Size of the event list is the same, keep it there.
        if (newNumEvents > 0)
        {
          // Save at the same place
          this->saveNexus( this->m_BoxController->getFile() );
        }
      }
    }
    // Free up memory by clearing the events
    data.clear();
    vec_t().swap(data); // Linux trick to really free the memory
    // Data is no longer in memory
    m_inMemory = false;
  }


  //-----------------------------------------------------------------------------------------------
  /** Save the box's Event data to an open nexus file.
   *
   * @param file :: Nexus File object, must already by opened with MDE::prepareNexusData()
   */
  TMDE(
  inline void MDBox)::saveNexus(::NeXus::File * file) const
  {
    //std::cout << "Box " << this->getId() << " saving to " << m_fileIndexStart << std::endl;
    MDE::saveVectorToNexusSlab(this->data, file, m_fileIndexStart,
                               this->m_signal, this->m_errorSquared);
  }


  //-----------------------------------------------------------------------------------------------
  /** Load the box's Event data from an open nexus file.
   * The FileIndex start and numEvents must be set correctly already.
   *
   * @param file :: Nexus File object, must already by opened with MDE::openNexusData()
   */
  TMDE(
  inline void MDBox)::loadNexus(::NeXus::File * file)
  {
    this->data.clear();
    MDE::loadVectorFromNexusSlab(this->data, file, m_fileIndexStart, m_fileNumEvents);
  }






  //-----------------------------------------------------------------------------------------------
  /** Allocate and return a vector with a copy of all events contained
   */
  TMDE(
  std::vector< MDE > * MDBox)::getEventsCopy()
  {
    std::vector< MDE > * out = new std::vector<MDE>();
    //Make the copy
    out->insert(out->begin(), data.begin(), data.end());
    return out;
  }

  //-----------------------------------------------------------------------------------------------
  /** Refresh the cache.
   * For MDBox, the signal/error is tracked on adding, so
   * this just calculates the centroid.
   */
  TMDE(
  void MDBox)::refreshCache(Kernel::ThreadScheduler * /*ts*/)
  {
#ifndef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    // Use the cached value if it is on disk
    if (!m_onDisk) // TODO: Dirty flag?
    {
      this->m_signal = 0;
      this->m_errorSquared = 0;

      typename std::vector<MDE>::const_iterator it_end = data.end();
      for(typename std::vector<MDE>::const_iterator it = data.begin(); it != it_end; it++)
      {
        const MDE & event = *it;
        this->m_signal += event.getSignal();
        this->m_errorSquared += event.getErrorSquared();
      }
    }
#endif
  }


  //-----------------------------------------------------------------------------------------------
  /** Calculate and ccache the centroid of this box.
   */
  TMDE(
  void MDBox)::refreshCentroid(Kernel::ThreadScheduler * /*ts*/)
  {
#ifdef MDBOX_TRACK_CENTROID
    calculateCentroid(this->m_centroid);
#endif
  }

  //-----------------------------------------------------------------------------------------------
  /** Calculate the centroid of this box.
   * @param centroid[out] :: nd-sized array that will be set to the centroid. */
  TMDE(
  void MDBox)::calculateCentroid(coord_t * centroid) const
  {
    for (size_t d=0; d<nd; d++)
      centroid[d] = 0;

    // Signal was calculated before (when adding)
    // Keep 0.0 if the signal is null. This avoids dividing by 0.0
    if (this->m_signal == 0) return;

    typename std::vector<MDE>::const_iterator it_end = data.end();
    for(typename std::vector<MDE>::const_iterator it = data.begin(); it != it_end; it++)
    {
      const MDE & event = *it;
      double signal = event.getSignal();
      for (size_t d=0; d<nd; d++)
      {
        // Total up the coordinate weighted by the signal.
        centroid[d] += event.getCenter(d) * signal;
      }
    }

    // Normalize by the total signal
    for (size_t d=0; d<nd; d++)
      centroid[d] /= this->m_signal;
  }

  //-----------------------------------------------------------------------------------------------
  /** Calculate the statistics for each dimension of this MDBox, using
   * all the contained events
   * @param stats :: nd-sized fixed array of MDDimensionStats, reset to 0.0 before!
   */
  TMDE(
  void MDBox)::calculateDimensionStats(MDDimensionStats * stats) const
  {
    typename std::vector<MDE>::const_iterator it_end = data.end();
    for(typename std::vector<MDE>::const_iterator it = data.begin(); it != it_end; it++)
    {
      const MDE & event = *it;
      for (size_t d=0; d<nd; d++)
      {
        stats[d].addPoint( event.getCenter(d) );
      }
    }
  }



  //-----------------------------------------------------------------------------------------------
  /** Add a MDLeanEvent to the box.
   * @param event :: reference to a MDLeanEvent to add.
   * */
  TMDE(
  void MDBox)::addEvent( const MDE & event)
  {
    dataMutex.lock();
    this->data.push_back(event);

#ifdef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    // Keep the running total of signal and error
    double signal = event.getSignal();
    this->m_signal += signal;
    this->m_errorSquared += event.getErrorSquared();
#endif

#ifdef MDBOX_TRACKCENTROID_WHENADDING
    // Running total of the centroid
    for (size_t d=0; d<nd; d++)
      this->m_centroid[d] += event.getCenter(d) * signal;
#endif

    dataMutex.unlock();
  }

  //-----------------------------------------------------------------------------------------------
  /** Add several events. No bounds checking is made!
   *
   * @param events :: vector of events to be copied.
   * @param start_at :: index to start at in vector
   * @param stop_at :: index to stop at in vector (exclusive)
   * @return the number of events that were rejected (because of being out of bounds)
   */
  TMDE(
  size_t MDBox)::addEvents(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at)
  {
    dataMutex.lock();
    typename std::vector<MDE>::const_iterator start = events.begin()+start_at;
    typename std::vector<MDE>::const_iterator end = events.begin()+stop_at;
    // Copy all the events
    this->data.insert(this->data.end(), start, end);

#ifdef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    //Running total of signal/error
    for(typename std::vector<MDE>::const_iterator it = start; it != end; it++)
    {
      double signal = it->getSignal();
      this->m_signal += signal;
      this->m_errorSquared += it->getErrorSquared();

#ifdef MDBOX_TRACKCENTROID_WHENADDING
    // Running total of the centroid
    for (size_t d=0; d<nd; d++)
      this->m_centroid[d] += it->getCenter(d) * signal;
#endif
    }
#endif

    dataMutex.unlock();
    return 0;
  }

  //-----------------------------------------------------------------------------------------------
  /** Perform centerpoint binning of events.
   * @param bin :: MDBin object giving the limits of events to accept.
   * @param fullyContained :: optional bool array sized [nd] of which dimensions are known to be fully contained (for MDSplitBox)
   */
  TMDE(
  void MDBox)::centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const
  {
    if (fullyContained)
    {
      // For MDSplitBox, check if we've already found that all dimensions are fully contained
      size_t d;
      for (d=0; d<nd; ++d)
      {
        if (!fullyContained[d]) break;
      }
      if (d == nd)
      {
//        std::cout << "MDBox at depth " << this->m_depth << " was fully contained in bin " << bin.m_index << ".\n";
        // All dimensions are fully contained, so just return the cached total signal instead of counting.
        bin.m_signal += this->m_signal;
        bin.m_errorSquared += this->m_errorSquared;
        return;
      }
    }

    // If the box is cached to disk, you need to retrieve it
    const std::vector<MDE> & events = this->getConstEvents();
    typename std::vector<MDE>::const_iterator it = events.begin();
    typename std::vector<MDE>::const_iterator it_end = events.end();

    // For each MDLeanEvent
    for (; it != it_end; ++it)
    {
      // Go through each dimension
      size_t d;
      for (d=0; d<nd; ++d)
      {
        // Check that the value is within the bounds given. (Rotation is for later)
        coord_t x = it->getCenter(d);
        if (x < bin.m_min[d])
          break;
        if (x >= bin.m_max[d])
          break;
      }
      // If the loop reached the end, then it was all within bounds.
      if (d == nd)
      {
        // Accumulate error and signal
        bin.m_signal += it->getSignal();
        bin.m_errorSquared += it->getErrorSquared();
      }
    }

    this->releaseEvents();
  }


  //-----------------------------------------------------------------------------------------------
  /** General (non-axis-aligned) centerpoint binning method.
   * TODO: TEST THIS!
   *
   * @param bin :: a MDBin object giving the limits, aligned with the axes of the workspace,
   *        of where the non-aligned bin MIGHT be present.
   * @param function :: a ImplicitFunction that will evaluate true for any coordinate that is
   *        contained within the (non-axis-aligned) bin.
   */
  TMDE(
  void MDBox)::generalBin(MDBin<MDE,nd> & bin, Mantid::API::ImplicitFunction & function) const
  {
    UNUSED_ARG(bin);

    typename std::vector<MDE>::const_iterator it = data.begin();
    typename std::vector<MDE>::const_iterator it_end = data.end();
    bool mask[3] = {false, false, false}; //HACK
    // For each MDLeanEvent
    for (; it != it_end; ++it)
    {
      if (function.evaluate(it->getCenter(), mask, 3)) //HACK
      {
        // Accumulate error and signal
        bin.m_signal += it->getSignal();
        bin.m_errorSquared += it->getErrorSquared();
      }
    }
  }


  /** Integrate the signal within a sphere; for example, to perform single-crystal
   * peak integration.
   * The CoordTransform object could be used for more complex shapes, e.g. "lentil" integration, as long
   * as it reduces the dimensions to a single value.
   *
   * @param radiusTransform :: nd-to-1 coordinate transformation that converts from these
   *        dimensions to the distance (squared) from the center of the sphere.
   * @param radiusSquared :: radius^2 below which to integrate
   * @param[out] signal :: set to the integrated signal
   * @param[out] errorSquared :: set to the integrated squared error.
   */
  TMDE(
  void MDBox)::integrateSphere(CoordTransform & radiusTransform, const coord_t radiusSquared, signal_t & signal, signal_t & errorSquared) const
  {
    // If the box is cached to disk, you need to retrieve it
    const std::vector<MDE> & events = this->getConstEvents();
    typename std::vector<MDE>::const_iterator it = events.begin();
    typename std::vector<MDE>::const_iterator it_end = events.end();

    // For each MDLeanEvent
    for (; it != it_end; ++it)
    {
      coord_t out[nd];
      radiusTransform.apply(it->getCenter(), out);
      if (out[0] < radiusSquared)
      {
        signal += it->getSignal();
        errorSquared += it->getErrorSquared();
      }
    }

    this->releaseEvents();
  }

  //-----------------------------------------------------------------------------------------------
  /** Find the centroid of all events contained within by doing a weighted average
   * of their coordinates.
   *
   * @param radiusTransform :: nd-to-1 coordinate transformation that converts from these
   *        dimensions to the distance (squared) from the center of the sphere.
   * @param radiusSquared :: radius^2 below which to integrate
   * @param[out] centroid :: array of size [nd]; its centroid will be added
   * @param[out] signal :: set to the integrated signal
   */
  TMDE(
  void MDBox)::centroidSphere(CoordTransform & radiusTransform, const coord_t radiusSquared, coord_t * centroid, signal_t & signal) const
  {
    // If the box is cached to disk, you need to retrieve it
    const std::vector<MDE> & events = this->getConstEvents();
    typename std::vector<MDE>::const_iterator it = events.begin();
    typename std::vector<MDE>::const_iterator it_end = events.end();

    // For each MDLeanEvent
    for (; it != it_end; ++it)
    {
      coord_t out[nd];
      radiusTransform.apply(it->getCenter(), out);
      if (out[0] < radiusSquared)
      {
        double eventSignal = it->getSignal();
        signal += eventSignal;
        for (size_t d=0; d<nd; d++)
          centroid[d] += it->getCenter(d) * eventSignal;
      }
    }

    this->releaseEvents();
  }


}//namespace MDEvents

}//namespace Mantid

