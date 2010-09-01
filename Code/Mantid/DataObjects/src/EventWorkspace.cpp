#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/WorkspaceIteratorCode.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Exception.h"

DECLARE_WORKSPACE(EventWorkspace)

using namespace boost::posix_time;

namespace Mantid
{

namespace DataObjects
{
using Kernel::Exception::NotImplementedError;

  // get a reference to the logger
  Kernel::Logger& EventWorkspace::g_log
                 = Kernel::Logger::get("EventWorkspace");

  //---- Constructors -------------------------------------------------------------------
  EventWorkspace::EventWorkspace() : m_bufferedDataY(100), m_bufferedDataE(100),
      m_cachedNumberOfEvents(0)
  {
  }

  EventWorkspace::~EventWorkspace()
  {
    //Make sure you free up the memory in the MRU
    m_bufferedDataY.clear();
    m_bufferedDataE.clear();
    //Go through the event list and clear them?
    EventListVector::iterator i = this->data.begin();
    for( ; i != this->data.end(); ++i )
    {
      //Deleting the event list should call its destructor to release the vector memory.
      delete (*i);
    }
  }

  //-----------------------------------------------------------------------------
  /** Returns true if the EventWorkspace is safe for multithreaded operations.
   */
  bool EventWorkspace::threadSafe() const
  {
    //Return false if ANY event list is not sorted. You can't have 2 threads trying to sort the
    //  same event list simultaneously.
    for (int i=0; i<static_cast<int>(data.size()); i++)
      if (!data[i]->isSortedByTof())
        return false;
    return true;
  }


  //-----------------------------------------------------------------------------
  /** Initialize the pixels
    *  @param NVectors The number of vectors/histograms/detectors in the workspace. Does not need
    *         to be set, but needs to be > 0
    *  @param XLength The number of X data points/bin boundaries in each vector (ignored)
    *  @param YLength The number of data/error points in each vector (ignored)
   */
  void EventWorkspace::init(const int &NVectors, const int &XLength,
          const int &YLength)
  {
    // Check validity of arguments
    if (NVectors <= 0)
    {
      g_log.error("Negative or 0 Number of Pixels specified to EventWorkspace::init");
      throw std::out_of_range("Negative or 0 Number of Pixels specified to EventWorkspace::init");
    }
    //Initialize the data
    m_noVectors = NVectors;
    data.resize(m_noVectors, NULL);
    this->done_loading_data = false;

    //Create axes.
    m_axes.resize(2);
    //I'm not sure what the heck this first axis is supposed to be; copying from Workspace2D
    m_axes[0] = new API::RefAxis(XLength, this);
    // Ok, looks like the second axis is supposed to be the spectrum # for each entry in the workspace index
    //  I have no idea why it is such a convoluted way of doing it.
    m_axes[1] = new API::SpectraAxis(m_noVectors);

  }

  //-----------------------------------------------------------------------------
  /**
   * Copy all of the data (event lists) from the source workspace to this workspace.
   */
  void EventWorkspace::copyDataFrom(const EventWorkspace& source)
  {
    //Start with nothing.
    this->data.clear();
    //Copy the vector of EventLists
    EventListVector source_data = source.data;
    EventListVector::iterator it;
    for (it = source_data.begin(); it != source_data.end(); it++ )
    {
      //Create a new event list, copying over the events
      EventList * newel = new EventList( **it );
      this->data.push_back(newel);
    }
    this->clearMRU();
    //The data map is useless now
    this->data_map.clear();
    //This marker needs to be set.
    this->done_loading_data = true;
  }


  //-----------------------------------------------------------------------------
  /// The total size of the workspace
  /// @returns the number of single indexable items in the workspace
  int EventWorkspace::size() const
  {
    return this->data.size() * this->blocksize();
  }

