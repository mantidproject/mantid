// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/Sample.h"

#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Material.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::Sample;
using Mantid::Geometry::OrientedLattice;
using Mantid::Kernel::Material; // NOLINT
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Material)
GET_POINTER_SPECIALIZATION(OrientedLattice)
GET_POINTER_SPECIALIZATION(Sample)

void export_Sample() {
  register_ptr_to_python<Sample *>();
  register_ptr_to_python<boost::shared_ptr<Sample>>();

  class_<Sample, boost::noncopyable>("Sample", no_init)
      .def("getName", &Sample::getName,
           return_value_policy<copy_const_reference>(), arg("self"),
           "Returns the string name of the sample")
      .def("getOrientedLattice",
           (const OrientedLattice &(Sample::*)() const) &
               Sample::getOrientedLattice,
           arg("self"), return_value_policy<reference_existing_object>(),
           "Get the oriented lattice for this sample")
      .def("hasOrientedLattice", &Sample::hasOrientedLattice, arg("self"),
           "Returns True if this sample has an oriented lattice, false "
           "otherwise")
      .def("getCrystalStructure", &Sample::getCrystalStructure, arg("self"),
           return_value_policy<reference_existing_object>(),
           "Get the crystal structure for this sample")
      .def("hasCrystalStructure", &Sample::hasCrystalStructure, arg("self"),
           "Returns True if this sample has a crystal structure, false "
           "otherwise")
      .def("setCrystalStructure", &Sample::setCrystalStructure,
           (arg("self"), arg("newCrystalStructure")),
           "Assign a crystal structure object to the sample.")
      .def("clearCrystalStructure", &Sample::clearCrystalStructure, arg("self"),
           "Removes the internally stored crystal structure.")
      .def("size", &Sample::size, arg("self"),
           "Return the number of samples contained within this sample")
      // Required for ISIS SANS reduction until the full sample geometry is
      // defined on loading
      .def("getGeometryFlag", &Sample::getGeometryFlag, arg("self"),
           "Return the geometry flag.")
      .def("getThickness", &Sample::getThickness, arg("self"),
           "Return the thickness in mm")
      .def("getHeight", &Sample::getHeight, arg("self"),
           "Return the height in mm")
      .def("getWidth", &Sample::getWidth, arg("self"), "Return the width in mm")
      .def("getMaterial", &Sample::getMaterial, arg("self"),
           "The material the sample is composed of",
           return_value_policy<reference_existing_object>())
      .def("setGeometryFlag", &Sample::setGeometryFlag,
           (arg("self"), arg("geom_id")), "Set the geometry flag.")
      .def("setThickness", &Sample::setThickness, (arg("self"), arg("thick")),
           "Set the thickness in mm.")
      .def("setHeight", &Sample::setHeight, (arg("self"), arg("height")),
           "Set the height in mm.")
      .def("setWidth", &Sample::setWidth, (arg("self"), arg("width")),
           "Set the width in mm.")
      .def("getShape", &Sample::getShape, arg("self"),
           "Returns a shape of a Sample object.",
           return_value_policy<reference_existing_object>())
      // -------------------------Operators
      // -------------------------------------
      .def("__len__", &Sample::size, arg("self"),
           "Gets the number of samples in this collection")
      .def("__getitem__", &Sample::operator[],(arg("self"), arg("index")),
           return_internal_reference<>());
}
