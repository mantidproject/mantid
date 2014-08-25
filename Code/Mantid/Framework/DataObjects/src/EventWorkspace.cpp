#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/DateAndTime.h"
#include <limits>
#include <numeric>
#include "MantidAPI/ISpectrum.h"
#include "MantidKernel/CPUTimer.h"

using namespace boost::posix_time;
using Mantid::API::ISpectrum;
using Mantid::Kernel::DateAndTime;

namespace Mantid
{
  namespace DataObjects
  {
    namespace
    {
      // static logger
      Kernel::Logger g_log("EventWorkspace");
    }

    DECLARE_WORKSPACE(EventWorkspace)

    using Kernel::Exception::NotImplementedError;
    using std::size_t;
    using namespace Mantid::Kernel;

    //---- Constructors -------------------------------------------------------------------
    EventWorkspace::EventWorkspace() :
        mru(new EventWorkspaceMRU)
    {
    }

    EventWorkspace::~EventWorkspace()
    {
      delete mru;

      for (auto i = data.begin(); i != this->data.end(); ++i)
      {
        delete (*i);
      }
    }

    //-----------------------------------------------------------------------------
    /** Returns true if the EventWorkspace is safe for multithreaded operations.
     */
    bool EventWorkspace::threadSafe() const
    {
      // Since there is a mutex lock around sorting, EventWorkspaces are always safe.
      return true;
    }

    //-----------------------------------------------------------------------------
    /** Initialize the pixels
     *  @param NVectors :: The number of vectors/histograms/detectors in the workspace. Does not need
     *         to be set, but needs to be > 0
     *  @param XLength :: The number of X data points/bin boundaries in each vector (ignored)
     *  @param YLength :: The number of data/error points in each vector (ignored)
     */
    void EventWorkspace::init(const std::size_t &NVectors, const std::size_t &XLength,
        const std::size_t &YLength)
    {
      (void) YLength; //Avoid compiler warning

      // Check validity of arguments
      if (NVectors <= 0)
      {
        throw std::out_of_range("Negative or 0 Number of Pixels specified to EventWorkspace::init");
      }
      //Initialize the data
      m_noVectors = NVectors;
      data.resize(m_noVectors, NULL);
      //Make sure SOMETHING exists for all initialized spots.
      for (size_t i = 0; i < m_noVectors; i++)
        data[i] = new EventList(mru, specid_t(i));

      // Set each X vector to have one bin of 0 & extremely close to zero
      MantidVecPtr xVals;
      MantidVec & x = xVals.access();
      x.resize(2, 0.0);
      // Move the rhs very,very slightly just incase something doesn't like them being the same
      x[1] = std::numeric_limits<double>::min();
      this->setAllX(xVals);

      //Create axes.
      m_axes.resize(2);
      m_axes[0] = new API::RefAxis(XLength, this);
      m_axes[1] = new API::SpectraAxis(this);
    }

    //-----------------------------------------------------------------------------
    /**
     * Copy all of the data (event lists) from the source workspace to this workspace.
     *
     * @param source: EventWorkspace from which we are taking data.
     * @param sourceStartWorkspaceIndex: index in the workspace of source where we start
     *          copying the data. This index will be 0 in the "this" workspace.
     *          Default: -1, meaning copy all.
     * @param sourceEndWorkspaceIndex: index in the workspace of source where we stop.
     *          It is inclusive = source[sourceEndWorkspaceIndex[ WILL be copied.
     *          Default: -1, meaning copy all.
     *
     */
    void EventWorkspace::copyDataFrom(const EventWorkspace& source,
        std::size_t sourceStartWorkspaceIndex, std::size_t sourceEndWorkspaceIndex)
    {
      //Start with nothing.
      this->clearData(); //properly de-allocates memory!

      //Copy the vector of EventLists
      EventListVector source_data = source.data;
      EventListVector::iterator it;
      EventListVector::iterator it_start = source_data.begin();
      EventListVector::iterator it_end = source_data.end();
      size_t source_data_size = source_data.size();

      //Do we copy only a range?
      if (sourceEndWorkspaceIndex == size_t(-1))
        sourceEndWorkspaceIndex = source_data_size - 1;
      if ((sourceStartWorkspaceIndex < source_data_size) && (sourceEndWorkspaceIndex < source_data_size)
          && (sourceEndWorkspaceIndex >= sourceStartWorkspaceIndex))
      {
        it_start += sourceStartWorkspaceIndex;
        it_end = source_data.begin() + sourceEndWorkspaceIndex + 1;
      }

      for (it = it_start; it != it_end; ++it)
      {
        //Create a new event list, copying over the events
        EventList * newel = new EventList(**it);
        // Make sure to update the MRU to point to THIS event workspace.
        newel->setMRU(this->mru);
        this->data.push_back(newel);
      }
      //Save the number of vectors
      m_noVectors = this->data.size();

      this->clearMRU();
    }

