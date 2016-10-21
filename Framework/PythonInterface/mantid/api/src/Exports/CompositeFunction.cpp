#include "MantidAPI/CompositeFunction.h"
#include <boost/python/class.hpp>

using Mantid::API::CompositeFunction;
using Mantid::API::IFunction;
using namespace boost::python;

void export_CompositeFunction() {

  class_<CompositeFunction, bases<IFunction>, boost::noncopyable>(
      "CompositeFunction", "Composite Fit functions")
      .def("nFunctions", &CompositeFunction::nFunctions, arg("self"),
           "Get the number of member functions.")
      .def("__len__", &CompositeFunction::nFunctions, arg("self"),
           "Get the number of member functions.")
      .def("getFunction", &CompositeFunction::getFunction,
           (arg("self"), arg("i")), "Get the i-th function.")
      .def("__getitem__", &CompositeFunction::getFunction,
           (arg("self"), arg("i")), "Get the i-th function.")
      .def("add", &CompositeFunction::addFunction,
           (arg("self"), arg("function")), "Add a member function.");
}
