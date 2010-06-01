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

  //-----------------------------------------------------------------------------
  EventWorkspace::EventWorkspace()
  {}
  EventWorkspace::~EventWorkspace()
  {}


  void EventWorkspace::init(const int &NVectors, const int &XLength,
          const int &YLength)
  {
    // Check validity of arguments
    if (NVectors <= 0 || XLength <= 0 || YLength <= 0)
    {
      g_log.error("All arguments to init must be positive and non-zero");
      throw std::out_of_range("All arguments to init must be positive and non-zero");
    }
    //throw NotImplementedError("EventWorkspace::init const");
    m_noVectors = NVectors;
  }



  //-----------------------------------------------------------------------------
  int EventWorkspace::size() const
  { // TODO implement
    throw NotImplementedError("EventWorkspace::size");
  }

  int EventWorkspace::blocksize() const
  { // TODO implement
    throw NotImplementedError("EventWorkspace::blocksize");
  }

  const int EventWorkspace::getNumberHistograms() const
  {
    return m_noVectors;
  }


  //-----------------------------------------------------------------------------
  // --- Data Access ----
  //-----------------------------------------------------------------------------
  MantidVec& EventWorkspace::dataX(const int index)
  {
    return this->data[index].dataX().access();
  }

  MantidVec& EventWorkspace::dataY(const int index)
  {
    return this->data[index].dataY().access();
  }

  MantidVec& EventWorkspace::dataE(const int index)
  {
    if (index<0 || index>=m_noVectors)
      throw std::range_error("EventWorkspace::dataE, histogram number out of range");
    return this->data[index].dataE().access();
  }


  MantidVec& EventWorkspace::dataX(const int index) const
  { // TODO implement
    //return this->data[index].dataX().access();
    throw NotImplementedError("EventWorkspace::dataY const");
  }

  MantidVec& EventWorkspace::dataY(const int index) const
  { // TODO implement
    throw NotImplementedError("EventWorkspace::dataY const");
  }

  MantidVec& EventWorkspace::dataE(const int index) const
  { // TODO implement
    throw NotImplementedError("EventWorkspace::dataE const");
  }


  Kernel::cow_ptr<MantidVec> EventWorkspace::refX(const int) const
  { // TODO implement
    throw NotImplementedError("EventWorkspace::refX const");
  }

  void EventWorkspace::setX(const int index,
			    const Kernel::cow_ptr<MantidVec> &x)
  { // TODO implement
    throw NotImplementedError("EventWorkspace::setX const");
  }

} // namespace DataObjects
} // namespace Mantid
