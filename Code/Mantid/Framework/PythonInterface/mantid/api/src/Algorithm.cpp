#include "MantidPythonInterface/api/AlgorithmWrapper.h"
#include "MantidAPI/AlgorithmProxy.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::IAlgorithm;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::Algorithm;
using Mantid::API::AlgorithmProxy;
using Mantid::PythonInterface::AlgorithmWrapper;

using boost::python::class_;
using boost::python::register_ptr_to_python;
using boost::python::bases;
using boost::python::no_init;

void export_algorithm()
{
  // Allow shared_ptrs to be treated as if they were objects
  register_ptr_to_python<IAlgorithm_sptr>();

  class_<IAlgorithm, boost::noncopyable>("IAlgorithm", "Interface for all algorithms", no_init)
    .def("name", &IAlgorithm::name, "Returns the name of the algorithm")
    .def("version", &IAlgorithm::version, "Returns the version number of the algorithm")
    .def("category", &IAlgorithm::category, "Returns the category containing the algorithm")
    .def("initialize", &IAlgorithm::initialize, "Initializes the algorithm")
    .def("execute", &IAlgorithm::execute, "Runs the algorithm")
    ;

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
