#include "MantidAPI/IEventList.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"
#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <vector>

using Mantid::API::EventType;
using Mantid::API::IEventList;
using Mantid::API::TOF;
using Mantid::API::WEIGHTED;
using Mantid::API::WEIGHTED_NOTIME;

namespace Policies = Mantid::PythonInterface::Policies;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IEventList)

/// return_value_policy for copied numpy array
using return_clone_numpy = return_value_policy<Policies::VectorToNumpy>;

void export_IEventList() {
  register_ptr_to_python<IEventList *>();

  enum_<EventType>("EventType")
      .value("TOF", TOF)
      .value("WEIGHTED", WEIGHTED)
      .value("WEIGHTED_NOTIME", WEIGHTED_NOTIME)
      .export_values();

  class_<IEventList, bases<Mantid::API::ISpectrum>, boost::noncopyable>(
      "IEventList", no_init)
      .def("getEventType", &IEventList::getEventType, args("self"),
           "Return the type of events stored.")
      .def("switchTo", &IEventList::switchTo, args("self", "newType"),
           "Switch the event type to the one specified")
      .def("clear", &IEventList::clear, args("self", "removeDetIDs"),
           "Clears the event list")
      .def("isSortedByTof", &IEventList::isSortedByTof, args("self"),
           "Returns true if the list is sorted in TOF")
      .def("getNumberEvents", &IEventList::getNumberEvents, args("self"),
           "Returns the number of events within the list")
      .def("getMemorySize", &IEventList::getMemorySize, args("self"),
           "Returns the memory size in bytes")
      .def("integrate", &IEventList::integrate,
           args("self", "minX", "maxX", "entireRange"),
           "Integrate the events between a range of X values, or all events.")
      .def("convertTof",
           (void (IEventList::*)(const double, const double)) &
               IEventList::convertTof,
           args("self", "factor", "offset"),
           "Convert the time of flight by tof'=tof*factor+offset")
      .def("scaleTof", &IEventList::scaleTof, args("self", "factor"),
           "Convert the tof units by scaling by a multiplier.")
      .def("addTof", &IEventList::addTof, args("self", "offset"),
           "Add an offset to the TOF of each event in the list.")
      .def("addPulsetime", &IEventList::addPulsetime, args("self", "seconds"),
           "Add an offset to the pulsetime (wall-clock time) of each event in "
           "the list.")
      .def("maskTof", &IEventList::maskTof, args("self", "tofMin", "tofMax"),
           "Mask out events that have a tof between tofMin and tofMax "
           "(inclusively)")
      .def("getTofs",
           (std::vector<double>(IEventList::*)(void) const) &
               IEventList::getTofs,
           args("self"), return_clone_numpy(),
           "Get a vector of the TOFs of the events")
      .def("getWeights",
           (std::vector<double>(IEventList::*)(void) const) &
               IEventList::getWeights,
           args("self"), return_clone_numpy(),
           "Get a vector of the weights of the events")
      .def("getWeightErrors",
           (std::vector<double>(IEventList::*)(void) const) &
               IEventList::getWeightErrors,
           args("self"), return_clone_numpy(),
           "Get a vector of the weights of the events")
      .def("getPulseTimes", &IEventList::getPulseTimes, args("self"),
           "Get a vector of the pulse times of the events")
      .def("getTofMin", &IEventList::getTofMin, args("self"),
           "The minimum tof value for the list of the events.")
      .def("getTofMax", &IEventList::getTofMax, args("self"),
           "The maximum tof value for the list of the events.")
      .def("multiply",
           (void (IEventList::*)(const double, const double)) &
               IEventList::multiply,
           args("self", "value", "error"),
           "Multiply the weights in this event "
           "list by a scalar variable with an "
           "error; though the error can be 0.0")
      .def("divide",
           (void (IEventList::*)(const double, const double)) &
               IEventList::divide,
           args("self", "value", "error"),
           "Divide the weights in this event "
           "list by a scalar with an "
           "(optional) error.");
}
