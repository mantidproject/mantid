#include "MantidCurveFitting/Functions/ProductFunction.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using Mantid::CurveFitting::Functions::ProductFunction;
using Mantid::API::CompositeFunction;
using namespace boost::python;

void export_ProductFunction() {

  class_<ProductFunction, bases<CompositeFunction>, boost::noncopyable>(
      "ProductFunction", "Composite Fit functions");
}
