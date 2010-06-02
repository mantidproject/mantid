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


  Kernel::cow_ptr<MantidVec> EventWorkspace::refX(const int) const
  { // TODO implement
    throw NotImplementedError("EventWorkspace::refX const");
  }

  //-----------------------------------------------------------------------------
  // --- Histogramming ----
  //-----------------------------------------------------------------------------
  void EventWorkspace::setX(const int index,
			    const Kernel::cow_ptr<MantidVec> &x)
  {
    this->data[index].setX(x);
  }

} // namespace DataObjects
} // namespace Mantid
