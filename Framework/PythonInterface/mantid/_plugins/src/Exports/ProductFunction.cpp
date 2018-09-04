#include "MantidCurveFitting/Functions/ProductFunction.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using Mantid::API::CompositeFunction;
using Mantid::CurveFitting::Functions::ProductFunction;
using namespace boost::python;

void export_ProductFunction() {

  class_<ProductFunction, bases<CompositeFunction>, boost::noncopyable>(
      "ProductFunction", "Composite Fit functions");
}
