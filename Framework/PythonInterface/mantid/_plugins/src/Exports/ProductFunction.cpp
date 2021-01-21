// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/ProductFunction.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using Mantid::API::CompositeFunction;
using Mantid::CurveFitting::Functions::ProductFunction;
using namespace boost::python;

void export_ProductFunction() {

  class_<ProductFunction, bases<CompositeFunction>, boost::noncopyable>("ProductFunction", "Composite Fit functions");
}
