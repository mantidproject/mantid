// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
/**
    This file contains the export definitions for various stl containers.
*/
#include "MantidKernel/WarningSuppressions.h"
GNU_DIAG_OFF("maybe-uninitialized")

#include <boost/python/class.hpp>
#include <boost/python/init.hpp>
#ifdef _MSC_VER
#pragma warning(disable : 4267) // Kill warning from line 176 of indexing suite
#endif
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/tuple.hpp>

#include <set>
#include <vector>

using boost::python::arg;

namespace Mantid {
namespace PythonInterface {
/**
 * Convert an element type within a sequence to a string for printing
 * @param value :: The elements value
 * @return A string representation of the value for printing within a sequence
 */
template <typename ElementType> inline std::string toString(const ElementType &value) {
  std::ostringstream os;
  os << value;
  return os.str();
}

/**
 * Convert a string element within a sequence to a string for printing
 * @param value :: The elements value
 * @return A string representation of the value for printing within a sequence,
 * i.e.
 * wrapped in single quotes to emulate printing a python sequence of strings
 */
template <> inline std::string toString(const std::string &value) {
  std::ostringstream os;
  os << "'" << value << "'";
  return os.str();
}

/// Convert a sequence of values to a string for printing
template <typename SequenceType, typename ElementType> std::string toString(const SequenceType &values) {
  auto iend = values.end();
  std::ostringstream os;
  for (auto itr = values.begin(); itr != iend;) {
    os << toString(*itr);
    if (++itr != iend) {
      os << ",";
    }
  }
  return os.str();
}

/**
 * A struct to help export std::vector types
 */
template <typename ElementType, bool NoIndexingProxy = false> struct std_vector_exporter {
  /// A typedef of a vector of template ElementTypes
  using w_t = std::vector<ElementType>;

  static std::string to_string(const w_t &values) {
    if (values.empty())
      return "[]";
    std::string retval("[");
    retval += toString<w_t, ElementType>(values);
    retval += "]";
    return retval;
  }

  /// a python wrapper
  static void wrap(std::string const &python_name) {
    boost::python::class_<w_t, std::shared_ptr<w_t>>(python_name.c_str())
        .def(boost::python::init<w_t const &>())
        .def(boost::python::vector_indexing_suite<w_t, NoIndexingProxy>())
        .def("__str__", &std_vector_exporter::to_string);
  }
};

//-------------------------------------------------------------------------
// std::set
//-------------------------------------------------------------------------

/// std::set wrapper
// Found this at
// http://cctbx.svn.sourceforge.net/viewvc/cctbx/trunk/scitbx/stl/set_wrapper.h?view=log
template <typename ElementType> struct std_set_exporter {
  using w_t = std::set<ElementType>;
  using e_t = ElementType;

  static void insert_element(w_t &self, e_t const &x) { self.insert(x); }

  static void insert_set(w_t &self, w_t const &other) { self.insert(other.begin(), other.end()); }

  static bool contains(w_t const &self, e_t const &x) { return self.find(x) != self.end(); }

  static e_t getitem(w_t const &self, std::size_t i) {
    if (i >= self.size()) {
      PyErr_SetString(PyExc_IndexError, "Index out of range");
      boost::python::throw_error_already_set();
    }
    return *std::next(self.begin(), i);
  }

  static boost::python::tuple getinitargs(w_t const &self) {
    return boost::python::make_tuple(boost::python::tuple(self));
  }

  static std::string to_string(const w_t &values) {
    if (values.empty())
      return "set()";
    std::string retval("set(");
    retval += toString<w_t, ElementType>(values);
    retval += ")";
    return retval;
  }

  static void wrap(std::string const &python_name) {
    boost::python::class_<w_t, std::shared_ptr<w_t>>(python_name.c_str())
        .def(boost::python::init<w_t const &>())
        // special methods
        .def("__str__", &std_set_exporter::to_string, arg("self"))
        .def("__len__", &w_t::size, arg("self"))
        .def("__contains__", contains, (arg("self"), arg("element")))
        .def("__getitem__", getitem, (arg("self"), arg("index")))
        .def("__getinitargs__", getinitargs, arg("self"))
        // Standard methods
        .def("size", &w_t::size, arg("self"))
        .def("insert", insert_element, (arg("self"), arg("element")))
        .def("append", insert_element, (arg("self"), arg("element")))
        .def("insert", insert_set, (arg("self"), arg("set")))
        .def("extend", insert_set, (arg("self"), arg("set")))
        .def("erase", (std::size_t(w_t::*)(e_t const &))&w_t::erase, (arg("self"), arg("index")))
        .def("clear", &w_t::clear, arg("self"))
        .enable_pickling()

        ;
  }
};
} // namespace PythonInterface
} // namespace Mantid
