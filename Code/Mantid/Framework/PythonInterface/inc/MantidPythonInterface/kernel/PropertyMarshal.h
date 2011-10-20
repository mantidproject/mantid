#ifndef MANTID_PYTHONINTERFACE_PROPERTYMARSHAL_H_
#define MANTID_PYTHONINTERFACE_PROPERTYMARSHAL_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/DataItem.h"

#include <boost/python/object.hpp>
#include <boost/python/extract.hpp>

#include <string>
#include <iostream>

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
     * The helpers declared here deal with calling the correct function depending
     * on the type passed to it.

     * We will also need more marshaling for these functions as we want be able to
     * pass numpy arrays seamlessly to algorithms.
     */

    /**
     * A non-templated base class that can be stored in a map so that
     * its virtual functions are overridden in templated derived classes
     * that can extract the correct type from the Python object
     */
    struct DLLExport PropertyHandler
    {
      /// Virtual Destructor
      virtual ~PropertyHandler() {};
      /// Virtual set function to handle Python -> C++ calls
      virtual void set(Kernel::IPropertyManager* alg, const std::string &name, boost::python::object value) = 0;
    };
    /**
     * A templated marshal that calls the corresponding setProperty method on the given algorithm
     */
    template<typename CType>
    struct DLLExport TypedHandler : public PropertyHandler
    {
      /// Set function to handle Python -> C++ calls and get the correct type
      void set(Kernel::IPropertyManager* alg, const std::string &name, boost::python::object value)
      {
        // Seems like a lot for 1 line of code but for more complex types we'll
        // need to specialise
        alg->setProperty<CType>(name, boost::python::extract<CType>(value));
      }
    };

    /**
     * Specialized string version to avoid a current bug where string property values are not
     * assigned polymorphically. This can be removed when the bug is fixed
     */
    template<>
    struct DLLExport TypedHandler<std::string> : public PropertyHandler
    {
      /// Set function to handle Python -> C++ calls and get the correct type
      void set(Kernel::IPropertyManager* alg, const std::string &name, boost::python::object value)
      {
        // Seems like a lot for 1 line of code but for more complex types we'll
        // need to specialise
        alg->setPropertyValue(name, boost::python::extract<std::string>(value));
      }
    };

    //------------------------------------------------------------------------------------------------
    /**
     * Defines static functions that allow method calls on Python
     * objects to be routed here
     */
    class DLLExport PropertyMarshal
    {
    public:
      /// Typedef the map of python types to C++ functions
      typedef std::map<PyTypeObject*, PropertyHandler*> PyTypeLookup;
    public:
      /// This static function allows a call to a method on an IAlgorithm object
      static void setProperty(boost::python::object self, const std::string & name,
                              boost::python::object value);

      /// Insert a new property handler
      static void insert(PyTypeObject* typeObject, PropertyHandler* handler);
    private:
      /// Map a python type object to a C++ setter
      static PyTypeLookup g_handlers;
    };

  }
}

/**
 * A macro to declare property handlers
 *
 * @param export_type :: The C++ type that is to be converted
 * @param base_type :: The C++ type that the export_type is to be treated as
 */
#define DECLARE_PROPERTYHANDLER(export_type, base_type) \
  const boost::python::converter::registration *reg = boost::python::converter::registry::query(typeid(export_type));\
  Mantid::PythonInterface::PropertyMarshal::insert(reg->get_class_object(), new Mantid::PythonInterface::TypedHandler<base_type>());\



#endif /* MANTID_PYTHONINTERFACE_PROPERTYMARSHAL_H_ */
