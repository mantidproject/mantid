// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::detid_t;
using Mantid::DataObjects::SpecialWorkspace2D;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(SpecialWorkspace2D)

void export_SpecialWorkspace2D {
    register_ptr_to_python<SpecialWorkspace2D *>();

    class_<SpecialWorkspace2D, boost::noncopyable("SpecialWorkspace2D", no_init)
        .def("setValue", &SpecialWorkspace2D::setValue,
            (arg("self"), arg("detectorID"), arg("value"), arg("error")))
        .def("getDetectorIDs",
            (const std::set<detid_t> &(SpecialWorkspace2D::*)() const) &
               SpecialWorkspace2D::getDetectorIDs,
            (arg("self,"), arg("workspaceIndex")))
}