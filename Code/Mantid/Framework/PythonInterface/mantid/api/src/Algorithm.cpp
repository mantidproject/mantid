#include "MantidPythonInterface/api/AlgorithmWrapper.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace boost::python;
using Mantid::API::IAlgorithm;
using Mantid::API::Algorithm;

void export_algorithm()
{
  // Allow shared_ptrs to be treated as if they were objects
  register_ptr_to_python<boost::shared_ptr<IAlgorithm> >();

  class_<IAlgorithm, boost::noncopyable>("IAlgorithm", "Interface for all algorithms", boost::python::no_init)
    .def("name", &IAlgorithm::name, "Returns the name of the algorithm")
    .def("version", &IAlgorithm::version, "Returns the version number of the algorithm")
    .def("category", &IAlgorithm::category, "Returns the category containing the algorithm")
    .def("initialize", &IAlgorithm::initialize, "Initializes the algorithm")
    .def("execute", &IAlgorithm::execute, "Runs the algorithm")
    ;

  class_<Mantid::PythonInterface::AlgorithmWrapper, bases<IAlgorithm>, boost::noncopyable>("Algorithm")
    ;
}
