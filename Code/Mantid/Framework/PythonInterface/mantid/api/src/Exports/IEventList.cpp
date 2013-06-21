#include "MantidAPI/IEventList.h"
#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <vector>
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"


using Mantid::API::IEventList;
using Mantid::API::EventType;
using Mantid::API::TOF;
using Mantid::API::WEIGHTED;
using Mantid::API::WEIGHTED_NOTIME;

namespace Policies = Mantid::PythonInterface::Policies;
namespace Converters = Mantid::PythonInterface::Converters;
using namespace boost::python;


/// return_value_policy for copied numpy array
typedef return_value_policy<Policies::VectorToNumpy> return_clone_numpy;

void export_IEventList()
{
  register_ptr_to_python<IEventList *>();

  enum_<EventType>("EventType")
    .value("TOF", TOF)
    .value("WEIGHTED", WEIGHTED)
    .value("WEIGHTED_NOTIME", WEIGHTED_NOTIME)
    .export_values();

  class_< IEventList, boost::noncopyable >("IEventList", no_init)
    .def("getEventType", &IEventList::getEventType, "Return the type of events stored.")
    .def("switchTo", &IEventList::switchTo, "Switch the event type to the one specified")
    .def("clear", &IEventList::clear, "Clears the event list")
    .def("isSortedByTof", &IEventList::isSortedByTof, "Returns true if the list is sorted in TOF")
    .def("getNumberEvents", &IEventList::getNumberEvents, "Returns the number of events within the list")
    .def("getMemorySize", &IEventList::getMemorySize, "Returns the memory size in bytes")
    .def("integrate", &IEventList::integrate, "Integrate the events between a range of X values, or all events.")
    .def("convertTof", &IEventList::convertTof, "Convert the time of flight by tof'=tof*factor+offset")
    .def("scaleTof", &IEventList::scaleTof, "Convert the tof units by scaling by a multiplier.")
    .def("addTof", &IEventList::addTof, "Add an offset to the TOF of each event in the list.")
    .def("addPulsetime", &IEventList::addPulsetime, "Add an offset to the pulsetime (wall-clock time) of each event in the list.")
    .def("maskTof", &IEventList::maskTof, "Mask out events that have a tof between tofMin and tofMax (inclusively)")
    .def("getTofs", (std::vector<double>(IEventList::*)(void)const) &IEventList::getTofs,return_clone_numpy(),
        "Get a vector of the TOFs of the events")
	.def("getWeights", (std::vector<double>(IEventList::*)(void)const) &IEventList::getWeights,return_clone_numpy(),
        "Get a vector of the weights of the events")
	.def("getWeightErrors", (std::vector<double>(IEventList::*)(void)const) &IEventList::getWeightErrors,return_clone_numpy(),
        "Get a vector of the weights of the events")
    .def("getPulseTimes", &IEventList::getPulseTimes, "Get a vector of the pulse times of the events")
    .def("getTofMin", &IEventList::getTofMin, "The minimum tof value for the list of the events.")
    .def("getTofMax", &IEventList::getTofMax, "The maximum tof value for the list of the events.")
    .def("multiply", (void(IEventList::*)(const double,const double)) &IEventList::multiply,
        "Multiply the weights in this event list by a scalar variable with an error; though the error can be 0.0")
    .def("divide", (void(IEventList::*)(const double,const double)) &IEventList::divide,
         "Divide the weights in this event list by a scalar with an (optional) error.")
      ;

}