  //-----------------------------------------------------------------------------
  /// Get the blocksize, aka the number of bins in the histogram
  /// @returns the number of bins in the Y data
  int EventWorkspace::blocksize() const
  {
    // Pick the first pixel to find the blocksize.
    EventListVector::const_iterator it = data.begin();
    if (it == data.end())
    {
      throw std::range_error("EventWorkspace::blocksize, no pixels in workspace, therefore cannot determine blocksize (# of bins).");
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
  int EventWorkspace::getNumberHistograms() const
  {
    return this->data.size();
  }

  //-----------------------------------------------------------------------------
  /// The total number of events across all of the spectra.
  /// @returns The total number of events
  size_t EventWorkspace::getNumberEvents() const
  {
    size_t total = 0;
    for (EventListVector::const_iterator it = this->data.begin();
        it != this->data.end(); it++) {
      total += (*it)->getNumberEvents();
    }
    return total;
  }

  //-----------------------------------------------------------------------------
  /// Returns true always - an EventWorkspace always represents histogramm-able data
  /// @returns If the data is a histogtram - always true for an eventWorkspace
  bool EventWorkspace::isHistogramData() const
  {
    return true;
  }

  //-----------------------------------------------------------------------------
  /// Return how many entries in the Y MRU list are used.
  int EventWorkspace::MRUSize() const
  {
    return this->m_bufferedDataY.size();
  }

  /** Clear the MRU list */
  void EventWorkspace::clearMRU() const
  {
    this->m_bufferedDataY.clear();
    this->m_bufferedDataE.clear();
  }

  //-----------------------------------------------------------------------------
  /// Returns the amount of memory used in KB
  long int EventWorkspace::getMemorySize() const
  {
//    std::stringstream out;
//    out << "Get memory size. m_cachedNumberOfEvents = " << m_cachedNumberOfEvents <<
//        ". sizeof(TofEvent) = " << sizeof(TofEvent) <<
//        ". sizeof(std::size_t) = " << sizeof(std::size_t) <<
//        ". sizeof(EventList) = " << sizeof(EventList);
//    g_log.information(out.str());
//    std::cout << out.str() << "\n";

    //Add up the two buffers and the events.
    return ((this->m_bufferedDataY.size() + this->m_bufferedDataE.size()) * this->blocksize() * sizeof(double)
        + this->m_cachedNumberOfEvents * sizeof(TofEvent)
        + this->getNumberHistograms() * sizeof(EventList) ) / 1024;
  }

  //-----------------------------------------------------------------------------
  // --- Data Access ----
  //-----------------------------------------------------------------------------

  /** Get an EventList object at the given pixelid/spectrum number
   * @param spectrumNumber Spectrum number to get. This is not necessarily the same as the workspace Index.
   * @returns A reference to the eventlist
   */
  EventList& EventWorkspace::getEventList(const int spectrumNumber)
  {
    if (this->done_loading_data)
      throw std::runtime_error("EventWorkspace::getEventList called after doneLoadingData(). Try getEventListAtWorkspaceIndex() instead.");
    //An empty entry will be made if needed
    EventListMap::iterator it = this->data_map.find(spectrumNumber);
    if (it == this->data_map.end())
    {
      //Need to make a new one!
      EventList * newel = new EventList();
      //Save it in the map
      this->data_map[spectrumNumber] = newel;
      return (*newel);
    }
    else
    {
      //Already exists; return it
      return *this->data_map[spectrumNumber];
    }
  }

  /** Get an EventList object at the given workspace index number
   * @param workspace_index The histogram workspace index number.
   * @returns A reference to the eventlist
   */
  EventList& EventWorkspace::getEventListAtWorkspaceIndex(const int workspace_index)
  {
    return *this->data[workspace_index];
  }

  /** Get a const EventList object at the given workspace index number
   * @param workspace_index The histogram workspace index number.
   * @returns A const reference to the eventlist
   */
  const EventList& EventWorkspace::getEventListAtWorkspaceIndex(const int workspace_index) const
  {
    return *this->data[workspace_index];
  }


  //-----------------------------------------------------------------------------
  /** Adds a new EventList at the end of the current workspace index list.
   * Copies the EventList from a const reference to another one.
   * The # of histograms in the workspace increases by one.
   *
   * @param existingEventList a reference to an existing event list;
   *            all events and the X histogram is copied.
   * @return a pointer to the new EventList that was created.
   */
  EventList * EventWorkspace::addNewEventList(const EventList& existingEventList)
  {
    EventList * newEL = new EventList(existingEventList);
    this->data.push_back(newEl);

    //Workspace index of the new data
    int wi = this->data.size()-1;


    //Spectrum number of this workspace index
    this->m_axes[1]->spectraNo()

    m_spectramap->addSpectrumEntries( )

    this->m_noVectors = this->data.size();
    return newEL;
  }


  //-----------------------------------------------------------------------------
  /** Call this method when loading event data is complete.
   * The map of pixelid to spectrum # is generated.
   * Also, a simple 1:1 map of spectrum # to pixel id (detector #) is generated
   * @param makeSpectraMap Generate a spectramap (0=No, otherwise yes). Default = YES
   * */
  void EventWorkspace::doneLoadingData(int makeSpectraMap)
  {
    //Ok, we need to take the data_map, and turn it into a data[] vector.

    //Let's make the vector big enough.
    if (static_cast<int>(this->data_map.size()) > m_noVectors)
    {
      //Too many vectors! Why did you initialize it bigger than you needed to, silly?
      for (int i=this->data_map.size(); i<m_noVectors; i++)
        //Delete the offending EventList so as to avoid memory leaks.
        delete this->data[i];
    }
    //Now resize
    m_noVectors = this->data_map.size();
    this->data.resize(m_noVectors, NULL);

    //For the mapping workspace
    int* index_table = new int [m_noVectors];
    int* pixelid_table = new int [m_noVectors];
    int max_pixel_id = 0;

    int counter = 0;
    EventListMap::iterator it;
    for (it = this->data_map.begin(); it != this->data_map.end(); it++)
    {
      //Iterate through the map
      index_table[counter] = counter;
      pixelid_table[counter] = it->first; //The key = the pixelid
      //Find the maximum pixel id #
      if (it->first > max_pixel_id)
        max_pixel_id = it->first;

      //Copy the pointer to the event list in there.
      this->data[counter] = it->second;

      counter++;
    }

    //We create a spectra-type axis that holds the spectrum # at each workspace index, in a convoluted and annoying way.
    delete m_axes[1];
    m_axes[1] = new API::SpectraAxis(m_noVectors);
    //Fill it with the pixel id / spectrum # at workspace index i.
    for (int i=0; i < m_noVectors; i++)
      m_axes[1]->setValue(i, pixelid_table[i]);

    if (makeSpectraMap)
    {
      g_log.information() << "About to populate spectra map with a 1:1 map up to " << max_pixel_id << "\n";
      if (max_pixel_id > 10e6)
      {
        g_log.warning() << "Warning! The maximum pixel ID counted, " << max_pixel_id << ", is so large that it might be in error. The spectra map will only be made up to 10e6 pixels.\n";
        max_pixel_id = 10e6;
      }
      //Make the mapping between spectrum # and pixel id (aka detector id)
      //  In this case, it is a simple 1-1 map.
      mutableSpectraMap().populateSimple(0, max_pixel_id+1); //Go to max_pixel_id+1 to make sure you catch that one too
    }

    //Now clear the data_map
    this->data_map.clear();

    //Get your memory back :)
    delete [] index_table;
    delete [] pixelid_table;

    //Cache the # of events (for getMemorySize)
    this->m_cachedNumberOfEvents = this->getNumberEvents();

    //Set the flag for raising errors later
    this->done_loading_data = true;
  }


  //-----------------------------------------------------------------------------
  /// Return the data X vector at a given workspace index
  /// Note: these non-const access methods will throw NotImplementedError
  /// @param index the workspace index to return
  /// @returns A reference to the vector of binned error values
  MantidVec& EventWorkspace::dataX(const int index)
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataX, histogram number out of range");
    throw NotImplementedError("EventWorkspace::dataX cannot return a non-const array: you can't modify the histogrammed data in an EventWorkspace!");
  }

  /// Return the data Y vector at a given workspace index
  /// Note: these non-const access methods will throw NotImplementedError
  /// @param index the workspace index to return
  /// @returns A reference to the vector of binned error values
  MantidVec& EventWorkspace::dataY(const int index)
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataY, histogram number out of range");
    throw NotImplementedError("EventWorkspace::dataY cannot return a non-const array: you can't modify the histogrammed data in an EventWorkspace!");
  }

