// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/LambdaValidator.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::LambdaValidator;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(LambdaValidator)

void export_LambdaValidator() {
  register_ptr_to_python<std::shared_ptr<IValidator>>();

  class_<IValidator, boost::noncopyable>("IValidator", no_init);
}
