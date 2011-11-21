#include "MantidPythonInterface/api/AlgorithmWrapper.h"
#include "MantidPythonInterface/kernel/PropertyMarshal.h"
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidKernel/Strings.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::IPropertyManager;
using Mantid::Kernel::Property;

using Mantid::API::IAlgorithm;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::Algorithm;
using Mantid::API::AlgorithmProxy;
using Mantid::PythonInterface::AlgorithmWrapper;

using namespace boost::python;


namespace
{
  /***********************************************************************
   *
   * Useful functions within Python to have on the algorithm interface
   *
   ***********************************************************************/

    ///Functor for use with std::sort to put the properties that do not
    ///have valid values first
    struct MandatoryFirst
    {
      ///Comparator operator for sort algorithm, places optional properties lower in the list
      bool operator()(const Property * p1, const Property * p2) const
      {
        //this is false, unless p1 is not valid and p2 is valid
        return ( p1->isValid() != "" ) && ( p2->isValid() == "" );
      }
    };

  //----------------------- Property order ------------------------------------
    /**
     * Returns a list of property names that is ordered such that the
     * mandatory properties are first followed by the optional ones.
     * @param self :: A pointer to the python object wrapping and Algorithm.
     * @return A Python list of strings
     */
    PyObject * getPropertyOrder(boost::python::object self)
    {
      IAlgorithm_sptr algm = boost::python::extract<IAlgorithm_sptr>(self);
      std::vector<Property*> properties(algm->getProperties());
      std::sort(properties.begin(), properties.end(), MandatoryFirst());
      const size_t numProps(properties.size());

      // Build the list
      PyObject *names = PyList_New(numProps);
      for ( size_t i = 0; i < numProps; ++i )
      {
        std::string name = properties[i]->name();
        PyList_SetItem(names, i, PyString_FromString(name.c_str()));
      }
      return names;
    }



  // ---------------------- Documentation -------------------------------------
  /**
   * Create a doc string for the simple API
   * @param self :: A pointer to the python object wrapping and Algorithm
   * @return A string that documents an algorithm
   */
  std::string createDocString(boost::python::object self)
  {
    IAlgorithm_sptr algm = boost::python::extract<IAlgorithm_sptr>(self);
    const std::string EOL="\n";

    // Put in the quick overview message
    std::stringstream buffer;
    std::string temp = algm->getOptionalMessage();
    if (temp.size() > 0)
      buffer << temp << EOL << EOL;

    // get a sorted copy of the properties
    std::vector<Property*> properties(algm->getProperties());
    std::sort(properties.begin(), properties.end(), MandatoryFirst());
    const size_t numProps(properties.size());

    buffer << "Property descriptions: " << EOL << EOL;
    // write the actual property descriptions
    for ( size_t i = 0; i < numProps; ++i)
    {
      Mantid::Kernel::Property *prop = properties[i];
      buffer << prop->name() << "("
             << Mantid::Kernel::Direction::asText(prop->direction());
      if (!prop->isValid().empty())
        buffer << ":req";
      buffer << ") *" << prop->type() << "* ";
      std::set<std::string> allowed = prop->allowedValues();
      if (!prop->documentation().empty() || !allowed.empty())
      {
        buffer << "      " << prop->documentation();
        if (!allowed.empty())
        {
          buffer << " [" << Mantid::Kernel::Strings::join(allowed.begin(), allowed.end(), ", ");
          buffer << "]";
        }
        buffer << EOL;
        if( i < numProps - 1 ) buffer << EOL;
      }
    }

    return buffer.str();
  }

}


void export_algorithm()
{
  register_ptr_to_python<IAlgorithm_sptr>();
  class_<IAlgorithm, bases<IPropertyManager>, boost::noncopyable>("IAlgorithm", "Interface for all algorithms", no_init)
    .def("name", &IAlgorithm::name, "Returns the name of the algorithm")
    .def("alias", &IAlgorithm::alias, "Return the aliases for the algorithm")
    .def("version", &IAlgorithm::version, "Returns the version number of the algorithm")
    .def("category", &IAlgorithm::category, "Returns the category containing the algorithm")
    .def("create_doc_string", &createDocString, "Returns a doc string for the algorithm")
    .def("get_property_order",&getPropertyOrder, "Returns a list of property names that is ordered "
          "such that the mandatory properties are first followed by the optional ones.")
    .def("initialize", &IAlgorithm::initialize, "Initializes the algorithm")
    .def("is_initialized", &IAlgorithm::isInitialized, "Returns True if the algorithm is initialized, False otherwise")
    .def("execute", &IAlgorithm::execute, "Runs the algorithm")
    .def("set_child", &IAlgorithm::setChild,
        "If true this algorithm is run as a child algorithm. There will be no logging and nothing is stored in the Analysis Data Service")
    .def("set_logging", &IAlgorithm::setLogging, "Toggle logging on/off.")
    ;
}

void export_algorithmHierarchy()
{
  register_ptr_to_python<boost::shared_ptr<AlgorithmProxy> >();
  class_<AlgorithmProxy, bases<IAlgorithm>, boost::noncopyable>("AlgorithmProxy", "Proxy class returned by managed algorithms", no_init)
    ;

  register_ptr_to_python<boost::shared_ptr<Algorithm> >();
  class_<AlgorithmWrapper, bases<IAlgorithm>, boost::noncopyable>("Algorithm", "Base class for all algorithms")
    ;
}

// Clean up namespace
#undef EXPORT_PROPERTY_ACCESSORS
