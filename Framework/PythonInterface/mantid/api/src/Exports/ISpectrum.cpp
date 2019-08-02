// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ISpectrum.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::ISpectrum;
using Mantid::detid_t;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(ISpectrum)

void export_ISpectrum() {
  register_ptr_to_python<ISpectrum *>();

  class_<ISpectrum, boost::noncopyable>("ISpectrum", no_init)
      .def("hasDetectorID", &ISpectrum::hasDetectorID,
           (arg("self"), arg("det_id")),
           "Returns True if the spectrum contain the given spectrum number")
      .def("getSpectrumNo", &ISpectrum::getSpectrumNo, arg("self"),
           "Returns the spectrum number of this spectrum")
      .def("getDetectorIDs",
           (const std::set<detid_t> &(ISpectrum::*)() const) &
               ISpectrum::getDetectorIDs,
           arg("self"), return_value_policy<copy_const_reference>(),
           "Returns a list of detector IDs for this spectrum")
      .def("addDetectorID", &ISpectrum::addDetectorID,
           (arg("self"), arg("det_id")), "Add a detector ID to this spectrum")
      .def("setDetectorID", &ISpectrum::setDetectorID,
           (arg("self"), arg("det_id")), "Set the given ID has the only")
      .def("clearDetectorIDs", &ISpectrum::clearDetectorIDs, arg("self"),
           "Clear the set of detector IDs")
      .def("setSpectrumNo", &ISpectrum::setSpectrumNo,
           (arg("self"), arg("num")),
           "Set the spectrum number for this spectrum")
      .def("hasDx", &ISpectrum::hasDx, arg("self"),
           "Returns True if the spectrum uses the "
           "DX (X Error) array, else False.");
}