  /// Return the data E vector at a given workspace index
  /// Note: these non-const access methods will throw NotImplementedError
  /// @param index the workspace index to return
  /// @returns A reference to the vector of binned error values
  MantidVec& EventWorkspace::dataE(const int index)
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataE, histogram number out of range");
    throw NotImplementedError("EventWorkspace::dataE cannot return a non-const array: you can't modify the histogrammed data in an EventWorkspace!");
  }




  //-----------------------------------------------------------------------------
  // --- Const Data Access ----
  //-----------------------------------------------------------------------------

  /// Return the const data X vector at a given workspace index
  const MantidVec& EventWorkspace::dataX(const int index) const
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataX, histogram number out of range");
    return this->data[index]->dataX();
  }


  //---------------------------------------------------------------------------
  /// Return the const data Y vector at a given workspace index
  const MantidVec& EventWorkspace::dataY(const int index) const
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataY, histogram number out of range");

    //std::stringstream out;  out << "I'm retrieving DataY for index " << index << ".";
    //g_log.information(out.str());

    //Is the data in the mrulist?
    MantidVecWithMarker * data = m_bufferedDataY.find(index);
    if (data == NULL)
    {
      //Create the MRU object
      data = new MantidVecWithMarker(index);
      //Set the Y data in it
      this->data[index]->generateCountsHistogram( *this->data[index]->getRefX(), data->m_data);

      //Lets save it in the MRU
      MantidVecWithMarker * oldData = m_bufferedDataY.insert(data);

      //And clear up the memory of the old one, if it is dropping out.
      if (oldData)
        delete oldData;
    }
    return data->m_data;
  }

  //---------------------------------------------------------------------------
  /// Return the const data E vector at a given workspace index
  const MantidVec& EventWorkspace::dataE(const int index) const
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataE, histogram number out of range");

    //Is the data in the mrulist?
    MantidVecWithMarker * data = m_bufferedDataE.find(index);
    if (data == NULL)
    {
      //Get a handle on the event list.
      const EventList * el = this->data[index];

      //Create the MRU object
      data = new MantidVecWithMarker(index);

      //Now use that to get E -- Y values are generated from another function
      MantidVec Y;
      el->generateCountsHistogram( *(el->getRefX()), Y);
      el->generateErrorsHistogram( Y, data->m_data);

      //Lets save it in the MRU
      MantidVecWithMarker * oldData = m_bufferedDataE.insert(data);

      //And clear up the memory of the old one, if it is dropping out.
      if (oldData)
        delete oldData;
    }
    return data->m_data;
  }

  //---------------------------------------------------------------------------
  /// Get a pointer to the x data at the given workspace index
  Kernel::cow_ptr<MantidVec> EventWorkspace::refX(const int index) const
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::refX, histogram number out of range");
    return this->data[index]->getRefX();
  }

  //-----------------------------------------------------------------------------
  // --- Histogramming ----
  //-----------------------------------------------------------------------------
  /*** Set a histogram X vector. Should only be called after doneLoadingData().
   * @param index Workspace histogram index to set.
   * @param x The X vector of histogram bins to use.
   */
  void EventWorkspace::setX(const int index, const Kernel::cow_ptr<MantidVec> &x)
  {
    if (!this->done_loading_data)
      throw std::runtime_error("EventWorkspace::setX called before doneLoadingData().");
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::setX, histogram number out of range");
    this->data[index]->setX(x);
  }


  /*** Set a histogram X vector but create a COW pointer for it. Should only be called after doneLoadingData().
   * @param index Workspace histogram index to set.
   * @param x The X vector of histogram bins to use.
   */
  void EventWorkspace::setX(const int index, const MantidVec &X)
  {
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.assign(X.begin(), X.end());
    this->setX(index, axis);
  }


  //-----------------------------------------------------------------------------
  /*** Set all histogram X vectors. Should only be called after doneLoadingData().
   * @param x The X vector of histogram bins to use.
   */
  void EventWorkspace::setAllX(Kernel::cow_ptr<MantidVec> &x)
  {
    if (!this->done_loading_data)
      throw std::runtime_error("EventWorkspace::setAllX called before doneLoadingData().");
    //int counter=0;
    EventListVector::iterator i = this->data.begin();
    for( ; i != this->data.end(); ++i )
    {
      (*i)->setX(x);
    }

    //Clear MRU lists now, free up memory
    this->clearMRU();
  }


  //-----------------------------------------------------------------------------
  // --- Frame Times ----
  //-----------------------------------------------------------------------------
  /** Get the absolute time corresponding to the give frame ID
   * @param frameId The id of the frame
   * */
  ptime EventWorkspace::getTime(const size_t frameId)
  {
    if ((frameId < 0) || (frameId >= this->frameTime.size()))
      throw std::range_error("EventWorkspace::getTime called with a frameId outside the range.");

    //Will throw an exception if you are out of bounds.
    return this->frameTime.at(frameId);
  }

  /** Add the absolute time corresponding to the give frame ID
   * @param frameId The id of the frame to add
   * @param absoluteTime The time to which to set the frame ID
   * */
  void EventWorkspace::addTime(const size_t frameId, boost::posix_time::ptime absoluteTime)
  {
    if (frameId < 0)
      throw std::range_error("EventWorkspace::addTime called with a frameId below 0.");
    //Resize, if needed, and fill with the default ptime (which is not-a-time)
    if (this->frameTime.size() <= frameId)
      this->frameTime.resize(frameId+1, ptime());
    this->frameTime[frameId] = absoluteTime;
  }


} // namespace DataObjects
} // namespace Mantid


///\cond TEMPLATE
template DLLExport class Mantid::API::workspace_iterator<Mantid::API::LocatedDataRef, Mantid::DataObjects::EventWorkspace>;
template DLLExport class Mantid::API::workspace_iterator<const Mantid::API::LocatedDataRef, const Mantid::DataObjects::EventWorkspace>;

template DLLExport class Mantid::API::WorkspaceProperty<Mantid::DataObjects::EventWorkspace>;

namespace Mantid
{
  namespace Kernel
  {
    template<> DLLExport
    Mantid::DataObjects::EventWorkspace_sptr IPropertyManager::getValue<Mantid::DataObjects::EventWorkspace_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::EventWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::EventWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type (EventWorkspace_sptr)";
        throw std::runtime_error(message);
      }
    }

    template<> DLLExport
    Mantid::DataObjects::EventWorkspace_const_sptr IPropertyManager::getValue<Mantid::DataObjects::EventWorkspace_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::EventWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::EventWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type (EventWorkspace_sptr)";
        throw std::runtime_error(message);
      }
    }
  } // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
