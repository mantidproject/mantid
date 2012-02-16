#include "MantidAPI/Run.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/args.hpp>

using Mantid::API::Run;
using namespace boost::python;

namespace
{
  namespace bpl = boost::python;

  /**
   * Emulate dict.get. Returns the value pointed to by the key or the default given
   * @param self The object called on
   * @param key The key
   * @param default_ The default to return if it does not exist
   */
  bpl::object getWithDefault(bpl::object self, bpl::object key, bpl::object default_)
  {
    bpl::object exists(self.attr("__contains__"));
    if( extract<bool>(exists(key))() )
    {
      return self.attr("__getitem__")(key);
    }
    else
    {
      return default_;
    }
  }

  /**
   * Emulate dict.get. Returns the value pointed to by the key or None if it doesn't exist
   * @param self The bpl::object called on
   * @param key The key
   */
  bpl::object get(bpl::object self, bpl::object key)
  {
    return getWithDefault(self, key, bpl::object());
  }


  /**
     * Add a property with the given name and value
     * @param self A reference to the run object that we have been called on
     * @param name The name of the new property
     * @param value The value of the property
     * @param units A string representing a unit
     * @param replace If true, replace an existing property with this one else raise an error
     */
    void addPropertyWithUnit(Run & self, const std::string & name, PyObject *value, const std::string & units, bool replace)
    {
      if( PyFloat_Check(value) )
      {
        self.addProperty(name, extract<double>(value)(), units, replace);
      }
      else if( PyInt_Check(value) )
      {
        self.addProperty(name, extract<long>(value)(), units, replace);
      }
      else if( PyString_Check(value) )
      {
        self.addProperty(name, extract<std::string>(value)(), units, replace);
      }
      else
      {
        std::ostringstream msg;
        msg << "Run::addProperty - Unknown value type given: " << value->ob_type->tp_name;
        throw std::invalid_argument(msg.str());
      }
    }

  /**
   * Add a property with the given name and value
   * @param self A reference to the run object that we have been called on
   * @param name The name of the new property
   * @param value The value of the property
   * @param replace If true, replace an existing property with this one else raise an error
   */
  void addProperty(Run & self, const std::string & name, PyObject *value, bool replace)
  {
    addPropertyWithUnit(self, name, value, "", replace);
  }

  /**
   * Add a property with the given name and value. An existing property is overwritten if it exists
   * @param self A reference to the run object that we have been called on
   * @param name The name of the new property
   * @param value The value of the property
   */
  void addOrReplaceProperty(Run & self, const std::string & name, PyObject *value)
  {
    addProperty(self, name, value, true);
  }


}

void export_Run()
{
  //Pointer
  register_ptr_to_python<Run*>();

  //Run class
  class_< Run,  boost::noncopyable >("Run", no_init)
    .def("getProtonCharge", &Run::getProtonCharge, "Return the total good proton charge for the run")
    .def("hasProperty", &Run::hasProperty, "Returns True if the given log value is contained within the run")
    .def("getProperty", &Run::getProperty, return_value_policy<return_by_value>(), "Returns the named property (log value). Use '.value' to return the value.")
    .def("getProperties", &Run::getProperties, return_internal_reference<>(), "Return the list of run properties managed by this object.")
    .def("addProperty", &addProperty, "Adds a property with the given name and value. If replace=True then an existing property is overwritten")
    .def("addProperty", &addPropertyWithUnit, "Adds a property with the given name, value and unit. If replace=True then an existing property is overwritten")
    //--------------------------- Dictionary access----------------------------
    .def("get", &getWithDefault, "Returns the value pointed to by the key or None if it does not exist")
    .def("get", &get, "Returns the value pointed to by the key or the default value given")
    .def("__contains__", &Run::hasProperty)
    .def("__getitem__", &Run::getProperty, return_value_policy<return_by_value>())
    .def("__setitem__", &addOrReplaceProperty)

   ;
}

