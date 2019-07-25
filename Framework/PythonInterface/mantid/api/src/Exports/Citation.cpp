// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/Citation.h"
#include "MantidKernel/WarningSuppressions.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/overloads.hpp>

using namespace Mantid::API;
using namespace boost::python;

void export_Citation() {
  class_<Citation, boost::noncopyable>(
      "Citation", init<optional<std::string, std::string, std::string,
                                std::string, std::string>>())
      .def(init<NeXus::File *, std::string>())
      .def(init<BaseCitation>())
      .def("description", &Citation::description, arg("self"),
           "Returns the description on the citation object")
      .def("url", &Citation::url, arg("self"),
           "Returns the url on the citation object")
      .def("doi", &Citation::doi, arg("self"),
           "Returns the doi on the citation object")
      .def("bibtex", &Citation::bibtex, arg("self"),
           "Returns the bibtex formatted citation from the citation object")
      .def("endnote", &Citation::endnote, arg("self"),
           "Returns the endnote formatted citation from the citation object")
      .def("loadNexus", &Citation::loadNexus,
           (arg("self"), arg("file"), arg("group")),
           "Load data into this object from a NeXus file")
      .def("saveNexus", &Citation::saveNexus,
           (arg("self"), arg("file"), arg("group")),
           "Save data from this object to a NeXus file")
      .def("__eq__", &Citation::operator==);
}
