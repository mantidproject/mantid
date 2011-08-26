#ifndef MANTID_PYTHONINTERFACE_PROPERTYMARSHAL_H_
#define MANTID_PYTHONINTERFACE_PROPERTYMARSHAL_H_

#include "MantidKernel/Property.h"
#include <boost/python/object.hpp>
#include <string>

namespace Mantid
{
  namespace PythonInterface
  {

    /**
     * Python is dynamically typed so that the type of a variable is not
     * known until run time. The [set,get]Property methods on an algorithm
     * expect the value passed/returned to match that of the declared property
     * type, i.e. an integer property must use alg.setProperty(name, int).
     *
     * Boost.Python is able to declare several 'overloaded' functions of the
     * same name but if the number of arguments are the same and only the
     * types differ it is not able to figure out the correct function to
     * call. It instead chooses the last from the exported list that contains
     * a type that the value can be converted to. This then raises an exception
     * in Mantid.
     *
     * The PropertyMarshal struct declared here deals with calling the correct
     * function depending on the type passed to it.

     * We will also need more marshaling for these functions as we want be able to
     * pass numpy arrays seamlessly to algorithms.
     *
     * The first argument of each function MUST be an object of type
     * boost::python::object. This provides access to the object that performed the
     * method call. It is equivalent to a python method that starts with 'self'
     */
    template<typename CType>
    struct PropertyMarshal
    {
      /// Set a named property to a given value
      static void setProperty(boost::python::object self, const std::string & name,
                CType value);
    };

    /// Retrieve a named property
    Kernel::Property * getProperty(boost::python::object self, const std::string & name);

  }
}

#endif /* MANTID_PYTHONINTERFACE_PROPERTYMARSHAL_H_ */
