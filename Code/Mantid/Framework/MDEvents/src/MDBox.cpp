#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDLeanEvent.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidKernel/DiskBuffer.h"

using Mantid::Kernel::DiskBuffer;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

//-----------------------------------------------------------------------------------------------
  /** Empty constructor */
  TMDE(MDBox)::MDBox()
   : MDBoxBase<MDE, nd>(),
     m_dataBusy(false), m_dataModified(false), m_dataAdded(false),
     m_fileIndexStart(0), m_fileNumEvents(0),
     m_onDisk(false), m_inMemory(true), m_bIsMasked(false)
  {
  }

  //-----------------------------------------------------------------------------------------------
  /** Constructor
   * @param splitter :: BoxController that controls how boxes split
   * @param depth :: splitting depth of the new box.
   */
  TMDE(MDBox)::MDBox(BoxController_sptr splitter, const size_t depth)
    : MDBoxBase<MDE, nd>(),
      m_dataBusy(false), m_dataModified(false), m_dataAdded(false),
      m_fileIndexStart(0), m_fileNumEvents(0),
      m_onDisk(false), m_inMemory(true), m_bIsMasked(false)
  {
    if (splitter->getNDims() != nd)
      throw std::invalid_argument("MDBox::ctor(): controller passed has the wrong number of dimensions.");
    this->m_BoxController = splitter;
    this->m_depth = depth;
    // Give it a fresh ID from the controller.
    this->setId( splitter->getNextId() );
  }

  //-----------------------------------------------------------------------------------------------
  /** Constructor
   * @param splitter :: BoxController that controls how boxes split
   * @param depth :: splitting depth of the new box.
   * @param extentsVector :: vector defining the extents
   */
  TMDE(MDBox)::MDBox(BoxController_sptr splitter, const size_t depth, const std::vector<Mantid::Geometry::MDDimensionExtents> & extentsVector)
      : MDBoxBase<MDE, nd>(extentsVector),
        m_dataBusy(false), m_dataModified(false), m_dataAdded(false),
        m_fileIndexStart(0), m_fileNumEvents(0),
        m_onDisk(false), m_inMemory(true), m_bIsMasked(false)
  {
    if (splitter->getNDims() != nd)
      throw std::invalid_argument("MDBox::ctor(): controller passed has the wrong number of dimensions.");
    this->m_BoxController = splitter;
    this->m_depth = depth;
    // Give it a fresh ID from the controller.
    this->setId( splitter->getNextId() );
  }


  //-----------------------------------------------------------------------------------------------
  /** Copy constructor */
  TMDE(MDBox)::MDBox(const MDBox<MDE,nd> & other)
   : MDBoxBase<MDE, nd>(other),
     data(other.data),
     m_dataBusy(other.m_dataBusy), m_dataModified(other.m_dataModified), m_dataAdded(other.m_dataAdded),
     m_fileIndexStart(other.m_fileIndexStart), m_fileNumEvents(other.m_fileNumEvents),
     m_onDisk(other.m_onDisk), m_inMemory(other.m_inMemory), m_bIsMasked(other.m_bIsMasked)
  {
  }


  //-----------------------------------------------------------------------------------------------
  /** Clear any points contained. */
  TMDE(
  void MDBox)::clear()
  {
    // Make sure the object is not in any of the disk MRUs, and mark any space it used as free
    if (this->m_BoxController->useWriteBuffer())
      this->m_BoxController->getDiskBuffer().objectDeleted(this, m_fileNumEvents);
    // Clear all contents
    this->m_signal = 0.0;
    this->m_errorSquared = 0.0;
    m_fileNumEvents = 0;
    data.clear();
    m_dataAdded = false;
  }

  //-----------------------------------------------------------------------------------------------
  /** Clear the data[] vector ONLY but does not change the file-backed settings.
   * Used to free up the memory in a file-backed workspace without removing the events from disk. */
  TMDE(
  void MDBox)::clearDataOnly() const
  {
    data.clear();
    vec_t().swap(data); // Linux trick to really free the memory
    m_inMemory = false;
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
  void MDBox)::getBoxes(std::vector<MDBoxBase<MDE,nd> *> & boxes, size_t /*maxDepth*/, bool /*leafOnly*/)
  {
    boxes.push_back(this);
  }

  //-----------------------------------------------------------------------------------------------
  /// Fill a vector with all the boxes up to a certain depth
  TMDE(
  void MDBox)::getBoxes(std::vector<MDBoxBase<MDE,nd> *> & boxes, size_t /*maxDepth*/, bool /*leafOnly*/, Mantid::Geometry::MDImplicitFunction * /*function*/)
  {
    boxes.push_back(this);
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this box */
  TMDE(
  uint64_t MDBox)::getNPoints() const
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
        // Mutex for disk access (prevent read/write at the same time)
        Kernel::Mutex & mutex = this->m_BoxController->getDiskBuffer().getFileMutex();
        mutex.lock();
        // Note that this APPENDS any events to the existing event list
        //  (in the event that addEvent() was called for a box that was on disk)
        try
        {
          MDE::loadVectorFromNexusSlab(data, file, m_fileIndexStart, m_fileNumEvents);
          mutex.unlock();
        }
        catch (std::exception &)
        {
          mutex.unlock();
          throw;
        }
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
      // The data vector is busy - can't release the memory yet
      this->m_dataBusy = true;
      // This access to data was NOT const, so it might have changed. We assume it has by setting m_dataModified to true.
      this->m_dataModified = true;

      // Tell the to-write buffer to write out the object (when no longer busy)
      this->m_BoxController->getDiskBuffer().toWrite(this);
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
      // The data vector is busy - can't release the memory yet
      this->m_dataBusy = true;
      // This access to data was const. Don't change the m_dataModified flag.

      // Tell the to-write buffer to write out the object (when no longer busy)
      this->m_BoxController->getDiskBuffer().toWrite(this);
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
      // If no write buffer is used, save it immediately if needed.
      if (!this->m_BoxController->useWriteBuffer())
        this->save();
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Call to save the data (if needed) and release the memory used.
   * Called from the DiskBuffer.
   */
  TMDE(
  void MDBox)::save() const
  {
    // Only save to disk when the access was non-const;
    //  or when you added events to a cached data
    if (m_dataModified || m_dataAdded)
    {
//      std::cout << "MDBox ID " << this->getId() << " being saved." << std::endl;

      // This will load and append events ONLY if needed.
      if (m_dataAdded)
        this->loadEvents();

      // This is the new size of the event list, possibly appended (if used AddEvent) or changed otherwise (non-const access)
      size_t newNumEvents = data.size();
      DiskBuffer & dbuf = this->m_BoxController->getDiskBuffer();
      if (newNumEvents != m_fileNumEvents)
      {
        // Event list changed size. The MRU can tell us where it best fits now.
        m_fileIndexStart = dbuf.relocate(m_fileIndexStart, m_fileNumEvents, newNumEvents);
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
    // Data was not modified
    m_dataModified = false;
    // Data was not added
    m_dataAdded = false;
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
   *
   * For MDBox, if MDBOX_TRACK_SIGNAL_WHEN_ADDING is defined,
   * then the signal/error is tracked on adding, so
   * this does nothing.
   */
  TMDE(
  void MDBox)::refreshCache(Kernel::ThreadScheduler * /*ts*/)
  {
#ifndef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    // Use the cached value if it is on disk
    if (m_inMemory)
    {
      // In memory - just sum up everything
      this->m_signal = 0;
      this->m_errorSquared = 0;

      typename std::vector<MDE>::const_iterator it_end = data.end();
      for(typename std::vector<MDE>::const_iterator it = data.begin(); it != it_end; ++it)
      {
        const MDE & event = *it;
        // Convert floats to doubles to preserve precision when adding them.
        this->m_signal += static_cast<signal_t>(event.getSignal());
        this->m_errorSquared += static_cast<signal_t>(event.getErrorSquared());
      }
    }
    else if (m_dataAdded)
    {
      // ADD the events that were added to the cache, without reloading the whole event list.
      typename std::vector<MDE>::const_iterator it_end = data.end();
      for(typename std::vector<MDE>::const_iterator it = data.begin(); it != it_end; ++it)
      {
        const MDE & event = *it;
        // Convert floats to doubles to preserve precision when adding them.
        this->m_signal += static_cast<signal_t>(event.getSignal());
        this->m_errorSquared += static_cast<signal_t>(event.getErrorSquared());
      }
    }
    /// TODO #4734: sum the individual weights of each event?
    this->m_totalWeight = static_cast<double>(this->getNPoints());
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
   * @param centroid [out] :: nd-sized array that will be set to the centroid.
   */
  TMDE(
  void MDBox)::calculateCentroid(coord_t * centroid) const
  {
    for (size_t d=0; d<nd; d++)
      centroid[d] = 0;

    // Signal was calculated before (when adding)
    // Keep 0.0 if the signal is null. This avoids dividing by 0.0
    if (this->m_signal == 0) return;

    typename std::vector<MDE>::const_iterator it_end = data.end();
    for(typename std::vector<MDE>::const_iterator it = data.begin(); it != it_end; ++it)
    {
      const MDE & event = *it;
      double signal = event.getSignal();
      for (size_t d=0; d<nd; d++)
      {
        // Total up the coordinate weighted by the signal.
        centroid[d] += event.getCenter(d) * static_cast<coord_t>(signal);
      }
    }

    // Normalize by the total signal
    for (size_t d=0; d<nd; d++)
      centroid[d] /= coord_t(this->m_signal);
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
    for(typename std::vector<MDE>::const_iterator it = data.begin(); it != it_end; ++it)
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
   * @param event :: reference to a MDEvent to add.
   * */
  TMDE(
  void MDBox)::addEvent( const MDE & event)
  {
    dataMutex.lock();
    this->data.push_back(event);

    // Yes, we added some data
    this->m_dataAdded = true;

//    // When we reach the split threshold exactly, track that the MDBox is too small
//    // We check on equality and not >= to only add a box once.
//    if (this->data.size() == this->m_BoxController->getSplitThreshold())
//    {
//      this->m_BoxController->addBoxToSplit(this);
//    }

#ifdef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    // Keep the running total of signal and error
    signal_t signal = static_cast<signal_t>(event.getSignal());
    this->m_signal += signal;
    this->m_errorSquared += static_cast<signal_t>(event.getErrorSquared());
#endif

#ifdef MDBOX_TRACKCENTROID_WHENADDING
    // Running total of the centroid
    for (size_t d=0; d<nd; d++)
      this->m_centroid[d] += event.getCenter(d) * signal;
#endif

    dataMutex.unlock();
  }

  //-----------------------------------------------------------------------------------------------
  /** Add a MDLeanEvent to the box, in a NON-THREAD-SAFE manner.
   * No lock is performed. This is only safe if no 2 threads will
   * try to add to the same box at the same time.
   *
   * @param event :: reference to a MDEvent to add.
   * */
  TMDE(
  void MDBox)::addEventUnsafe( const MDE & event)
  {
    this->data.push_back(event);
    this->m_dataAdded = true;

#ifdef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    // Keep the running total of signal and error
    double signal = static_cast<signal_t>(event.getSignal());
    this->m_signal += signal;
    this->m_errorSquared += static_cast<signal_t>(event.getErrorSquared());
#endif

#ifdef MDBOX_TRACKCENTROID_WHENADDING
    // Running total of the centroid
    for (size_t d=0; d<nd; d++)
      this->m_centroid[d] += event.getCenter(d) * signal;
#endif
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
  size_t MDBox)::addEventsPart(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at)
  {
    dataMutex.lock();
    typename std::vector<MDE>::const_iterator start = events.begin()+start_at;
    typename std::vector<MDE>::const_iterator end = events.begin()+stop_at;
    // Copy all the events
    this->data.insert(this->data.end(), start, end);

    // Yes, we added some data
    this->m_dataAdded = true;

#ifdef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    //Running total of signal/error
    for(typename std::vector<MDE>::const_iterator it = start; it != end; ++it)
    {
      double signal = static_cast<signal_t>(it->getSignal());
      this->m_signal += signal;
      this->m_errorSquared += static_cast<signal_t>(it->getErrorSquared());

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
  /** Add several events, within a given range, with no bounds checking,
   * and not in a thread-safe way
   *
   * @param events :: vector of events to be copied.
   * @param start_at :: index to start at in vector
   * @param stop_at :: index to stop at in vector (exclusive)
   * @return the number of events that were rejected (because of being out of bounds)
   */
  TMDE(
  size_t MDBox)::addEventsPartUnsafe(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at)
  {
    // The regular MDBox is just as safe/unsafe
    return this->addEventsPart(events, start_at, stop_at);
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
        // All dimensions are fully contained, so just return the cached total signal instead of counting.
        bin.m_signal += static_cast<signal_t>(this->m_signal);
        bin.m_errorSquared += static_cast<signal_t>(this->m_errorSquared);
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
        // Accumulate error and signal (as doubles, to preserve precision)
        bin.m_signal += static_cast<signal_t>(it->getSignal());
        bin.m_errorSquared += static_cast<signal_t>(it->getErrorSquared());
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
  void MDBox)::generalBin(MDBin<MDE,nd> & bin, Mantid::Geometry::MDImplicitFunction & function) const
  {
    UNUSED_ARG(bin);

    typename std::vector<MDE>::const_iterator it = data.begin();
    typename std::vector<MDE>::const_iterator it_end = data.end();
    // For each MDLeanEvent
    for (; it != it_end; ++it)
    {
      if (function.isPointContained(it->getCenter())) //HACK
      {
        // Accumulate error and signal
        bin.m_signal += static_cast<signal_t>(it->getSignal());
        bin.m_errorSquared += static_cast<signal_t>(it->getErrorSquared());
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
  void MDBox)::integrateSphere(Mantid::API::CoordTransform & radiusTransform, const coord_t radiusSquared, signal_t & signal, signal_t & errorSquared) const
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
        signal += static_cast<signal_t>(it->getSignal());
        errorSquared += static_cast<signal_t>(it->getErrorSquared());
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
  void MDBox)::centroidSphere(Mantid::API::CoordTransform & radiusTransform, const coord_t radiusSquared, coord_t * centroid, signal_t & signal) const
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
        coord_t eventSignal = static_cast<coord_t>(it->getSignal());
        signal += signal_t(eventSignal);
        for (size_t d=0; d<nd; d++)
          centroid[d] += it->getCenter(d) * eventSignal;
      }
    }

    this->releaseEvents();
  }


  //-----------------------------------------------------------------------------------------------
  /** Transform the dimensions contained in this box
   * x' = x*scaling + offset.
   *
   * @param scaling :: multiply each coordinate by this value.
   * @param offset :: after multiplying, add this offset.
   */
  TMDE(
  void MDBox)::transformDimensions(std::vector<double> & scaling, std::vector<double> & offset)
  {
    MDBoxBase<MDE,nd>::transformDimensions(scaling, offset);
    std::vector<MDE> & events = this->getEvents();
    typename std::vector<MDE>::iterator it;
    typename std::vector<MDE>::iterator it_end = events.end();
    for (it = events.begin(); it != it_end; ++it)
    {
      coord_t * center = it->getCenterNonConst();
      for (size_t d=0; d<nd; d++)
        center[d] = (center[d] * static_cast<coord_t>(scaling[d])) + static_cast<coord_t>(offset[d]);
    }
    this->releaseEvents();
  }

    ///Setter for masking the box
  TMDE(
  void MDBox)::mask()
  {
    m_bIsMasked = true;
  }

  ///Setter for unmasking the box
  TMDE(
  void MDBox)::unmask()
  {
    m_bIsMasked = false;
  }



}//namespace MDEvents

}//namespace Mantid

