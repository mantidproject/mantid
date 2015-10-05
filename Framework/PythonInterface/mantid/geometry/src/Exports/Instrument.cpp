#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidPythonInterface/kernel/Policies/RemoveConst.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::Geometry;
using Mantid::detid_t;
using namespace boost::python;
using Mantid::PythonInterface::Policies::RemoveConstSharedPtr;

void export_Instrument() {
  register_ptr_to_python<boost::shared_ptr<Instrument>>();

  class_<Instrument, bases<CompAssembly>, boost::noncopyable>("Instrument",
                                                              no_init)
      .def("getSample", &Instrument::getSample,
           return_value_policy<RemoveConstSharedPtr>(),
           "Return the object that represents the sample")

      .def("getSource", &Instrument::getSource,
           return_value_policy<RemoveConstSharedPtr>(),
           "Return the object that represents the source")

      .def("getComponentByName",
           (boost::shared_ptr<IComponent>(Instrument::*)(const std::string &)) &
               Instrument::getComponentByName,
           "Returns the named component")

      .def("getDetector", (boost::shared_ptr<IDetector>(
                              Instrument::*)(const detid_t &) const) &
                              Instrument::getDetector,
           "Returns the detector with the given ID")

      .def("getReferenceFrame",
           (boost::shared_ptr<const ReferenceFrame>(Instrument::*)()) &
               Instrument::getReferenceFrame,
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the reference frame attached that defines the instrument "
           "axes")

      .def("getValidFromDate", &Instrument::getValidFromDate,
           "Return the valid from date of the instrument")

      .def("getValidToDate", &Instrument::getValidToDate,
           "Return the valid to date of the instrument")

      .def("getBaseInstrument", &Instrument::baseInstrument,
           return_value_policy<RemoveConstSharedPtr>(),
           "Return reference to the base instrument");
  ;

  ;
}
