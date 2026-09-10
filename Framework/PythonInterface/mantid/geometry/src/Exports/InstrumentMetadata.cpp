// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/InstrumentMetadata.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>

using Mantid::Geometry::InstrumentMetadata;
using namespace boost::python;

void export_InstrumentMetadata() {
  // Read-only: built at instrument load and never mutated thereafter, so there is no
  // equivalent of Instrument.setFilename() here.
  class_<InstrumentMetadata, boost::noncopyable>("InstrumentMetadata", no_init)
      .def("validFromDate", &InstrumentMetadata::validFromDate, arg("self"),
           return_value_policy<copy_const_reference>(),
           "Return the valid-from :class:`~mantid.kernel.DateAndTime` of the instrument.")

      .def("validToDate", &InstrumentMetadata::validToDate, arg("self"), return_value_policy<copy_const_reference>(),
           "Return the valid-to :class:`~mantid.kernel.DateAndTime` of the instrument.")

      .def("filename", &InstrumentMetadata::filename, arg("self"), return_value_policy<copy_const_reference>(),
           "Return the name of the file that the original IDF was from.")

      .def("xmlText", &InstrumentMetadata::xmlText, arg("self"), return_value_policy<copy_const_reference>(),
           "Return the instrument XML.")

      .def("defaultView", &InstrumentMetadata::defaultView, arg("self"), return_value_policy<copy_const_reference>(),
           "Return the name of the preferred view in instrument view.")

      .def("defaultAxis", &InstrumentMetadata::defaultAxis, arg("self"), return_value_policy<copy_const_reference>(),
           "Return the axis the instrument view is wrapped around by default.");
}
