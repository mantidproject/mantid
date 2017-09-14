#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidAPI/CompositeFunction.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::CompositeFunction;
using Mantid::API::IFunction;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(CompositeFunction)

namespace {

typedef double (CompositeFunction::*getParameterType1)(size_t) const;
typedef double (CompositeFunction::*getParameterType2)(
    const std::string &) const;

typedef void (CompositeFunction::*setParameterType2)(const std::string &,
                                                     const double &, bool);
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(setParameterType2_Overloads,
                                       setParameter, 2, 3)
#ifdef __clang__
#pragma clang diagnostic pop
#endif
} // namespace

void export_CompositeFunction() {

  register_ptr_to_python<boost::shared_ptr<CompositeFunction>>();

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
      .def("__setitem__", &CompositeFunction::replaceFunction,
           (arg("self"), arg("i"), arg("f")),
           "Put function in place of the i-th function.")
      .def("add", &CompositeFunction::addFunction,
           (arg("self"), arg("function")), "Add a member function.")
      .def("getParameterValue",
           (getParameterType1)&CompositeFunction::getParameter,
           (arg("self"), arg("i")), "Get value of parameter of given index.")
      .def("getParameterValue",
           (getParameterType2)&CompositeFunction::getParameter,
           (arg("self"), arg("name")), "Get value of parameter of given name.")
      .def("__getitem__", (getParameterType2)&CompositeFunction::getParameter,
           (arg("self"), arg("name")), "Get value of parameter of given name.")
      .def("__setitem__", (setParameterType2)&CompositeFunction::setParameter,
           setParameterType2_Overloads(
               (arg("self"), arg("name"), arg("value"), arg("explicitlySet")),
               "Get value of parameter of given name."))
      .def("__delitem__", &CompositeFunction::removeFunction,
           (arg("self"), arg("index")));
}
