#include "MantidCurveFitting/Functions/ProductFunction.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using Mantid::CurveFitting::Functions::ProductFunction;
using Mantid::API::CompositeFunction;
using namespace boost::python;

namespace {

typedef double (ProductFunction::*getParameterType1)(size_t) const;
typedef double (ProductFunction::*getParameterType2)(const std::string &) const;

typedef void (ProductFunction::*setParameterType2)(const std::string &,
                                                   const double &, bool);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(setParameterType2_Overloads,
                                       setParameter, 2, 3)
}

void export_ProductFunction() {

  class_<ProductFunction, bases<CompositeFunction>, boost::noncopyable>(
      "ProductFunction", "Composite Fit functions");
}
