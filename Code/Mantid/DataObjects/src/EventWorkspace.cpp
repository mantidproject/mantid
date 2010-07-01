#include "MantidAPI/RefAxis.h"
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
  EventWorkspace::EventWorkspace()
  {
  }
  EventWorkspace::~EventWorkspace()
  {}

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
    m_axes[1] = new API::Axis(API::AxisType::Spectra,m_noVectors);

  }


  //-----------------------------------------------------------------------------
  /// Returns the number of single indexable items in the workspace
  int EventWorkspace::size() const
  {
    return this->data.size() * this->blocksize();
  }

  //-----------------------------------------------------------------------------
  /// Get the blocksize, aka the number of bins in the histogram
  int EventWorkspace::blocksize() const
  {
    // Pick the first pixel to find the blocksize.
    EventListVector::iterator it = data.begin();
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
  /** Get the number of histograms, usually the same as the number of pixels or detectors. */
  const int EventWorkspace::getNumberHistograms() const
  {
    return this->data.size();
  }

  //-----------------------------------------------------------------------------
  /// The total number of events across all of the spectra.
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
  const bool EventWorkspace::isHistogramData() const
  {
    return true;
  }


  //-----------------------------------------------------------------------------
  // --- Data Access ----
  //-----------------------------------------------------------------------------

  /** Get an EventList object at the given pixelid/spectrum number
   * @param pixelid Pixel ID (aka spectrum number)
   */
  EventList& EventWorkspace::getEventList(const int pixelid)
  {
    if (this->done_loading_data)
      throw std::runtime_error("EventWorkspace::getEventList called after doneLoadingData(). Try getEventListAtWorkspaceIndex() instead.");
    //An empty entry will be made if needed
    EventListMap::iterator it = this->data_map.find(pixelid);
    if (it == this->data_map.end())
    {
      //Need to make a new one!
      EventList * newel = new EventList();
      //Save it in the map
      this->data_map[pixelid] = newel;
      return (*newel);
    }
    else
    {
      //Already exists; return it
      return *this->data_map[pixelid];
    }
  }

  /** Get an EventList object at the given workspace index number
   * @param workspace_index The histogram workspace index number.
   */
  EventList& EventWorkspace::getEventListAtWorkspaceIndex(const int workspace_index)
  {
    return *this->data[workspace_index];
  }


  //-----------------------------------------------------------------------------
  /** Call this method when loading event data is complete.
   * The map of pixelid to spectrum # is generated.
   * */
  void EventWorkspace::doneLoadingData()
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

      //std::cout << "added" << std::endl;
      counter++;
    }

    //We create a spectra-type axis that holds the spectrum # at each workspace index, in a convoluted and annoying way.
    delete m_axes[1];
    m_axes[1] = new API::Axis(API::AxisType::Spectra, m_noVectors);
    //Fill it with the pixel id / spectrum # at workspace index i.
    for (int i=0; i < m_noVectors; i++)
      m_axes[1]->setValue(i, pixelid_table[i]);

    //Make the mapping between spectrum # and pixel id (aka detector id)
    //  In this case, it is a simple 1-1 map.
    mutableSpectraMap().populateSimple(0, max_pixel_id);


    //Now clear the data_map
    this->data_map.clear();

    //Get your memory back :)
    delete [] index_table;
    delete [] pixelid_table;

    //Set the flag for raising errors later
    this->done_loading_data = true;
  }


  //-----------------------------------------------------------------------------
  /// Return the data X vector at a given workspace index
  /// Note: these non-const access methods will throw NotImplementedError
  MantidVec& EventWorkspace::dataX(const int index)
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataX, histogram number out of range");
    throw NotImplementedError("EventWorkspace::dataX cannot return a non-const array: you can't modify the histogrammed data in an EventWorkspace!");
  }

  /// Return the data Y vector at a given workspace index
  /// Note: these non-const access methods will throw NotImplementedError
  MantidVec& EventWorkspace::dataY(const int index)
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataY, histogram number out of range");
    throw NotImplementedError("EventWorkspace::dataY cannot return a non-const array: you can't modify the histogrammed data in an EventWorkspace!");
  }

  /// Return the data E vector at a given workspace index
  /// Note: these non-const access methods will throw NotImplementedError
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

  /// Return the const data Y vector at a given workspace index
  const MantidVec& EventWorkspace::dataY(const int index) const
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataY, histogram number out of range");
    return this->data[index]->dataY();
  }

  /// Return the const data E vector at a given workspace index
  const MantidVec& EventWorkspace::dataE(const int index) const
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataE, histogram number out of range");
    return this->data[index]->dataE();
  }

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
  /*** Set a histogram X vector.
   * @param index Workspace histogram index to set.
   * @param x The X vector of histogram bins to use.
   */
  void EventWorkspace::setX(const int index,
      const Kernel::cow_ptr<MantidVec> &x)
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::setX, histogram number out of range");
    this->data[index]->setX(x);
  }

  //-----------------------------------------------------------------------------
  /*** Set all histogram X vectors.
   * @param x The X vector of histogram bins to use.
   */
  void EventWorkspace::setAllX(Kernel::cow_ptr<MantidVec> &x)
  {
    EventListVector::iterator i = this->data.begin();
    for( ; i != this->data.end(); ++i )
    {
      (*i)->setX(x);
    }
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
