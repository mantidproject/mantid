// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifdef _MSC_VER
#pragma warning(disable : 4250) // Disable warning regarding inheritance via
                                // dominance, we have no way around it with the
                                // design
#endif
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/PropertyManagerFactory.h"

#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>

using Mantid::Kernel::IPropertyManager;
using Mantid::Kernel::PropertyManager;
using Mantid::Kernel::PropertyManager_sptr;
using Mantid::PythonInterface::Registry::createPropertyManager;

using namespace boost::python;

GET_POINTER_SPECIALIZATION(PropertyManager)

void export_PropertyManager() {

  // The second argument defines the actual type held within the Python object.
  // This means that when a PropertyManager is constructed in Python
  // it actually used a shared_ptr to the object rather than a raw pointer.
  // This knowledge is used by DataServiceExporter::extractCppValue to assume
  // that it can always extract a shared_ptr type
  class_<PropertyManager, PropertyManager_sptr, bases<IPropertyManager>,
         boost::noncopyable>("PropertyManager")
      .def("__init__", make_constructor(&createPropertyManager));
}

#ifdef _MSC_VER
#pragma warning(default : 4250)
#endif
