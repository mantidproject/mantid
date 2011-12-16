#include "MantidAPI/ISpectrum.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/copy_const_reference.hpp>

using Mantid::API::ISpectrum;
using Mantid::detid_t;
using namespace boost::python;

void export_ISpectrum()
{
  register_ptr_to_python<ISpectrum*>();

  class_<ISpectrum, boost::noncopyable>("ISpectrum", no_init)
    .def("hasDetectorID", &ISpectrum::hasDetectorID, "Returns True if the spectrum contain the given spectrum number")
    .def("getSpectrumNo", &ISpectrum::getSpectrumNo, "Returns the spectrum number of this spectrum")
    .def("getDetectorIDs", (const std::set<detid_t>& (ISpectrum::*)() const)&ISpectrum::getDetectorIDs, return_value_policy<copy_const_reference>(),
         "Returns a list of detector IDs for this spectrum")
    .def("addDetectorID", &ISpectrum::addDetectorID, "Add a detector ID to this spectrum")
    .def("setDetectorID", &ISpectrum::setDetectorID, "Set the given ID has the only")
    .def("clearDetectorIDs", &ISpectrum::clearDetectorIDs, "Clear the set of detector IDs")
    .def("setSpectrumNo", &ISpectrum::setSpectrumNo, "Set the spectrum number for this spectrum")
    ;
}

