// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/RegionSelectorObserver.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/WeakPtr.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

GET_POINTER_SPECIALIZATION(RegionSelectorObserver)

using namespace boost::python;
using Mantid::API::RegionSelectorObserver;

void export_RegionSelectorObserver() {

  register_ptr_to_python<std::shared_ptr<RegionSelectorObserver>>();
  register_ptr_to_python<std::weak_ptr<RegionSelectorObserver>>();

  class_<RegionSelectorObserver, boost::noncopyable>("RegionSelectorObserver", no_init)
      .def("notifyRegionChanged", &RegionSelectorObserver::notifyRegionChanged, arg("self"),
           "Notification that the region selection has changed");
}
