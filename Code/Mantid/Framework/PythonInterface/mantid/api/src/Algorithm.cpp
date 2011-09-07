#include "MantidPythonInterface/api/AlgorithmWrapper.h"
#include "MantidPythonInterface/kernel/PropertyMarshal.h"
#include "MantidAPI/AlgorithmProxy.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_internal_reference.hpp>

using Mantid::Kernel::IPropertyManager;

using Mantid::API::IAlgorithm;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::Algorithm;
using Mantid::API::AlgorithmProxy;
using Mantid::PythonInterface::AlgorithmWrapper;
using Mantid::PythonInterface::PropertyMarshal;

using namespace boost::python;

void export_algorithm()
{
  register_ptr_to_python<IAlgorithm_sptr>();
  class_<IAlgorithm, bases<IPropertyManager>, boost::noncopyable>("IAlgorithm", "Interface for all algorithms", no_init)
    .def("name", &IAlgorithm::name, "Returns the name of the algorithm")
    .def("version", &IAlgorithm::version, "Returns the version number of the algorithm")
    .def("category", &IAlgorithm::category, "Returns the category containing the algorithm")
    .def("initialize", &IAlgorithm::initialize, "Initializes the algorithm")
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
  class_<Algorithm, bases<IAlgorithm>, boost::noncopyable>("Algorithm", "Base for all algorithms", no_init)
    ;

  register_ptr_to_python<boost::shared_ptr<AlgorithmWrapper> >();
  // We change the name because the boost::python framework makes AlgorithmWrapper appear as it is a PythonAlgorithm
  class_<AlgorithmWrapper, bases<Algorithm>, boost::noncopyable>("PythonAlgorithm", "Base for all Python algorithms")
    ;
}

// Clean up namespace
#undef EXPORT_PROPERTY_ACCESSORS
