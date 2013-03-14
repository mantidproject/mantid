#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDBoxSaveable.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDLeanEvent.h"
#include "MantidKernel/DiskBuffer.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/BoxCtrlChangesList.h"

using Mantid::Kernel::DiskBuffer;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{
  /* Internal TMP class to simplify building event constructor box for events and lean events using single interface*/
  template<typename MDE,size_t nd>
  struct IF
  {
  public:
      inline MDEvent<nd> EXEC(const signal_t Signal, const signal_t Error, const  coord_t *Coord,const uint16_t runIndex=0,const uint32_t detectorId=0)
      {
          return MDEvent<nd>(Signal,Error, runIndex, detectorId, Coord);
      }
  };

  template<size_t nd>
  struct IF<MDLeanEvent<nd>,nd>
  {
  public:
      inline MDLeanEvent<nd> EXEC(const signal_t Signal, const signal_t Error, const  coord_t *Coord,const uint16_t /*runIndex*/,const uint32_t /*detectorId*/)
      {
          return MDLeanEvent<nd>(Signal,Error,Coord);
      }
  };



  TMDE(MDBox)::~MDBox()
  {
      if(m_Saveable)delete m_Saveable;
  }
//-----------------------------------------------------------------------------------------------
  /** Empty constructor */
  TMDE(MDBox)::MDBox()
   : MDBoxBase<MDE, nd>(),
     m_Saveable(NULL),
     m_bIsMasked(false)
  {
  }

  //-----------------------------------------------------------------------------------------------
  /** Constructor
   * @param splitter :: BoxController that controls how boxes split
   * @param depth :: splitting depth of the new box.
   * @param boxSize :: size of reserve for data
   * @param boxID :: id for the given box
   */
  TMDE(MDBox)::MDBox(BoxController_sptr splitter, const size_t depth,int64_t boxSize,int64_t boxID)
    : MDBoxBase<MDE, nd>(),
      m_Saveable(NULL),
      m_bIsMasked(false)
  {
    if (splitter->getNDims() != nd)
      throw std::invalid_argument("MDBox::ctor(): controller passed has the wrong number of dimensions.");
    this->m_BoxController = splitter;
    this->m_depth = depth;

    if(boxID<0)  // Give it a fresh ID from the controller.
      this->setId( splitter->getNextId() );
    else         // somebody gives the ID on constructor
      this->setId(boxID);

    if(boxSize>0) data.reserve(boxSize);
   }

  //-----------------------------------------------------------------------------------------------
  /** Constructor
   * @param splitter :: BoxController that controls how boxes split
   * @param depth :: splitting depth of the new box.
   * @param extentsVector :: vector defining the extents
   * @param boxSize :: size of reserve for data
   * @param boxID :: id for the given box
   */
  TMDE(MDBox)::MDBox(BoxController_sptr splitter, const size_t depth, const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > & extentsVector,int64_t boxSize,int64_t boxID)
   :   MDBoxBase<MDE, nd>(extentsVector),
       m_Saveable(NULL),
       m_bIsMasked(false)
  {
    if (splitter->getNDims() != nd)
      throw std::invalid_argument("MDBox::ctor(): controller passed has the wrong number of dimensions.");
    this->m_BoxController = splitter;
    this->m_depth = depth;
    // Give it a fresh ID from the controller.
    if(boxID<0) // Give it a fresh ID from the controller.
      this->setId( splitter->getNextId() );
    else     // somebody gives the ID on constructor
      this->setId(boxID);

    if(boxSize>0) data.reserve(boxSize);
  }


  //-----------------------------------------------------------------------------------------------
  /** Copy constructor
   * @param other: MDBox object to copy from.
   */
  TMDE(MDBox)::MDBox(const MDBox<MDE,nd> & other,const Mantid::API::BoxController * otherBC)
     : MDBoxBase<MDE, nd>(other,otherBC),
     m_Saveable(NULL),
     data(other.data),
     m_bIsMasked(other.m_bIsMasked)
  {
      //TODO: inheriting Saveable logic on the basis of otherBC
  }


  //-----------------------------------------------------------------------------------------------
  /** Clear any points contained. */
  TMDE(
  void MDBox)::clear()
  {
    // Make sure the object is not in any of the disk MRUs, and mark any space it used as free
    //if (this->m_BoxController->useWriteBuffer())
    if(m_Saveable)
       this->m_BoxController->getDiskBuffer().objectDeleted(m_Saveable);
    // Clear all contents
    this->m_signal = 0.0;
    this->m_errorSquared = 0.0;

    this->clearDataFromMemory();

  }

  //-----------------------------------------------------------------------------------------------
  /** Clear the data[] vector ONLY but does not change the file-backed settings.
   * Used to free up the memory in a file-backed workspace without removing the events from disk. */
  TMDE(
  void MDBox)::clearDataFromMemory()
  {
    data.clear();
    vec_t().swap(data); // Linux trick to really free the memory
    // mark data unchanged
    this->resetDataChanges();
    if(m_Saveable)
    {
        m_Saveable->setLoaded(false);
        m_Saveable->setBusy(false);
    }

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
  TMDE(
  void MDBox)::getBoxes(std::vector<API::IMDNode *> & boxes, size_t /*maxDepth*/, bool /*leafOnly*/)
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
  TMDE(
  void MDBox)::getBoxes(std::vector<API::IMDNode *> & boxes, size_t /*maxDepth*/, bool /*leafOnly*/, Mantid::Geometry::MDImplicitFunction * /*function*/)
  {
    boxes.push_back(this);
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this box */
  TMDE(
  uint64_t MDBox)::getNPoints() const
  {
      if(!m_Saveable)
          return data.size();

          
       if (m_Saveable->wasSaved())
       {
           if (m_Saveable->isLoaded())
               return data.size();
            else // m_fileNumEvents
                return m_Saveable->getFileSize() + data.size();    
       }
       else
        return data.size();
  }


 
  //-----------------------------------------------------------------------------------------------
  /** Returns a reference to the events vector contained within.
   * VERY IMPORTANT: call MDBox::releaseEvents() when you are done accessing that data.
   */
  TMDE(
  std::vector< MDE > & MDBox)::getEvents()
  {
      if(!m_Saveable)
          return data;


      if (m_Saveable->wasSaved())
      {
        // Load and concatenate the events if needed
        m_Saveable->load();
      // The data vector is busy - can't release the memory yet
        m_Saveable->setBusy();
      // Tell the to-write buffer to write out/discard the object (when no longer busy)
        this->m_BoxController->getDiskBuffer().toWrite(m_Saveable);
      }
    // else: do nothing if the events are already in memory.
      // the non-const access to events assumes that the data will be modified;
       m_Saveable->setDataChanged();
      return data;
  }

  TMDE(
  const std::vector<MDE> & MDBox)::getEvents()const 
  {
      return getConstEvents();
  }
  //-----------------------------------------------------------------------------------------------
  /** Returns a const reference to the events vector contained within.
   * VERY IMPORTANT: call MDBox::releaseEvents() when you are done accessing that data.
   */
  TMDE(
  const std::vector<MDE> & MDBox)::getConstEvents()const 
  {
    if(!m_Saveable)
          return data;

    if (m_Saveable->wasSaved())
    {
      // Load and concatenate the events if needed
        m_Saveable->load();  // this will set isLoaded to true if not already loaded;
        // The data vector is busy - can't release the memory yet
        m_Saveable->setBusy();

      // This access to data was const. Don't change the m_dataModified flag.

      // Tell the to-write buffer to discard the object (when no longer busy) as it has not been modified
        this->m_BoxController->getDiskBuffer().toWrite(m_Saveable);
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
  void MDBox)::releaseEvents() 
  {
    // Data vector is no longer busy.
      if(m_Saveable)
          m_Saveable->setBusy(false);
  }


   /** The method to convert events in a box into a table of coodrinates/signal/errors casted into coord_t type 
     *   Used to save events from plain binary file
     *   @returns coordTable -- vector of events parameters
     *   @return nColumns    -- number of parameters for each event
     */
  TMDE(
  void MDBox)::getEventsData(std::vector<coord_t> &coordTable,size_t &nColumns)const
  {
      double signal,errorSq;
      MDE::eventsToData(this->data,coordTable,nColumns,signal,errorSq);
      this->m_signal       = static_cast<signal_t>(signal);
      this->m_errorSquared = static_cast<signal_t>(errorSq);
  };
    /** The method to convert the table of data into vector of events 
     *   Used to load events from plain binary file
     *   @param coordTable -- vector of events parameters
     *   @param nColumns    -- number of parameters for each event
     */
  TMDE(
  void MDBox)::setEventsData(const std::vector<coord_t> &coordTable)
  {
      MDE::dataToEvents(coordTable, this->data);
  };

 

  //-----------------------------------------------------------------------------------------------
  /** Allocate and return a vector with a copy of all events contained
   */
  TMDE(
  std::vector< MDE > * MDBox)::getEventsCopy()
  {
     if(m_Saveable)
     {
     }
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

   * beware, that it wrongly accumulates signal and error when part of the data is on file and 
   * and some recent data were not saved to the HDD before adding new data
   * This actually means that refreshCache has to be called only once in events adding process
   */
  TMDE(
  void MDBox)::refreshCache(Kernel::ThreadScheduler * /*ts*/)
  {

    // Use the cached value if it is on disk
    double signalSum(0);
    double errorSum(0);

    if (this->wasSaved()) // There are possible problems with disk buffered events, as saving calculates averages and these averages has to be added to memory contents
    {
      if(!m_isLoaded)  // events were saved,  averages calculated and stored 
      {
        // the partial data were not loaded from HDD but their averages should be calculated when loaded. Add them 
         signalSum +=double(this->m_signal);
         errorSum  +=double(this->m_errorSquared);
      }
    }
    // calculate all averages from memory
    typename std::vector<MDE>::const_iterator it_end = data.end();
    for(typename std::vector<MDE>::const_iterator it = data.begin(); it != it_end; ++it)
    {
        const MDE & event = *it;
        // Convert floats to doubles to preserve precision when adding them.
        signalSum += static_cast<signal_t>(event.getSignal());
        errorSum += static_cast<signal_t>(event.getErrorSquared());
    }

    this->m_signal = signal_t(signalSum);
    this->m_errorSquared=signal_t(errorSum);

    /// TODO #4734: sum the individual weights of each event?
    this->m_totalWeight = static_cast<double>(this->getNPoints());

  }


  //-----------------------------------------------------------------------------------------------
  /** Calculate and ccache the centroid of this box.
   */
  TMDE(
  void MDBox)::refreshCentroid(Kernel::ThreadScheduler * /*ts*/)
  {
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
      const MDE & Evnt = *it;
      double signal = Evnt.getSignal();
      for (size_t d=0; d<nd; d++)
      {
        // Total up the coordinate weighted by the signal.
        centroid[d] += Evnt.getCenter(d) * static_cast<coord_t>(signal);
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
      const MDE & Evnt = *it;
      for (size_t d=0; d<nd; d++)
      {
        stats[d].addPoint( Evnt.getCenter(d) );
      }
    }
  }
  /** Create event from the input data and add it to the box.
   * @param point :: reference to the  MDEvent coordinates
   * @param Signal  :: events signal
   * @param errorSq :: events Error squared
   * @param index   run  index
   * @param index   detector's ID
   * */
   TMDE(
   void MDBox)::addEvent(const std::vector<coord_t> &point, signal_t Signal, signal_t errorSq,uint16_t runIndex,uint32_t detectorId)
   {
       this->addEvent(IF<MDE,nd>::EXEC(Signal, errorSq, &point[0],runIndex, detectorId));
   }


  //-----------------------------------------------------------------------------------------------
  /** Add a MDLeanEvent to the box.
   * @param Evnt :: reference to a MDEvent to add.
   * */
  TMDE(
  void MDBox)::addEvent( const MDE & Evnt)
  {
    dataMutex.lock();
    this->data.push_back(Evnt );

//    // When we reach the split threshold exactly, track that the MDBox is too small
//    // We check on equality and not >= to only add a box once.
//    if (this->data.size() == this->m_BoxController->getSplitThreshold())
//    {
//      this->m_BoxController->addBoxToSplit(this);
//    }

    dataMutex.unlock();
  }
  /** Create MD MDEvent amd add it to the box.
   // add a single event and set pointer to the box which needs splitting (if one actually need) 

   * @param point :: reference to a MDEvent to add.
   * @param index :: current index for box
   */

   TMDE(
   void MDBox)::addAndTraceEvent(const std::vector<coord_t> &point, signal_t Signal, signal_t errorSq,uint16_t runIndex,uint32_t detectorId,size_t index)
   {
       this->addAndTraceEvent(IF<MDE,nd>::EXEC(Signal, errorSq, &point[0], runIndex, detectorId));
   }
  //-----------------------------------------------------------------------------------------------
  /** Add a MDEvent to the box.
   // add a single event and set pounter to the box which needs splitting (if one actually need) 

   * @param point :: reference to a MDEvent to add.
   * @param index :: current index for box
   */
  TMDE(
  void MDBox)::addAndTraceEvent( const MDE & point, size_t index)
  {
    dataMutex.lock();
    this->data.push_back(point);

    dataMutex.unlock();

    // When we reach the split threshold exactly, track that the MDBox is too small
    // We check on equality and not >= to only add a box once.
    if (this->data.size() == this->m_BoxController->getSplitThreshold())
    {     
       auto BoxCtrl = dynamic_cast<BoxCtrlChangesList<MDBoxToChange<MDE,nd> >*>(this->m_BoxController.get());
       BoxCtrl->addBoxToSplit(MDBoxToChange<MDE,nd>(this,index));

    }

  }
 
  //-----------------------------------------------------------------------------------------------
  /**Create MDEvent and add it to the box, in a NON-THREAD-SAFE manner.
   * No lock is performed. This is only safe if no 2 threads will
   * try to add to the same box at the same time.
   *
   * @param Evnt :: reference to a MDEvent to add.
   * */
  TMDE(
  void MDBox)::addEventUnsafe(const std::vector<coord_t> &point, signal_t Signal, signal_t errorSq,uint16_t runIndex,uint32_t detectorId)
  {
       this->addEventUnsafe(IF<MDE,nd>::EXEC(Signal, errorSq, &point[0], runIndex, detectorId));
  }

  //-----------------------------------------------------------------------------------------------
  /** Add a MDLeanEvent to the box, in a NON-THREAD-SAFE manner.
   * No lock is performed. This is only safe if no 2 threads will
   * try to add to the same box at the same time.
   *
   * @param Evnt :: reference to a MDEvent to add.
   * */
  TMDE(
  void MDBox)::addEventUnsafe( const MDE & Evnt)
  {
    this->data.push_back(Evnt );

  }


  /** Create and Add several events. No bounds checking is made!
   *
   */
  TMDE(
  size_t MDBox)::addEvents(const std::vector<signal_t> &sigErrSq,const  std::vector<coord_t> &Coord,const std::vector<uint16_t> &runIndex,const std::vector<uint32_t> &detectorId)
  {
    size_t nEvents = sigErrSq.size()/2;
    size_t nExisiting = data.size();
    data.reserve(nExisiting+nEvents);
    dataMutex.lock();
    for(size_t i=0;i<nEvents;i++)
    {
        this->data.push_back(IF<MDE,nd>::EXEC(sigErrSq[2*i],sigErrSq[2*i+1],&Coord[i*nd],runIndex[i], detectorId[i]));
    }
    dataMutex.unlock();

  }

 
   //virtual size_t addEventsPart(const std::vector<coord_t> &coords,const signal_t *Signal,const signal_t *errorSq,const  uint16_t *runIndex,const uint32_t *detectorId, const size_t start_at, const size_t stop_at);
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
    // it is constant access, so no saving or fiddling with the buffer is needed. Events just can be dropped if necessary
    this->setBusy(false);
    //this->releaseEvents();
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
    // it is constant access, so no saving or fiddling with the buffer is needed. Events just can be dropped if necessary
    this->setBusy(false);
    //this->releaseEvents();
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
    // it is constant access, so no saving or fiddling with the buffer is needed. Events just can be dropped if necessary
    this->setBusy(false);
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
    this->setBusy(false);
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

