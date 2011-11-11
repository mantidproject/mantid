#ifndef MANTID_PYTHONINTERFACE_STLEXPORTDEFINITIONS_H_
#define MANTID_PYTHONINTERFACE_STLEXPORTDEFINITIONS_H_
/**
    This file contains the export definitions for various stl containers.

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include <boost/python/class.hpp>
#include <boost/python/init.hpp>
#ifdef _MSC_VER
  #pragma warning(disable:4267) // Kill warning from line 176 of indexing suite
#endif
  #include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <vector>
#include <set>

namespace Mantid
{
  namespace PythonInterface
  {
    /**
     * Convert an element type within a sequence to a string for printing
     * @param value :: The elements value
     * @return A string representation of the value for printing within a sequence
     */
    template<typename ElementType>
    std::string toString(const ElementType & value)
    {
      std::ostringstream os;
      os << value;
      return os.str();
    }

    /**
     * Convert a string element within a sequence to a string for printing
     * @param value :: The elements value
     * @return A string representation of the value for printing within a sequence, i.e.
     * wrapped in single quotes to emulate printing a python sequence of strings
     */
    template<>
    std::string toString(const std::string & value)
    {
      std::ostringstream os;
      os << value;
      return os.str();
    }


    /// Convert a sequence of values to a string for printing
    template<typename SequenceType,typename ElementType>
    std::string toString(const SequenceType & values)
    {
      typename SequenceType::const_iterator iend = values.end();
      std::ostringstream os;
      std::string retval;
      for( typename SequenceType::const_iterator itr = values.begin(); itr != iend; )
      {
        os << toString(*itr);
        os << *itr;
        if( ++itr != iend )
        {
          retval += ",";
        }
      }
      return retval;
    }

    /**
     * A struct to help export std::vector types
     */
    template <typename ElementType>
    struct std_vector_exporter
    {
      ///A typedef of a vector of template ElementTypes
      typedef std::vector<ElementType> w_t;

      static std::string to_string(const w_t & values)
      {
        if( values.empty() ) return "[]";
        std::string retval("[");
        retval += toString<w_t, ElementType>(values);
        retval += "]";
        return retval;
      }

      ///a python wrapper
      static void wrap(std::string const& python_name)
      {
        boost::python::class_<w_t, std::auto_ptr<w_t> >(python_name.c_str())
          .def(boost::python::init<w_t const&>())
          .def(boost::python::vector_indexing_suite<w_t>())
          .def("__str__", &std_vector_exporter::to_string)
        ;
     }

    };

  }
}

#endif /* MANTID_PYTHONINTERFACE_STLEXPORTDEFINITIONS_H_ */
