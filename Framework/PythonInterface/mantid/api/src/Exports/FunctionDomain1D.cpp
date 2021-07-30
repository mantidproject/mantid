// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionDomain.h"

#include <boost/python/class.hpp>
#include <boost/python/module.hpp>

using Mantid::API::FunctionDomain;
using Mantid::API::FunctionDomain1D;
using namespace boost::python;

void export_FunctionDomain1D() {
  class_<FunctionDomain1D, bases<FunctionDomain>, boost::noncopyable>("FunctionDomain1D", no_init);
}