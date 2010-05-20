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

  EventWorkspace::EventWorkspace()
  {}
  EventWorkspace::~EventWorkspace()
  {}

  int EventWorkspace::size() const
  { // TODO impement
    throw NotImplementedError("EventWorkspace::size");
  }

  int EventWorkspace::blocksize() const
  { // TODO impement
    throw NotImplementedError("EventWorkspace::blocksize");
  }

  const int EventWorkspace::getNumberHistograms() const
  { // TODO impement
    throw NotImplementedError("EventWorkspace::getNumberHistogram");
  }

  MantidVec& EventWorkspace::dataX(const int index)
  { // TODO impement
    throw NotImplementedError("EventWorkspace::dataX");
  }

  MantidVec& EventWorkspace::dataY(const int index)
  { // TODO impement
    throw NotImplementedError("EventWorkspace::dataY");
  }

  MantidVec& EventWorkspace::dataE(const int index)
  { // TODO impement
    throw NotImplementedError("EventWorkspace::dataE");
  }

  MantidVec& EventWorkspace::dataX(const int index) const
  { // TODO impement
    throw NotImplementedError("EventWorkspace::dataX const");
  }

  MantidVec& EventWorkspace::dataY(const int index) const
  { // TODO impement
    throw NotImplementedError("EventWorkspace::dataY const");
  }

  MantidVec& EventWorkspace::dataE(const int index) const
  { // TODO impement
    throw NotImplementedError("EventWorkspace::dataE const");
  }

  Kernel::cow_ptr<MantidVec> EventWorkspace::refX(const int) const
  { // TODO impement
    throw NotImplementedError("EventWorkspace::refX const");
  }

  void EventWorkspace::setX(const int index,
			    const Kernel::cow_ptr<MantidVec> &x)
  { // TODO impement
    throw NotImplementedError("EventWorkspace::setX const");
  }

  void EventWorkspace::init(const int &NVectors, const int &XLength,
			    const int &YLength)
  { // TODO impement
    throw NotImplementedError("EventWorkspace::init const");
  }

} // namespace DataObjects
} // namespace Mantid
