#include "MantidAPI/Run.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/list.hpp>
#include "MantidGeometry/Instrument/Goniometer.h"
#include <boost/python/copy_const_reference.hpp>

#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"

using Mantid::API::Run;
using Mantid::Kernel::Property;
using namespace boost::python;

namespace
{
  namespace bpl = boost::python;

  /**
     * Add a property with the given name and value
     * @param self A reference to the run object that we have been called on
     * @param name The name of the new property
     * @param value The value of the property
     * @param units A string representing a unit
     * @param replace If true, replace an existing property with this one else raise an error
     */
    void addPropertyWithUnit(Run & self, const std::string & name, const bpl::object & value, const std::string & units, bool replace)
    {
      extract<Property*> extractor(value);
      if(extractor.check())
      {
        Property *prop = extractor();
        self.addProperty(prop->clone(), replace); // Clone the property as Python owns the one that is passed in
        return;
      }

      // Use the factory
      auto property = Mantid::PythonInterface::Registry::PropertyWithValueFactory::create(name, value, Mantid::Kernel::Direction::Input);
      property->setUnits(units);
      self.addProperty(property, replace);
    }

  /**
   * Add a property with the given name and value
   * @param self A reference to the run object that we have been called on
   * @param name The name of the new property
   * @param value The value of the property
   * @param replace If true, replace an existing property with this one else raise an error
   */
  void addProperty(Run & self, const std::string & name, const bpl::object & value, bool replace)
  {
    addPropertyWithUnit(self, name, value, "", replace);
  }

  /**
   * Add a property with the given name and value. An existing property is overwritten if it exists
   * @param self A reference to the run object that we have been called on
   * @param name The name of the new property
   * @param value The value of the property
   */
  void addOrReplaceProperty(Run & self, const std::string & name, const bpl::object & value)
  {
    addProperty(self, name, value, true);
  }


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
   * Emulate dict.keys. Returns a list of property names know to the run object
   * @param self :: A reference to the Run object that called this method
   */
  bpl::list keys(Run & self)
  {
    const std::vector<Mantid::Kernel::Property*> & logs = self.getProperties();
    bpl::list names;
    for(auto iter = logs.begin(); iter != logs.end(); ++iter)
    {
      names.append((*iter)->name());
    }
    return names;
  }


}

void export_Run()
{
  //Pointer
  register_ptr_to_python<Run*>();

  //Run class
  class_< Run,  boost::noncopyable >("Run", no_init)
    .def("getProtonCharge", &Run::getProtonCharge, "Return the total good proton charge for the run")

    .def("integrateProtonCharge", &Run::integrateProtonCharge, "Return the total good proton charge for the run")

    .def("hasProperty", &Run::hasProperty, "Returns True if the given log value is contained within the run")

    .def("getProperty", &Run::getProperty, return_value_policy<return_by_value>(), "Returns the named property (log value). Use '.value' to return the value.")

    .def("getProperties", &Run::getProperties, return_internal_reference<>(), "Return the list of run properties managed by this object.")

    .def("getLogData", (Property* (Run::*)(const std::string &) const)&Run::getLogData, return_value_policy<return_by_value>(),
         "Returns the named log. Use '.value' to return the value. The same as getProperty.")

    .def("getLogData", (const std::vector<Property*>& (Run::*)() const)&Run::getLogData, return_internal_reference<>(), 
         "Return the list of logs for this run. The same as getProperties.")

    .def("getGoniometer", (const Mantid::Geometry::Goniometer & (Run::*)() const)&Run::getGoniometer,
         return_value_policy<reference_existing_object>(), "Get the oriented lattice for this sample")

    .def("addProperty", &addProperty, "Adds a property with the given name and value. If replace=True then an existing property is overwritten")

    .def("addProperty", &addPropertyWithUnit, "Adds a property with the given name, value and unit. If replace=True then an existing property is overwritten")

    .def("setStartAndEndTime", &Run::setStartAndEndTime, "Set the start and end time of the run")

    .def ("startTime", &Run::startTime, "Return the total starting time of the run.")

    .def ("endTime", &Run::endTime, "Return the total ending time of the run.")

    //--------------------------- Dictionary access----------------------------
    .def("get", &getWithDefault, "Returns the value pointed to by the key or None if it does not exist")
    .def("get", &get, "Returns the value pointed to by the key or the default value given")
    .def("keys", &keys, "Returns the names of the properties as list")
    .def("__contains__", &Run::hasProperty)
    .def("__getitem__", &Run::getProperty, return_value_policy<return_by_value>())
    .def("__setitem__", &addOrReplaceProperty)

   ;
}

