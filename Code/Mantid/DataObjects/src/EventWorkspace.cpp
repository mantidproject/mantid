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
  {}
  EventWorkspace::~EventWorkspace()
  {}


  void EventWorkspace::init(const int &NVectors, const int &XLength,
          const int &YLength)
  {
    // Check validity of arguments
    if (NVectors <= 0)
    {
      g_log.error("All arguments to init must be positive and non-zero");
      throw std::out_of_range("All arguments to init must be positive and non-zero");
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
    return m_noVectors;
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
    if (index<0 || index>=m_noVectors)
      throw std::range_error("EventWorkspace::dataX, histogram number out of range");
    return this->data[index].dataX();
  }

  MantidVec& EventWorkspace::dataY(const int index)
  {
    if (index<0 || index>=m_noVectors)
      throw std::range_error("EventWorkspace::dataY, histogram number out of range");
    return this->data[index].dataY();
  }

  MantidVec& EventWorkspace::dataE(const int index)
  {
    if (index<0 || index>=m_noVectors)
      throw std::range_error("EventWorkspace::dataE, histogram number out of range");
    return this->data[index].dataE();
  }


  //-----------------------------------------------------------------------------
  // --- Const Data Access ----
  //-----------------------------------------------------------------------------
  const MantidVec& EventWorkspace::dataX(const int index) const
  {
    if (index<0 || index>=m_noVectors)
      throw std::range_error("EventWorkspace::dataX, histogram number out of range");
    //Can't use the [] operator for const access; you need to use find, which returns an iterator, that returns a struct with 2 members.
    return this->data.find(index)->second.dataX();
  }

  const MantidVec& EventWorkspace::dataY(const int index) const
  {
    if (index<0 || index>=m_noVectors)
      throw std::range_error("EventWorkspace::dataY, histogram number out of range");
    //Can't use the [] operator for const access; you need to use find, which returns an iterator, that returns a struct with 2 members.
    return this->data.find(index)->second.dataY();
  }

  const MantidVec& EventWorkspace::dataE(const int index) const
  {
    if (index<0 || index>=m_noVectors)
      throw std::range_error("EventWorkspace::dataE, histogram number out of range");
    //Can't use the [] operator for const access; you need to use find, which returns an iterator, that returns a struct with 2 members.
    return this->data.find(index)->second.dataE();
  }


  Kernel::cow_ptr<MantidVec> EventWorkspace::refX(const int index) const
  {
    if (index<0 || index>=m_noVectors)
      throw std::range_error("EventWorkspace::refX, histogram number out of range");
    return this->data[index].getRefX();
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

} // namespace DataObjects
} // namespace Mantid