    //-----------------------------------------------------------------------------
    /// The total size of the workspace
    /// @returns the number of single indexable items in the workspace
    size_t EventWorkspace::size() const
    {
      return this->data.size() * this->blocksize();
    }

    //-----------------------------------------------------------------------------
    /// Get the blocksize, aka the number of bins in the histogram
    /// @returns the number of bins in the Y data
    size_t EventWorkspace::blocksize() const
    {
      // Pick the first pixel to find the blocksize.
      EventListVector::const_iterator it = data.begin();
      if (it == data.end())
      {
        throw std::range_error(
            "EventWorkspace::blocksize, no pixels in workspace, therefore cannot determine blocksize (# of bins).");
      }
      else
      {
        return (*it)->histogram_size();
      }
    }

    //-----------------------------------------------------------------------------
    /** Get the number of histograms, usually the same as the number of pixels or detectors.
     @returns the number of histograms / event lists
     */
    size_t EventWorkspace::getNumberHistograms() const
    {
      return this->data.size();
    }

    //--------------------------------------------------------------------------------------------
    /// Return the underlying ISpectrum ptr at the given workspace index.
    Mantid::API::ISpectrum * EventWorkspace::getSpectrum(const size_t index)
    {
      if (index >= m_noVectors)
        throw std::range_error("EventWorkspace::getSpectrum, workspace index out of range");
      return data[index];
    }

    /// Return the underlying ISpectrum ptr at the given workspace index.
    const Mantid::API::ISpectrum * EventWorkspace::getSpectrum(const size_t index) const
    {
      if (index >= m_noVectors)
        throw std::range_error("EventWorkspace::getSpectrum, workspace index out of range");
      return data[index];
    }

    //-----------------------------------------------------------------------------

    double EventWorkspace::getTofMin() const
    {
      return this->getEventXMin();
    }

    double EventWorkspace::getTofMax() const
    {
      return this->getEventXMax();
    }

    /**
     Get the minimum pulse time for events accross the entire workspace.
     @return minimum pulse time as a DateAndTime.
     */
    DateAndTime EventWorkspace::getPulseTimeMin() const
    {
      // set to crazy values to start
      Mantid::Kernel::DateAndTime tMin = DateAndTime::maximum();
      size_t numWorkspace = this->data.size();
      DateAndTime temp;
      for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++)
      {
        const EventList &evList = this->getEventList(workspaceIndex);
        temp = evList.getPulseTimeMin();
        if (temp < tMin)
          tMin = temp;
      }
      return tMin;
    }

