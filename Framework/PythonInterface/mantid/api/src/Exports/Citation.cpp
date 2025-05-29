// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/Citation.h"
#include "MantidKernel/WarningSuppressions.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/overloads.hpp>

using namespace Mantid::API;
using namespace boost::python;

void export_Citation() {
  using return_copy = return_value_policy<copy_const_reference>;

  class_<Citation, boost::noncopyable>("Citation",
                                       init<optional<const std::string &, const std::string &, const std::string &,
                                                     const std::string &, const std::string &>>())
      .def(init<Mantid::Nexus::File *, const std::string &>())
      .def("description", &Citation::description, arg("self"), return_copy(),
           "Returns the description on the citation object")
      .def("url", &Citation::url, arg("self"), return_copy(), "Returns the url on the citation object")
      .def("doi", &Citation::doi, arg("self"), return_copy(), "Returns the doi on the citation object")
      .def("bibtex", &Citation::bibtex, arg("self"), return_copy(),
           "Returns the bibtex formatted citation from the citation object")
      .def("endnote", &Citation::endnote, arg("self"), return_copy(),
           "Returns the endnote formatted citation from the citation object")
      .def("saveNexus", &Citation::saveNexus, (arg("self"), arg("file"), arg("group")), return_copy(),
           "Save data from this object to a NeXus file")
      .def("__eq__", &Citation::operator==);
}
