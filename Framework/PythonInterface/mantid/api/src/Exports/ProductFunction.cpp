#include "MantidAPI/ProductFunction.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using Mantid::API::IFunction;
using Mantid::API::ProductFunction;
using namespace boost::python;

namespace {

using getParameterType1 = double (ProductFunction::*)(size_t) const;
using getParameterType2 =
    double (ProductFunction::*)(const std::string &) const;

using setParameterType2 = void (ProductFunction::*)(const std::string &,
                                                    const double &, bool);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(setParameterType2_Overloads,
                                       setParameter, 2, 3)
} // namespace

void export_ProductFunction() {

  class_<ProductFunction, bases<IFunction>, boost::noncopyable>(
      "ProductFunction", "Composite Fit functions")
      .def("nFunctions", &ProductFunction::nFunctions, arg("self"),
           "Get the number of member functions.")
      .def("__len__", &ProductFunction::nFunctions, arg("self"),
           "Get the number of member functions.")
      .def("getFunction", &ProductFunction::getFunction,
           (arg("self"), arg("i")), "Get the i-th function.")
      .def("__getitem__", &ProductFunction::getFunction,
           (arg("self"), arg("i")), "Get the i-th function.")
      .def("add", &ProductFunction::addFunction, (arg("self"), arg("function")),
           "Add a member function.")
      .def("getParameterValue",
           (getParameterType1)&ProductFunction::getParameter,
           (arg("self"), arg("i")), "Get value of parameter of given index.")
      .def("getParameterValue",
           (getParameterType2)&ProductFunction::getParameter,
           (arg("self"), arg("name")), "Get value of parameter of given name.")
      .def("__getitem__", (getParameterType2)&ProductFunction::getParameter,
           (arg("self"), arg("name")), "Get value of parameter of given name.")
      .def("__setitem__", (setParameterType2)&ProductFunction::setParameter,
           setParameterType2_Overloads(
               (arg("self"), arg("name"), arg("value"), arg("explicitlySet")),
               "Get value of parameter of given name."))
      .def("__delitem__", &ProductFunction::removeFunction,
           (arg("self"), arg("index")));
}
