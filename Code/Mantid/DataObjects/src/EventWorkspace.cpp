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

  EventWorkspace::EventWorkspace()
  {}
  EventWorkspace::~EventWorkspace()
  {}

  int EventWorkspace::size() const
  { // TODO implement
    throw NotImplementedError("EventWorkspace::size");
  }

  int EventWorkspace::blocksize() const
  { // TODO implement
    throw NotImplementedError("EventWorkspace::blocksize");
  }

  const int EventWorkspace::getNumberHistograms() const
  { // TODO implement
    throw NotImplementedError("EventWorkspace::getNumberHistogram");
  }

  MantidVec& EventWorkspace::dataX(const int index)
  { // TODO implement
    throw NotImplementedError("EventWorkspace::dataX");
  }

  MantidVec& EventWorkspace::dataY(const int index)
  { // TODO implement
    throw NotImplementedError("EventWorkspace::dataY");
  }

  MantidVec& EventWorkspace::dataE(const int index)
  { // TODO implement
    throw NotImplementedError("EventWorkspace::dataE");
  }

  MantidVec& EventWorkspace::dataX(const int index) const
  { // TODO implement
    throw NotImplementedError("EventWorkspace::dataX const");
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

  void EventWorkspace::init(const int &NVectors, const int &XLength,
			    const int &YLength)
  { // TODO implement
    throw NotImplementedError("EventWorkspace::init const");
  }

} // namespace DataObjects
} // namespace Mantid
