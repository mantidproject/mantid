// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/ObjComponent.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::Component;
using Mantid::Geometry::IObjComponent;
using Mantid::Geometry::ObjComponent;
using namespace boost::python;

void export_ObjComponent() {
  class_<ObjComponent, boost::python::bases<IObjComponent, Component>, boost::noncopyable>("ObjComponent", no_init);
}
