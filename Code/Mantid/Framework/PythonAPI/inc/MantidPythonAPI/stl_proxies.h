#ifndef STL_PROXIES_H_
#define STL_PROXIES_H_

#include <vector>
#include <set>
#include <sstream>

#include "std_operator_definitions.h"
#include "MantidPythonAPI/BoostPython_Silent.h"

namespace Mantid
{
namespace PythonAPI
{
  //@cond Disable Doxygen for Python bindings
  /// Prefix a value with a string when it is printed
  template<typename TYPE>
  std::string toStringPrefix(const TYPE & value)
  {
    (void)value;
    return "";
  }

  /// Prefix a value with a string when it is printed (std::string specialization)
  template<>
  inline std::string toStringPrefix(const std::string& value)
  {
    (void)value;
    return "'";
  }

  /// Suffix a value with a string when it is printed
  template<typename TYPE>
  std::string toStringSuffix(const TYPE & value)
  {
    (void)value;
    return "";
  }

  /// Suffix a value with a string when it is printed (std::string specialization)
  template<>
  inline std::string toStringSuffix(const std::string& value)
  {
    (void)value;
    return "'";
  }

  /// Convert a sequence of values to a string for printing
  template<typename SequenceType,typename ElementType>
  std::string sequence_to_string(const SequenceType & values)
  {
    typename SequenceType::const_iterator iend = values.end();
    std::ostringstream os;
    std::string retval;
    for( typename SequenceType::const_iterator itr = values.begin(); itr != iend; )
    {
      os << *itr;
      retval += toStringPrefix<ElementType>(*itr);
      retval += os.str();
      os.str("");
      retval += toStringSuffix<ElementType>(*itr);
      if( ++itr != iend )
      {
        retval += ",";
      }
    }
    return retval;
  }

  /// std::vector wrapper
  template <typename ElementType>
  struct vector_proxy
  {
    ///A typedef of a vector of tempolate ElementTypes
    typedef std::vector<ElementType> w_t;

    static std::string to_string(const w_t & values)
    {
      if( values.empty() ) return "[]";
      std::string retval("[");
      retval += sequence_to_string<w_t, ElementType>(values);
      retval += "]";
      return retval;
    }

    ///a python wrapper
    static void wrap(std::string const& python_name)
    {
      boost::python::class_<w_t, std::auto_ptr<w_t> >(python_name.c_str())
        .def(boost::python::init<w_t const&>())
	      .def(boost::python::vector_indexing_suite<w_t>())
        .def("__str__", &vector_proxy::to_string)
      ;
   }

  };
  
  /// std::set wrapper
  // Found this at http://cctbx.svn.sourceforge.net/viewvc/cctbx/trunk/scitbx/stl/set_wrapper.h?view=log
  template <typename ElementType>
  struct set_proxy
  {
    typedef std::set<ElementType> w_t;
    typedef ElementType e_t;

    static void
    insert_element(w_t& self, e_t const& x) { self.insert(x); }

    static void
    insert_set(w_t& self, w_t const& other)
    {
      self.insert(other.begin(), other.end());
    }

    static bool
    contains(w_t const& self, e_t const& x)
    {
      return self.find(x) != self.end();
    }

    static e_t
    getitem(w_t const& self, std::size_t i)
    {
      if (i >= self.size()) 
      {
        PyErr_SetString(PyExc_IndexError, "Index out of range");
        boost::python::throw_error_already_set();
      }
      typename w_t::const_iterator p = self.begin();
      while (i > 0) { p++; i--; }
      return *p;
    }

    static boost::python::tuple
    getinitargs(w_t const& self)
    {
      return boost::python::make_tuple(boost::python::tuple(self));
    }

    static std::string to_string(const w_t & values)
    {
      if( values.empty() ) return "set()";
      std::string retval("set(");
      retval += sequence_to_string<w_t, ElementType>(values);
      retval += ")";
      return retval;
    }

    static void wrap(std::string const& python_name)
    {
      boost::python::class_<w_t, std::auto_ptr<w_t> >(python_name.c_str())
        .def(boost::python::init<w_t const&>())
        //special methods
        .def("__str__", &set_proxy::to_string)
        .def("__len__", &w_t::size)
        .def("__contains__", contains)
        .def("__getitem__", getitem)
        .def("__getinitargs__", getinitargs)
        // Standard methods
        .def("size", &w_t::size)
        .def("insert", insert_element)
        .def("append", insert_element)
        .def("insert", insert_set)
        .def("extend", insert_set)
        .def("erase", (std::size_t(w_t::*)(e_t const&)) &w_t::erase)
        .def("clear", &w_t::clear)
        .enable_pickling()
        
      ;
    }
  };
//@endcond
}
}

#endif //STL_PROXIES_H_
