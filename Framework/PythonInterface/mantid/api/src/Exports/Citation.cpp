// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/Citation.h"
#include "MantidAPI/CitationConstructorHelpers.h"
#include "MantidKernel/WarningSuppressions.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/overloads.hpp>

using namespace Mantid::API;
using namespace boost::python;

std::string description(Citation &self) { return self.description(); }
std::string url(Citation &self) { return self.url(); }
std::string doi(Citation &self) { return self.doi(); }
std::string bibtex(Citation &self) { return self.bibtex(); }
std::string endnote(Citation &self) { return self.endnote(); }

void export_Citation() {
  class_<Citation, boost::noncopyable>(
      "Citation", init<optional<const std::string &, const std::string &,
                                const std::string &, const std::string &,
                                const std::string &>>())
      .def(init<NeXus::File *, const std::string &>())
      .def(init<const BaseCitation &>())
      .def("description", &description, arg("self"),
           "Returns the description on the citation object")
      .def("url", &url, arg("self"), "Returns the url on the citation object")
      .def("doi", &doi, arg("self"), "Returns the doi on the citation object")
      .def("bibtex", &bibtex, arg("self"),
           "Returns the bibtex formatted citation from the citation object")
      .def("endnote", &endnote, arg("self"),
           "Returns the endnote formatted citation from the citation object")
      .def("loadNexus", &Citation::loadNexus,
           (arg("self"), arg("file"), arg("group")),
           "Load data into this object from a NeXus file")
      .def("saveNexus", &Citation::saveNexus,
           (arg("self"), arg("file"), arg("group")),
           "Save data from this object to a NeXus file")
      .def("__eq__", &Citation::operator==);
}
