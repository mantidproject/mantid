#include "MantidAPI/RefAxis.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Exception.h"

DECLARE_WORKSPACE(EventWorkspace)

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
    //Initialize the  frame time.
    this->frameTime = std::vector<ptime>();
  }
  EventWorkspace::~EventWorkspace()
  {}


  void EventWorkspace::init(const int &NVectors, const int &XLength,
          const int &YLength)
  {
    // Check validity of arguments
    if (NVectors < 0)
    {
      g_log.error("Negative Number of Pixels specified to EventWorkspace::init");
      throw std::out_of_range("Negative Number of Pixels specified to EventWorkspace::init");
    }
    m_noVectors = NVectors;
    //Create all the event list objects from 0 to m_noVectors;
    for (int i=0; i < m_noVectors; i++)
    {
      data[i] = EventList();
    }

  }



  //-----------------------------------------------------------------------------
  int EventWorkspace::size() const
  {
    return this->data.size() * this->blocksize();
  }

  int EventWorkspace::blocksize() const
  {
    // Pick the first pixel to find the blocksize.
    EventListMap::iterator it = data.begin();
    if (it == data.end())
    {
      throw std::range_error("EventWorkspace::blocksize, no pixels in workspace, therefore cannot determine blocksize (# of bins).");
    }
    else
    {
      return it->second.getRefX()->size();
    }
  }

  const int EventWorkspace::getNumberHistograms() const
  {
    return this->data.size();
  }

  size_t EventWorkspace::getNumberEvents() const
  {
    size_t total = 0;
    for (EventListMap::const_iterator it = this->data.begin();
        it != this->data.end(); it++) {
      total += it->second.getNumberEvents();
    }
    return total;
  }

  const bool EventWorkspace::isHistogramData() const
  {
    return true;
  }


  //-----------------------------------------------------------------------------
  // --- Data Access ----
  //-----------------------------------------------------------------------------

  EventList& EventWorkspace::getEventList(const int index)
  {
    return this->data[index];
  }


  // Note: these non-const access methods will throw NotImplementedError
  MantidVec& EventWorkspace::dataX(const int index)
  {
    EventListMap::iterator iter = this->data.find(index);
    if (iter==this->data.end())
      throw std::range_error("EventWorkspace::dataX, histogram number out of range");
    return iter->second.dataX();
  }

  MantidVec& EventWorkspace::dataY(const int index)
  {
    EventListMap::iterator iter = this->data.find(index);
    if (iter==this->data.end())
      throw std::range_error("EventWorkspace::dataY, histogram number out of range");
    return iter->second.dataY();
  }

  MantidVec& EventWorkspace::dataE(const int index)
  {
    EventListMap::iterator iter = this->data.find(index);
    if (iter==this->data.end())
      throw std::range_error("EventWorkspace::dataE, histogram number out of range");
    return iter->second.dataE();
  }


  //-----------------------------------------------------------------------------
  // --- Const Data Access ----
  //-----------------------------------------------------------------------------
  //Can't use the [] operator for const access; you need to use find, which returns an iterator, that returns a struct with 2 members.

  const MantidVec& EventWorkspace::dataX(const int index) const
  {
    EventListMap::iterator iter = this->data.find(index);
    if (iter==this->data.end())
      throw std::range_error("EventWorkspace::dataX, histogram number out of range");
    return iter->second.dataX();
  }

  const MantidVec& EventWorkspace::dataY(const int index) const
  {
    EventListMap::iterator iter = this->data.find(index);
    if (iter==this->data.end())
      throw std::range_error("EventWorkspace::dataY, histogram number out of range");
    return iter->second.dataY();
  }

  const MantidVec& EventWorkspace::dataE(const int index) const
  {
    EventListMap::iterator iter = this->data.find(index);
    if (iter==this->data.end())
      throw std::range_error("EventWorkspace::dataE, histogram number out of range");
    return iter->second.dataE();
  }

  Kernel::cow_ptr<MantidVec> EventWorkspace::refX(const int index) const
  {
    EventListMap::iterator iter = this->data.find(index);
    if (iter==this->data.end())
      throw std::range_error("EventWorkspace::refX, histogram number out of range");
    return iter->second.getRefX();

  }

  //-----------------------------------------------------------------------------
  // --- Histogramming ----
  //-----------------------------------------------------------------------------
  void EventWorkspace::setX(const int index,
      const Kernel::cow_ptr<MantidVec> &x)
  {
    this->data[index].setX(x);
  }

  void EventWorkspace::setAllX(Kernel::cow_ptr<MantidVec> &x)
  {
    EventListMap::iterator i = this->data.begin();
    for( ; i != this->data.end(); ++i )
    {
      // i->first is your key
      //Set the x now.
      i->second.setX(x);
    }
  }


  //-----------------------------------------------------------------------------
  // --- Frame Times ----
  //-----------------------------------------------------------------------------

  /** Get the absolute time corresponding to the give frame ID */
  ptime EventWorkspace::getTime(const size_t frameId)
  {
    if ((frameId < 0) || (frameId >= this->frameTime.size()))
      throw std::range_error("EventWorkspace::getTime called with a frameId outside the range.");

    //Will throw an exception if you are out of bounds.
    return this->frameTime.at(frameId);
  }

  /** Add ahe absolute time corresponding to the give frame ID */
  void EventWorkspace::addTime(const size_t frameId, ptime absoluteTime)
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
