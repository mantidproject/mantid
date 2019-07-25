// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/CitationConstructorHelpers.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/pure_virtual.hpp>

using namespace Mantid::API;
using namespace boost::python;

void export_CitationConstructorHelpers() {
  class_<BaseCitation, boost::noncopyable>("BaseCitation", no_init)
      .def("toEndNote", pure_virtual(&BaseCitation::toEndNote), arg("self"),
           "Return endnote formatted citation")
      .def("toBibTex", pure_virtual(&BaseCitation::toBibTex), arg("self"),
           "Return bibtex formatted citation")
      .def_readwrite("m_doi", &BaseCitation::m_doi)
      .def_readwrite("m_description", &BaseCitation::m_description)
      .def_readwrite("m_url", &BaseCitation::m_url);

  class_<ArticleCitation, bases<BaseCitation>>(
      "ArticleCitation",
      init<std::vector<std::string>, std::string, std::string, std::string,
           optional<std::string, std::string, std::string, std::string,
                    std::string, std::string, std::string>>())
      .def("toEndNote", &ArticleCitation::toEndNote, arg("self"),
           "Return endnote formatted citation")
      .def("toBibTex", &ArticleCitation::toBibTex, arg("self"),
           "Return bibtex formatted citation");

  class_<BookCitation, bases<BaseCitation>>(
      "BookCitation",
      init<std::vector<std::string>, std::string, std::string, std::string,
           optional<std::string, std::string, std::string, std::string,
                    std::string, std::string, std::string, std::string>>())
      .def("toEndNote", &BookCitation::toEndNote, arg("self"),
           "Return endnote formatted citation")
      .def("toBibTex", &BookCitation::toBibTex, arg("self"),
           "Return bibtex formatted citation");

  class_<BookletCitation, bases<BaseCitation>>(
      "BookletCitation",
      init<std::string, optional<std::vector<std::string>, std::string,
                                 std::string, std::string, std::string,
                                 std::string, std::string, std::string>>())
      .def("toEndNote", &BookletCitation::toEndNote, arg("self"),
           "Return endnote formatted citation")
      .def("toBibTex", &BookletCitation::toBibTex, arg("self"),
           "Return bibtex formatted citation");

  class_<InBookCitation, bases<BaseCitation>>(
      "InBookCitation", init<std::vector<std::string>, std::string, std::string,
                             std::string, std::string,
                             optional<std::string, std::string, std::string,
                                      std::string, std::string, std::string,
                                      std::string, std::string, std::string>>())
      .def("toEndNote", &InBookCitation::toEndNote, arg("self"),
           "Return endnote formatted citation")
      .def("toBibTex", &InBookCitation::toBibTex, arg("self"),
           "Return bibtex formatted citation");

  class_<InCollectionCitation, bases<BaseCitation>>(
      "InCollectionCitation",
      init<std::vector<std::string>, std::string, std::string, std::string,
           std::string,
           optional<std::string, std::string, std::string, std::string,
                    std::string, std::string, std::string, std::string,
                    std::string, std::string, std::string>>())
      .def("toEndNote", &InCollectionCitation::toEndNote, arg("self"),
           "Return endnote formatted citation")
      .def("toBibTex", &InCollectionCitation::toBibTex, arg("self"),
           "Return bibtex formatted citation");

  class_<InProceedingsCitation, bases<BaseCitation>>(
      "InProceedingsCitation",
      init<std::vector<std::string>, std::string, std::string, std::string,
           optional<std::string, std::string, std::string, std::string,
                    std::string, std::string, std::string, std::string,
                    std::string, std::string, std::string>>())
      .def("toEndNote", &InProceedingsCitation::toEndNote, arg("self"),
           "Return endnote formatted citation")
      .def("toBibTex", &InProceedingsCitation::toBibTex, arg("self"),
           "Return bibtex formatted citation");

  class_<ManualCitation, bases<BaseCitation>>(
      "ManualCitation",
      init<std::string,
           optional<std::vector<std::string>, std::string, std::string,
                    std::string, std::string, std::string, std::string,
                    std::string, std::string>>())
      .def("toEndNote", &ManualCitation::toEndNote, arg("self"),
           "Return endnote formatted citation")
      .def("toBibTex", &ManualCitation::toBibTex, arg("self"),
           "Return bibtex formatted citation");

  class_<MastersThesisCitation, bases<BaseCitation>>(
      "MastersThesisCitation",
      init<std::vector<std::string>, std::string, std::string, std::string,
           optional<std::string, std::string, std::string, std::string,
                    std::string, std::string>>())
      .def("toEndNote", &MastersThesisCitation::toEndNote, arg("self"),
           "Return endnote formatted citation")
      .def("toBibTex", &MastersThesisCitation::toBibTex, arg("self"),
           "Return bibtex formatted citation");

  class_<MiscCitation, bases<BaseCitation>>(
      "MiscCitation", init<optional<std::vector<std::string>, std::string,
                                    std::string, std::string, std::string,
                                    std::string, std::string, std::string>>())
      .def("toEndNote", &MiscCitation::toEndNote, arg("self"),
           "Return endnote formatted citation")
      .def("toBibTex", &MiscCitation::toBibTex, arg("self"),
           "Return bibtex formatted citation");
}