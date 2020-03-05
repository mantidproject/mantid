// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::ICompAssembly;
using Mantid::Geometry::ObjCompAssembly;
using Mantid::Geometry::ObjComponent;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(ObjCompAssembly)

void export_ObjCompAssembly() {
  register_ptr_to_python<boost::shared_ptr<ObjCompAssembly>>();

  class_<ObjCompAssembly, boost::python::bases<ICompAssembly, ObjComponent>,
         boost::noncopyable>("IObjCompAssembly", no_init);
}
