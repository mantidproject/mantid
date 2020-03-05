// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/NullValidator.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::IValidator;
using Mantid::Kernel::NullValidator;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(NullValidator)

void export_NullValidator() {
  register_ptr_to_python<boost::shared_ptr<NullValidator>>();

  class_<NullValidator, bases<IValidator>, boost::noncopyable>("NullValidator");
}