    /**
     Get the maximum pulse time for events accross the entire workspace.
     @return maximum pulse time as a DateAndTime.
     */
    DateAndTime EventWorkspace::getPulseTimeMax() const
    {
      // set to crazy values to start
      Mantid::Kernel::DateAndTime tMax = DateAndTime::minimum();
      size_t numWorkspace = this->data.size();
      DateAndTime temp;
      for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++)
      {
        const EventList &evList = this->getEventList(workspaceIndex);
        temp = evList.getPulseTimeMax();
        if (temp > tMax)
          tMax = temp;
      }
      return tMax;
    }

    /**
     Get the minimum time at sample for events across the entire workspace.
     @param tofOffset :: Time of flight offset. defaults to zero.
     @return minimum time at sample as a DateAndTime.
     */
    DateAndTime EventWorkspace::getTimeAtSampleMin(double tofOffset) const
    {
      auto instrument = this->getInstrument();
      auto sample = instrument->getSample();
      auto source = instrument->getSource();
      const double L1 = sample->getDistance(*source.get());

      // set to crazy values to start
      Mantid::Kernel::DateAndTime tMin = DateAndTime::maximum();
      size_t numWorkspace = this->data.size();
      DateAndTime temp;
      for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++)
      {
        const double L2 = this->getDetector(workspaceIndex)->getDistance(*sample.get());
        const double tofFactor = L1 / (L1 + L2);

        const EventList &evList = this->getEventList(workspaceIndex);
        temp = evList.getTimeAtSampleMin(tofFactor, tofOffset);
        if (temp < tMin)
          tMin = temp;
      }
      return tMin;
    }

    /**
     Get the maximum time at sample for events across the entire workspace.
     @param tofOffset :: Time of flight offset. defaults to zero.
     @return maximum time at sample as a DateAndTime.
     */
    DateAndTime EventWorkspace::getTimeAtSampleMax(double tofOffset) const
    {
      auto instrument = this->getInstrument();
      auto sample = instrument->getSample();
      auto source = instrument->getSource();
      const double L1 = sample->getDistance(*source.get());

      // set to crazy values to start
      Mantid::Kernel::DateAndTime tMax = DateAndTime::minimum();
      size_t numWorkspace = this->data.size();
      DateAndTime temp;
      for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++)
      {
        const double L2 = this->getDetector(workspaceIndex)->getDistance(*sample.get());
        const double tofFactor = L1 / (L1 + L2);

        const EventList &evList = this->getEventList(workspaceIndex);
        temp = evList.getTimeAtSampleMax(tofFactor, tofOffset);
        if (temp > tMax)
          tMax = temp;
      }
      return tMax;
    }

    /**
     * Get them minimum x-value for the events themselves, ignoring the histogram
     * representation.
     *
     * @return The minimum x-value for the all events.
     *
     * This does copy some of the code from getEventXMinXMax, but that is because
     * getting both min and max then throwing away the max is significantly slower
     * on an unsorted event list.
     */
    double EventWorkspace::getEventXMin() const
    {
      // set to crazy values to start
      double xmin = std::numeric_limits<double>::max();
      size_t numWorkspace = this->data.size();
      for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++)
      {
        const EventList &evList = this->getEventList(workspaceIndex);
        const double temp = evList.getTofMin();
        if (temp < xmin)
          xmin = temp;
      }
      return xmin;
    }

    /**
     * Get them maximum x-value for the events themselves, ignoring the histogram
     * representation.
     *
     * @return The maximum x-value for the all events.
     *
     * This does copy some of the code from getEventXMinXMax, but that is because
     * getting both min and max then throwing away the min is significantly slower
     * on an unsorted event list.
     */
    double EventWorkspace::getEventXMax() const
    {
      // set to crazy values to start
      double xmax = -1.0 * std::numeric_limits<double>::max();
      size_t numWorkspace = this->data.size();
      for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++)
      {
        const EventList &evList = this->getEventList(workspaceIndex);
        const double temp = evList.getTofMax();
        if (temp > xmax)
          xmax = temp;
      }
      return xmax;
    }

    /**
     * Get them minimum and maximum x-values for the events themselves, ignoring the
     * histogram representation. Since this does not modify the sort order, the method
     * will run significantly faster on a TOF_SORT event list.
     */
    void EventWorkspace::getEventXMinMax(double &xmin, double &xmax) const
    {
      // set to crazy values to start
      xmin = std::numeric_limits<double>::max();
      xmax = -1.0 * xmin;
      size_t numWorkspace = this->data.size();
      for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++)
      {
        const EventList &evList = this->getEventList(workspaceIndex);
        double temp = evList.getTofMin();
        if (temp < xmin)
          xmin = temp;
        temp = evList.getTofMax();
        if (temp > xmax)
          xmax = temp;
      }
    }

    //-----------------------------------------------------------------------------
    /// The total number of events across all of the spectra.
    /// @returns The total number of events
    size_t EventWorkspace::getNumberEvents() const
    {
      size_t total = 0;
      for (EventListVector::const_iterator it = this->data.begin(); it != this->data.end(); ++it)
      {
        total += (*it)->getNumberEvents();
      }
      return total;
    }

    //-----------------------------------------------------------------------------
    /** Get the EventType of the most-specialized EventList in the workspace
     *
     * @return the EventType of the most-specialized EventList in the workspace
     */
    Mantid::API::EventType EventWorkspace::getEventType() const
    {
      Mantid::API::EventType out = Mantid::API::TOF;
      for (EventListVector::const_iterator it = this->data.begin(); it != this->data.end(); ++it)
      {
        Mantid::API::EventType thisType = (*it)->getEventType();
        if (static_cast<int>(out) < static_cast<int>(thisType))
        {
          out = thisType;
          // This is the most-specialized it can get.
          if (out == Mantid::API::WEIGHTED_NOTIME)
            return out;
        }
      }
      return out;
    }

    //-----------------------------------------------------------------------------
    /** Switch all event lists to the given event type
     *
     * @param type :: EventType to switch to
     */
    void EventWorkspace::switchEventType(const Mantid::API::EventType type)
    {
      for (EventListVector::const_iterator it = this->data.begin(); it != this->data.end(); ++it)
      {
        (*it)->switchTo(type);
      }
    }

    //-----------------------------------------------------------------------------
    /// Returns true always - an EventWorkspace always represents histogramm-able data
    /// @returns If the data is a histogram - always true for an eventWorkspace
    bool EventWorkspace::isHistogramData() const
    {
      return true;
    }

    //-----------------------------------------------------------------------------
    /** Return how many entries in the Y MRU list are used.
     * Only used in tests. It only returns the 0-th MRU list size.
     * @return :: number of entries in the MRU list.
     */
    size_t EventWorkspace::MRUSize() const
    {
      return mru->MRUSize();
    }

    //-----------------------------------------------------------------------------
    /** Clears the MRU lists */
    void EventWorkspace::clearMRU() const
    {
      mru->clear();
    }

    //-----------------------------------------------------------------------------
    /** Clear the data[] vector and delete
     * any EventList objects in it
     */
    void EventWorkspace::clearData()
    {
      m_noVectors = data.size();
      for (size_t i = 0; i < m_noVectors; i++)
      {
        delete data[i];
      }
      data.clear();
      m_noVectors = 0;
    }

    //-----------------------------------------------------------------------------
    /// Returns the amount of memory used in bytes
    size_t EventWorkspace::getMemorySize() const
    {
      size_t total = 0;

      //TODO: Add the MRU buffer

      // Add the memory from all the event lists
      for (EventListVector::const_iterator it = this->data.begin(); it != this->data.end(); ++it)
      {
        total += (*it)->getMemorySize();
      }

      total += run().getMemorySize();

      total += this->getMemorySizeForXAxes();

      // Return in bytes
      return total;
    }

    //-----------------------------------------------------------------------------
    // --- Data Access ----
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    /** Get an EventList object at the given workspace index number
     * @param workspace_index :: The histogram workspace index number.
     * @returns A reference to the eventlist
     */
    EventList& EventWorkspace::getEventList(const std::size_t workspace_index)
    {
      EventList * result = data[workspace_index];
      if (!result)
        throw std::runtime_error("EventWorkspace::getEventList: NULL EventList found.");
      else
        return *result;
    }

    //-----------------------------------------------------------------------------
    /** Get a const EventList object at the given workspace index number
     * @param workspace_index :: The workspace index number.
     * @returns A const reference to the eventlist
     */
    const EventList& EventWorkspace::getEventList(const std::size_t workspace_index) const
    {
      EventList * result = data[workspace_index];
      if (!result)
        throw std::runtime_error("EventWorkspace::getEventList (const): NULL EventList found.");
      else
        return *result;
    }

    //-----------------------------------------------------------------------------
    /** Get an EventList pointer at the given workspace index number
     * @param workspace_index :: index into WS
     * @return an EventList pointer at the given workspace index number
     */
    EventList * EventWorkspace::getEventListPtr(const std::size_t workspace_index)
    {
      return data[workspace_index];
    }

    //-----------------------------------------------------------------------------
    /** Either return an existing EventList from the list, or
     * create a new one if needed and expand the list.
     *  to finalize the stuff that needs to.
     **
     * @param workspace_index :: The workspace index number.
     * @return An event list (new or existing) at the index provided
     */
    EventList& EventWorkspace::getOrAddEventList(const std::size_t workspace_index)
    {
      size_t old_size = data.size();
      if (workspace_index >= old_size)
      {
        //Increase the size of the eventlist lists.
        for (size_t wi = old_size; wi <= workspace_index; wi++)
        {
          //Need to make a new one!
          EventList * newel = new EventList(mru, specid_t(wi));
          //Add to list
          this->data.push_back(newel);
        }
        m_noVectors = data.size();
      }

      //Now it should be safe to return the value
      EventList * result = data[workspace_index];
      if (!result)
        throw std::runtime_error("EventWorkspace::getOrAddEventList: NULL EventList found.");
      else
        return *result;
    }

    /** Resizes the workspace to contain the number of spectra/events lists given.
     *  Any existing eventlists will be cleared first.
     *  Spectrum numbers will be set to count from 1
     *  @param numSpectra The number of spectra to resize the workspace to
     */
    void EventWorkspace::resizeTo(const std::size_t numSpectra)
    {
      // Remove all old EventLists and resize the vector
      this->clearData();
      data.resize(numSpectra);
      m_noVectors = numSpectra;
      for (size_t i = 0; i < numSpectra; ++i)
      {
        data[i] = new EventList(mru, static_cast<specid_t>(i + 1));
      }

      // Put on a default set of X vectors, with one bin of 0 & extremely close to zero
      MantidVecPtr xVals;
      MantidVec & x = xVals.access();
      x.resize(2, 0.0);
      // Move the rhs very,very slightly just incase something doesn't like them being the same
      x[1] = std::numeric_limits<double>::min();
      this->setAllX(xVals);

      //Clearing the MRU list is a good idea too.
      this->clearMRU();
    }

    /** Expands the workspace to a number of spectra corresponding to the number of
     *  pixels/detectors (not including monitors) contained in the instrument attached
     *  to the workspace.
     *  All events lists will be empty after calling this method. Spectrum numbers will
     *  count from 1 and detector IDs will be ordered as they are in the instrument.
     */
    void EventWorkspace::padSpectra()
    {
      const std::vector<detid_t> pixelIDs = getInstrument()->getDetectorIDs(true);

      resizeTo(pixelIDs.size());

      for (size_t i = 0; i < pixelIDs.size(); ++i)
      {
        getSpectrum(i)->setDetectorID(pixelIDs[i]);
      }
    }

    void EventWorkspace::deleteEmptyLists()
    {
      // figure out how much data to copy
      size_t orig_length = this->data.size();
      size_t new_length = 0;
      for (size_t i = 0; i < orig_length; i++)
      {
        if (!(this->data[i]->empty()))
          new_length++;
      }

      // copy over the data
      EventListVector notEmpty;
      notEmpty.reserve(new_length);
      for (size_t i = 0; i < orig_length; i++)
      {
        if (!(this->data[i]->empty()))
          notEmpty.push_back(this->data[i]);
        else
          delete this->data[i];
      }

      // replace the old vector
      this->data.swap(notEmpty);

      this->m_noVectors = this->data.size();

      //Clearing the MRU list is a good idea too.
      this->clearMRU();
    }

    //-----------------------------------------------------------------------------
    /// Return the data X vector at a given workspace index
    /// Note: the MRUlist should be cleared before calling getters for the Y or E data
    /// @param index :: the workspace index to return
    /// @returns A reference to the vector of binned X values
    MantidVec& EventWorkspace::dataX(const std::size_t index)
    {
      if (index >= this->m_noVectors)
        throw std::range_error("EventWorkspace::dataX, histogram number out of range");
      return this->data[index]->dataX();
    }

    /// Return the data X error vector at a given workspace index
    /// Note: the MRUlist should be cleared before calling getters for the Y or E data
    /// @param index :: the workspace index to return
    /// @returns A reference to the vector of binned error values
    MantidVec& EventWorkspace::dataDx(const std::size_t index)
    {
      if (index >= this->m_noVectors)
        throw std::range_error("EventWorkspace::dataDx, histogram number out of range");
      return this->data[index]->dataDx();
    }

    /// Return the data Y vector at a given workspace index
    /// Note: these non-const access methods will throw NotImplementedError
    /// @param index :: the workspace index to return
    /// @returns A reference to the vector of binned Y values
    MantidVec& EventWorkspace::dataY(const std::size_t index)
    {
      if (index >= this->m_noVectors)
        throw std::range_error("EventWorkspace::dataY, histogram number out of range");
      throw NotImplementedError(
          "EventWorkspace::dataY cannot return a non-const array: you can't modify the histogrammed data in an EventWorkspace!");
    }

    /// Return the data E vector at a given workspace index
    /// Note: these non-const access methods will throw NotImplementedError
    /// @param index :: the workspace index to return
    /// @returns A reference to the vector of binned error values
    MantidVec& EventWorkspace::dataE(const std::size_t index)
    {
      if (index >= this->m_noVectors)
        throw std::range_error("EventWorkspace::dataE, histogram number out of range");
      throw NotImplementedError(
          "EventWorkspace::dataE cannot return a non-const array: you can't modify the histogrammed data in an EventWorkspace!");
    }

    //-----------------------------------------------------------------------------
    // --- Const Data Access ----
    //-----------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    /** @return the const data X vector at a given workspace index
     * @param index :: workspace index   */
    const MantidVec& EventWorkspace::dataX(const std::size_t index) const
    {
      if (index >= this->m_noVectors)
        throw std::range_error("EventWorkspace::dataX, histogram number out of range");
      return this->data[index]->constDataX();
    }

    /** @return the const data X error vector at a given workspace index
     * @param index :: workspace index   */
    const MantidVec& EventWorkspace::dataDx(const std::size_t index) const
    {
      if (index >= this->m_noVectors)
        throw std::range_error("EventWorkspace::dataDx, histogram number out of range");
      return this->data[index]->readDx();
    }

    //---------------------------------------------------------------------------
    /** @return the const data Y vector at a given workspace index
     * @param index :: workspace index   */
    const MantidVec& EventWorkspace::dataY(const std::size_t index) const
    {
      if (index >= this->m_noVectors)
        throw std::range_error("EventWorkspace::dataY, histogram number out of range");
      const MantidVec& out = this->data[index]->constDataY();
      return out;
    }

    //---------------------------------------------------------------------------
    /** @return the const data E (error) vector at a given workspace index
     * @param index :: workspace index   */
    const MantidVec& EventWorkspace::dataE(const std::size_t index) const
    {
      if (index >= this->m_noVectors)
        throw std::range_error("EventWorkspace::dataE, histogram number out of range");
      const MantidVec& out = this->data[index]->constDataE();
      return out;
    }

    //---------------------------------------------------------------------------
    /** @return a pointer to the X data vector at a given workspace index
     * @param index :: workspace index   */
    Kernel::cow_ptr<MantidVec> EventWorkspace::refX(const std::size_t index) const
    {
      if (index >= this->m_noVectors)
        throw std::range_error("EventWorkspace::refX, histogram number out of range");
      return this->data[index]->ptrX();
    }

    //---------------------------------------------------------------------------
    /** Using the event data in the event list, generate a histogram of it w.r.t TOF.
     *
     * @param index :: workspace index to generate
     * @param X :: input X vector of the bin boundaries.
     * @param Y :: output vector to be filled with the Y data.
     * @param E :: output vector to be filled with the Error data (optionally)
     * @param skipError :: if true, the error vector is NOT calculated.
     *        This may save some processing time.
     */
    void EventWorkspace::generateHistogram(const std::size_t index, const MantidVec& X, MantidVec& Y,
        MantidVec& E, bool skipError) const
    {
      if (index >= this->m_noVectors)
        throw std::range_error("EventWorkspace::generateHistogram, histogram number out of range");
      this->data[index]->generateHistogram(X, Y, E, skipError);
    }

    //---------------------------------------------------------------------------
    /** Using the event data in the event list, generate a histogram of it w.r.t PULSE TIME.
     *
     * @param index :: workspace index to generate
     * @param X :: input X vector of the bin boundaries.
     * @param Y :: output vector to be filled with the Y data.
     * @param E :: output vector to be filled with the Error data (optionally)
     * @param skipError :: if true, the error vector is NOT calculated.
     *        This may save some processing time.
     */
    void EventWorkspace::generateHistogramPulseTime(const std::size_t index, const MantidVec& X,
        MantidVec& Y, MantidVec& E, bool skipError) const
    {
      if (index >= this->m_noVectors)
        throw std::range_error(
            "EventWorkspace::generateHistogramPulseTime, histogram number out of range");
      this->data[index]->generateHistogramPulseTime(X, Y, E, skipError);
    }

    //-----------------------------------------------------------------------------
    // --- Histogramming ----
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    /*** Set all histogram X vectors.
     * @param x :: The X vector of histogram bins to use.
     */
    void EventWorkspace::setAllX(Kernel::cow_ptr<MantidVec> &x)
    {
      //int counter=0;
      EventListVector::iterator i = this->data.begin();
      for (; i != this->data.end(); ++i)
      {
        (*i)->setX(x);
      }

      //Clear MRU lists now, free up memory
      this->clearMRU();
    }

    //-----------------------------------------------------------------------------
    /** Task for sorting an event list */
    class EventSortingTask: public Task
    {
    public:
      /// ctor
      EventSortingTask(const EventWorkspace * WS, size_t wiStart, size_t wiStop, EventSortType sortType,
          size_t howManyCores, Mantid::API::Progress * prog) :
          Task(), m_wiStart(wiStart), m_wiStop(wiStop), m_sortType(sortType), m_howManyCores(
              howManyCores), m_WS(WS), prog(prog)
      {
        m_cost = 0;
        if (m_wiStop > m_WS->getNumberHistograms())
          m_wiStop = m_WS->getNumberHistograms();

        for (size_t wi = m_wiStart; wi < m_wiStop; wi++)
        {
          double n = static_cast<double>(m_WS->getEventList(wi).getNumberEvents());
          // Sorting time is approximately n * ln (n)
          m_cost += n * log(n);
        }

        if (!((m_howManyCores == 1) || (m_howManyCores == 2) || (m_howManyCores == 4)))
          throw std::invalid_argument("howManyCores should be 1,2 or 4.");
      }

      // Execute the sort as specified.
      void run()
      {
        if (!m_WS)
          return;
        for (size_t wi = m_wiStart; wi < m_wiStop; wi++)
        {
          if (m_sortType != TOF_SORT)
            m_WS->getEventList(wi).sort(m_sortType);
          else
          {
            if (m_howManyCores == 1)
            {
              m_WS->getEventList(wi).sort(m_sortType);
            }
            else if (m_howManyCores == 2)
            {
              m_WS->getEventList(wi).sortTof2();
              Mantid::API::MemoryManager::Instance().releaseFreeMemory();
            }
            else if (m_howManyCores == 4)
            {
              m_WS->getEventList(wi).sortTof4();
              Mantid::API::MemoryManager::Instance().releaseFreeMemory();
            }
          }
          // Report progress
          if (prog)
            prog->report("Sorting");
        }
      }

    private:
      /// Start workspace index to process
      size_t m_wiStart;
      /// Stop workspace index to process
      size_t m_wiStop;
      /// How to sort
      EventSortType m_sortType;
      /// How many cores for each sort
      size_t m_howManyCores;
      /// EventWorkspace on which to sort
      const EventWorkspace * m_WS;
      /// Optional Progress dialog.
      Mantid::API::Progress * prog;
    };

    //-----------------------------------------------------------------------------

    /*
     * Review each event list to get the sort type
     * If any 2 have different order type, then be unsorted
     */
    EventSortType EventWorkspace::getSortType() const
    {
      size_t size = this->data.size();
      EventSortType order = data[0]->getSortType();
      for (size_t i = 1; i < size; i++)
      {
        if (order != data[i]->getSortType())
          return UNSORTED;
      }
      return order;
    }

    /*** Sort all event lists. Uses a parallelized algorithm
     * @param sortType :: How to sort the event lists.
     * @param prog :: a progress report object. If the pointer is not NULL, each event list will call prog.report() once.
     */
    void EventWorkspace::sortAll(EventSortType sortType, Mantid::API::Progress * prog) const
    {
      if (this->getSortType() == sortType)
      {
        if (prog != NULL)
        {
          prog->reportIncrement(this->data.size());
        }
        return;
      }

      size_t num_threads;
      num_threads = ThreadPool::getNumPhysicalCores();
      g_log.debug() << num_threads << " cores found. ";

      // Initial chunk size: set so that each core will be called for 20 tasks.
      // (This is to avoid making too small tasks.)
      size_t chunk_size = m_noVectors / (num_threads * 20);
      if (chunk_size < 1)
        chunk_size = 1;

      // Sort with 1 core per event list by default
      size_t howManyCores = 1;
      // And auto-detect how many threads
      size_t howManyThreads = 0;
      if (m_noVectors < num_threads * 10)
      {
        // If you have few vectors, sort with 2 cores.
        chunk_size = 1;
        howManyCores = 2;
        howManyThreads = num_threads / 2 + 1;
      }
      else if (m_noVectors < num_threads)
      {
        // If you have very few vectors, sort with 4 cores.
        chunk_size = 1;
        howManyCores = 4;
        howManyThreads = num_threads / 4 + 1;
      }
      g_log.debug() << "Performing sort with " << howManyCores << " cores per EventList, in "
          << howManyThreads << " threads, using a chunk size of " << chunk_size << ".\n";

      // Create the thread pool, and optimize by doing the longest sorts first.
      ThreadPool pool(new ThreadSchedulerLargestCost(), howManyThreads);
      for (size_t i = 0; i < m_noVectors; i += chunk_size)
      {
        pool.schedule(new EventSortingTask(this, i, i + chunk_size, sortType, howManyCores, prog));
      }

      // Now run it all
      pool.joinAll();
    }

    //---------------------------------------------------------------------------------------
    /** Integrate all the spectra in the matrix workspace within the range given.
     * Default implementation, can be overridden by base classes if they know something smarter!
     *
     * @param out :: returns the vector where there is one entry per spectrum in the workspace. Same
     *            order as the workspace indices.
     * @param minX :: minimum X bin to use in integrating.
     * @param maxX :: maximum X bin to use in integrating.
     * @param entireRange :: set to true to use the entire range. minX and maxX are then ignored!
     */
    void EventWorkspace::getIntegratedSpectra(std::vector<double> & out, const double minX,
        const double maxX, const bool entireRange) const
    {
      //Start with empty vector
      out.resize(this->getNumberHistograms(), 0.0);

      //We can run in parallel since there is no cross-reading of event lists
      PARALLEL_FOR_NO_WSP_CHECK()
      for (int wksp_index = 0; wksp_index < int(this->getNumberHistograms()); wksp_index++)
      {
        // Get Handle to data
        EventList * el = this->data[wksp_index];

        //Let the eventList do the integration
        out[wksp_index] = el->integrate(minX, maxX, entireRange);
      }
    }

  } // namespace DataObjects
} // namespace Mantid

///\cond TEMPLATE
template DLLExport class Mantid::API::WorkspaceProperty<Mantid::DataObjects::EventWorkspace>;

namespace Mantid
{
  namespace Kernel
  {
    template<> DLLExport
    Mantid::DataObjects::EventWorkspace_sptr IPropertyManager::getValue<
        Mantid::DataObjects::EventWorkspace_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::EventWorkspace_sptr>* prop = dynamic_cast<PropertyWithValue<
          Mantid::DataObjects::EventWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property " + name
            + " to incorrect type. Expected EventWorkspace.";
        throw std::runtime_error(message);
      }
    }

    template<> DLLExport
    Mantid::DataObjects::EventWorkspace_const_sptr IPropertyManager::getValue<
        Mantid::DataObjects::EventWorkspace_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::EventWorkspace_sptr>* prop = dynamic_cast<PropertyWithValue<
          Mantid::DataObjects::EventWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property " + name
            + " to incorrect type. Expected const EventWorkspace.";
        throw std::runtime_error(message);
      }
    }
  } // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
