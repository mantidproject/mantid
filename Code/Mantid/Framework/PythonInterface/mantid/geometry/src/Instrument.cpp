#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::Instrument;
using Mantid::Geometry::Instrument_sptr;
using Mantid::Geometry::Instrument_const_sptr;
using Mantid::Geometry::CompAssembly;
using Mantid::Geometry::IObjComponent;
using Mantid::Geometry::IDetector;
using Mantid::Geometry::IComponent;
using Mantid::Geometry::ReferenceFrame;
using Mantid::detid_t;
using namespace boost::python;

void export_Instrument()
{
  register_ptr_to_python<Instrument_sptr>();
  register_ptr_to_python<Instrument_const_sptr>();

  class_<Instrument, bases<CompAssembly>, boost::noncopyable>("Instrument", no_init)
    .def("getSample", (boost::shared_ptr<IObjComponent> (Instrument::*)())&Instrument::getSample, 
      "Return the object that represents the sample")
    .def("getSource", (boost::shared_ptr<IObjComponent> (Instrument::*)())&Instrument::getSource,
         "Return the object that represents the source")
    .def("getComponentByName", (boost::shared_ptr<IComponent> (Instrument::*)(const std::string&))&Instrument::getComponentByName,
         "Returns the named component")
    .def("getDetector", (boost::shared_ptr<IDetector> (Instrument::*)(const detid_t&)const)&Instrument::getDetector, 
         "Returns the dector with the given ID")
    .def("getReferenceFrame", (boost::shared_ptr<const ReferenceFrame> (Instrument::*)())&Instrument::getReferenceFrame )
      ;
    ;
    
;
}

