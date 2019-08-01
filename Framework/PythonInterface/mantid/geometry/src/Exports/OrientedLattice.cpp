// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidPythonInterface/core/Converters/MatrixToNDArray.h"
#include "MantidPythonInterface/core/Converters/PyObjectToMatrix.h"
#include "MantidPythonInterface/core/Converters/PyObjectToV3D.h"

#include "MantidPythonInterface/core/Policies/MatrixToNumpy.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::OrientedLattice;
using Mantid::Geometry::UnitCell;
using Mantid::Geometry::angDegrees;
using namespace boost::python;

namespace //<unnamed>
{
using namespace Mantid::PythonInterface;

/// Set the U vector via a numpy array
void setU(OrientedLattice &self, const object &data, const bool force) {
  self.setU(Converters::PyObjectToMatrix(data)(), force);
}

/// Set the U vector via a numpy array
void setUB(OrientedLattice &self, const object &data) {
  self.setUB(Converters::PyObjectToMatrix(data)());
}

/// Set the U matrix from 2 Python objects representing a V3D type. This can be
/// Set the U vector via a numpy array
void setModUB(OrientedLattice &self, const object &data) {
  self.setModUB(Converters::PyObjectToMatrix(data)());
}

/// Set the U matrix from 2 Python objects representing a V3D type. This can be
/// a V3D object, a list
/// or a numpy array. If the arrays are used they must be of length 3
void setUFromVectors(OrientedLattice &self, const object &vec1,
                     const object &vec2) {
  self.setUFromVectors(Converters::PyObjectToV3D(vec1)(),
                       Converters::PyObjectToV3D(vec2)());
}

Mantid::Kernel::V3D qFromHKL(OrientedLattice &self, const object &vec) {
  return self.qFromHKL(Converters::PyObjectToV3D(vec)());
}

Mantid::Kernel::V3D hklFromQ(OrientedLattice &self, const object &vec) {
  return self.hklFromQ(Converters::PyObjectToV3D(vec)());
}
} // namespace

void export_OrientedLattice() {
  /// return_value_policy for read-only numpy array
  using return_readonly_numpy =
      return_value_policy<Policies::MatrixRefToNumpy<Converters::WrapReadOnly>>;

  class_<OrientedLattice, bases<UnitCell>>(
      "OrientedLattice",
      init<>("Default constructor, with "
             ":math:`a=b=c=1 \\rm{\\AA}, \\alpha = \\beta = \\gamma = "
             "90^\\circ`. The :math:`U` matrix is set to the identity matrix."))
      .def(init<OrientedLattice const &>(
          (arg("other")),
          "Copy constructor for creating a new oriented lattice."))
      .def(init<double, double, double>(
          (arg("_a"), arg("_b"), arg("_c")),
          "Constructor using :math:`a, b, c` (in :math:`\\rm{\\AA}`), "
          ":math:`\\alpha=\\beta=\\gamma=90^\\circ`. The :math:`U` matrix is "
          "set to the identity matrix."))
      .def(init<double, double, double, double, double, double, optional<int>>(
          (arg("_a"), arg("_b"), arg("_c"), arg("_alpha"), arg("_beta"),
           arg("_gamma"), arg("Unit") = static_cast<int>(angDegrees)),
          "Constructor using :math:`a, b, c` (in :math:`\\rm{\\AA}`), "
          ":math:`\\alpha, \\beta, "
          "\\gamma` (in degrees or radians). The optional parameter ``Unit`` "
          "controls the "
          "units for the angles, and can have the value of ``Degrees`` or "
          "``Radians``. By default ``Unit`` = ``Degrees``."))
      .def(init<UnitCell>(
          arg("uc"), "Constructor from a :class:`~mantid.geometry.UnitCell`. "
                     "The :math:`U` matrix is set to the identity matrix."))
      .def("getuVector", (&OrientedLattice::getuVector), arg("self"),
           "Returns the vector along the beam direction when "
           ":class:`~mantid.geometry.Goniometer` s are at 0. ")
      .def("getvVector", (&OrientedLattice::getvVector), arg("self"),
           "Returns the vector along the horizontal plane, perpendicular to "
           "the beam direction when :class:`~mantid.geometry.Goniometer` s are "
           "at 0. ")
      .def("getU", &OrientedLattice::getU, arg("self"), return_readonly_numpy(),
           "Returns the :math:`U` rotation matrix. This will return a "
           ":class:`numpy.ndarray` with shape ``(3,3)``.")
      .def("setU", &setU, (arg("self"), arg("newU"), arg("force") = true),
           "Set the :math:`U` rotation matrix. This method expects a "
           ":class:`numpy.ndarray` with shape ``(3,3)``.")
      .def("getUB", &OrientedLattice::getUB, arg("self"),
           return_readonly_numpy(),
           "Returns the :math:`UB` matrix for this oriented lattice. This will "
           "return "
           "a :class:`numpy.ndarray` with shape ``(3,3)``.")
      .def("setUB", &setUB, (arg("self"), arg("newUB")),
           "Set the :math:`UB` matrix. This methiod will calculate first the "
           "lattice parameters, then the :math:`B` matrix, and then :math:`U`. "
           "This method expects a "
           ":class:`numpy.ndarray` with shape ``(3,3)``. ")
      .def("getModUB", &OrientedLattice::getModUB, arg("self"),
           return_readonly_numpy(),
           "Returns the :math:`ModUB` matrix for this oriented lattice. This "
           "will "
           "return "
           "a :class:`numpy.ndarray` with shape ``(3,3)``.")
      .def(
          "setModUB", &setModUB, (arg("self"), arg("newModUB")),
          "Set the :math:`ModUB` matrix. This methiod will calculate first the "
          "lattice parameters, then the :math:`B` matrix, and then :math:`U`. "
          "This method expects a "
          ":class:`numpy.ndarray` with shape ``(3,3)``. ")
      .def("setUFromVectors", &setUFromVectors,
           (arg("self"), arg("u"), arg("v")),
           "Set the :math:`U` rotation matrix using two vectors to define a "
           "new "
           "coordinate system. This method with return the new :math:`U` "
           "matrix "
           "as a :class:`numpy.ndarray` with shape ``(3,3)``. ")
      .def("qFromHKL", &qFromHKL, (arg("self"), arg("vec")),
           ":math:`Q` vector from :math:`HKL` vector")
      .def("hklFromQ", &hklFromQ, (arg("self"), arg("vec")),
           ":math:`HKL` value from :math:`Q` vector");
}
