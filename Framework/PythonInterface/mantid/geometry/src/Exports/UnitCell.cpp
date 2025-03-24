// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidPythonInterface/core/Converters/MatrixToNDArray.h"
#include "MantidPythonInterface/core/Converters/PyObjectToMatrix.h"
#include "MantidPythonInterface/core/Converters/PyObjectToV3D.h"
#include "MantidPythonInterface/core/Policies/MatrixToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/self.hpp>

using Mantid::Geometry::angDegrees;
using Mantid::Geometry::AngleUnits;
using Mantid::Geometry::angRadians;
using Mantid::Geometry::UnitCell;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::V3D;
using namespace boost::python;

// Functions purely to aid with wrapping
namespace //<unnamed>
{
using namespace Mantid::PythonInterface;

/// Pass-through function to set the unit cell from a 2D numpy array
void recalculateFromGstar(UnitCell &self, const object &values) {
  // Create a double matrix and put this in to the unit cell
  self.recalculateFromGstar(Converters::PyObjectToMatrix(values)());
}

/// Export for python's special __str__ method
std::string __str__implementation(const UnitCell &self) {
  std::stringstream ss;
  ss << "UnitCell with lattice parameters:";
  ss << " a = " << self.a();
  ss << " b = " << self.b();
  ss << " c = " << self.c();

  ss << " alpha = " << self.alpha();
  ss << " beta = " << self.beta();
  ss << " gamma = " << self.gamma();

  return ss.str();
}

/// Export for python's special __repr__ method
std::string __repr__implementation(const UnitCell &self) {
  std::stringstream ss;
  ss << "UnitCell(";
  ss << self.a() << ", ";
  ss << self.b() << ", ";
  ss << self.c() << ", ";

  ss << self.alpha() << ", ";
  ss << self.beta() << ", ";
  ss << self.gamma();
  ss << ")";

  return ss.str();
}

} // namespace

void export_UnitCell() {
  enum_<AngleUnits>("AngleUnits").value("Degrees", angDegrees).value("Radians", angRadians).export_values();

  /// return_value_policy for read-only numpy array
  using return_readonly_numpy = return_value_policy<Policies::MatrixRefToNumpy<Converters::WrapReadOnly>>;

  class_<UnitCell>("UnitCell", init<>("Default constructor, with "
                                      ":math:`a=b=c=1 \\rm{\\AA}, \\alpha = \\beta = \\gamma = 90^\\circ`"))
      .def(init<UnitCell const &>((arg("self"), arg("other")), "Copy constructor for creating a new unit cell."))
      .def(init<double, double, double>((arg("self"), arg("_a"), arg("_b"), arg("_c")),
                                        "Constructor using :math:`a, b, c` (in :math:`\\rm{\\AA}`), "
                                        ":math:`\\alpha=\\beta=\\gamma=90^\\circ`"))
      .def(init<double, double, double, double, double, double, optional<int>>(
          (arg("self"), arg("_a"), arg("_b"), arg("_c"), arg("_alpha"), arg("_beta"), arg("_gamma"),
           arg("Unit") = static_cast<int>(angDegrees)),
          "Constructor using "
          ":math:`a, b, c` (in :math:`\\rm{\\AA}`), :math:`\\alpha, \\beta, "
          "\\gamma` (in "
          "degrees or radians). The optional parameter ``Unit`` controls the "
          "units for the angles, and can have the value of ``Degrees`` or "
          "``Radians``. By default ``Unit`` = ``Degrees``."))
      .def("a", (double(UnitCell::*)() const) & UnitCell::a, arg("self"),
           "Returns the length of the :math:`a` direction of the unit cell in "
           ":math:`\\rm{\\AA}`.")
      .def("a1", (double(UnitCell::*)() const) & UnitCell::a1, arg("self"),
           "Returns the length of the :math:`a_{1} = a` direction of the unit "
           "cell. "
           "This is an alias for :func:`~mantid.geometry.UnitCell.a`. ")
      .def("a2", (double(UnitCell::*)() const) & UnitCell::a2, arg("self"),
           "Returns the length of the :math:`a_{2} = b` direction of the unit "
           "cell. "
           "This is an alias for :func:`~mantid.geometry.UnitCell.b`. ")
      .def("a3", (double(UnitCell::*)() const) & UnitCell::a3, arg("self"),
           "Returns the length of the :math:`a_{2} = c` direction of the unit "
           "cell. "
           "This is an alias for :func:`~mantid.geometry.UnitCell.c`. ")
      .def("alpha", (double(UnitCell::*)() const) & UnitCell::alpha, arg("self"),
           "Returns the :math:`\\alpha` angle for this unit cell in degrees.")
      .def("alpha1", (double(UnitCell::*)() const) & UnitCell::alpha1, arg("self"),
           "Returns the :math:`\\alpha_{1} = \\alpha` angle of the unit cell "
           "in radians. "
           "See also :func:`~mantid.geometry.UnitCell.alpha`. ")
      .def("alpha2", (double(UnitCell::*)() const) & UnitCell::alpha2, arg("self"),
           "Returns the :math:`\\alpha_{2} = \\beta` angle of the unit cell in "
           "radians. "
           "See also :func:`~mantid.geometry.UnitCell.beta`. ")
      .def("alpha3", (double(UnitCell::*)() const) & UnitCell::alpha3, arg("self"),
           "Returns the :math:`\\alpha_{3} = \\gamma` angle of the unit cell "
           "in radians. "
           "See also :func:`~mantid.geometry.UnitCell.gamma`. ")
      .def("alphastar", (double(UnitCell::*)() const) & UnitCell::alphastar, arg("self"),
           "Returns the reciprocal :math:`\\alpha` angle for this unit cell in "
           "degrees.")
      .def("astar", (double(UnitCell::*)() const) & UnitCell::astar, arg("self"),
           "Returns the length of the reciprocal :math:`a` direction for this "
           "unit cell in reciprocal :math:`\\rm{\\AA}`.")
      .def("b", (double(UnitCell::*)() const) & UnitCell::b, arg("self"),
           "Returns the length of the :math:`b` direction of the unit cell in "
           ":math:`\\rm{\\AA}`.")
      .def("b1", (double(UnitCell::*)() const) & UnitCell::b1, arg("self"),
           "Returns the length of the :math:`b_{1} = a^{*}` direction of the "
           "unit "
           "cell. This is an alias for "
           ":func:`~mantid.geometry.UnitCell.astar`. ")
      .def("b2", (double(UnitCell::*)() const) & UnitCell::b2, arg("self"),
           "Returns the length of the :math:`b_{2} = b^{*}` direction of the "
           "unit "
           "cell. This is an alias for "
           ":func:`~mantid.geometry.UnitCell.bstar`. ")
      .def("b3", (double(UnitCell::*)() const) & UnitCell::b3, arg("self"),
           "Returns the length of the :math:`b_{3} = c^{*}` direction of the "
           "unit "
           "cell. This is an alias for "
           ":func:`~mantid.geometry.UnitCell.cstar`. ")
      .def("beta", (double(UnitCell::*)() const) & UnitCell::beta, arg("self"),
           "Returns the :math:`\\beta` angle for this unit cell in degrees.")
      .def("beta1", (double(UnitCell::*)() const) & UnitCell::beta1, arg("self"),
           "Returns the :math:`\\beta_{1} = \\alpha^{*}` angle of the unit "
           "cell in "
           "radians. See also :func:`~mantid.geometry.UnitCell.alphastar`. ")
      .def("beta2", (double(UnitCell::*)() const) & UnitCell::beta2, arg("self"),
           "Returns the :math:`\\beta_{2} = \\beta^{*}` angle of the unit cell "
           "in radians. "
           "See also :func:`~mantid.geometry.UnitCell.betastar`. ")
      .def("beta3", (double(UnitCell::*)() const) & UnitCell::beta3, arg("self"),
           "Returns the :math:`\\beta_{3} = \\gamma^{*}` angle of the unit "
           "cell in "
           "radians. See also :func:`~mantid.geometry.UnitCell.gammastar`. ")
      .def("betastar", (double(UnitCell::*)() const) & UnitCell::betastar, arg("self"),
           "Returns the :math:`\\beta^{*}` angle for this unit cell in "
           "degrees.")
      .def("bstar", (double(UnitCell::*)() const) & UnitCell::bstar, arg("self"),
           "Returns the length of the :math:`b^{*}` direction for this "
           "unit cell in reciprocal :math:`\\rm{\\AA}`.")
      .def("c", (double(UnitCell::*)() const) & UnitCell::c, arg("self"),
           "Returns the length of the :math:`c` direction of the unit cell in "
           ":math:`\\rm{\\AA}`.")
      .def("cstar", (double(UnitCell::*)() const) & UnitCell::cstar, arg("self"),
           "Returns the length of the :math:`c^{*}` direction for this "
           "unit cell in reciprocal :math:`\\rm{\\AA}`.")
      .def("d", (double(UnitCell::*)(double, double, double) const) & UnitCell::d,
           (arg("self"), arg("h"), arg("k"), arg("l")),
           "Returns :math:`d`-spacing for a given H, K, L coordinate in "
           ":math:`\\rm{\\AA}`.")
      // cppcheck-suppress cstyleCast
      .def("d", (double(UnitCell::*)(const V3D &) const) & UnitCell::d, (arg("self"), arg("hkl")),
           "Returns :math:`d`-spacing for a given H, K, L coordinate in "
           ":math:`\\rm{\\AA}`.")
      .def("dstar", (double(UnitCell::*)(double, double, double) const) & UnitCell::dstar,
           (arg("self"), arg("h"), arg("k"), arg("l")),
           "Returns :math:`d^{*} = 1/d` for a given H, K, L coordinate in "
           ":math:`\\rm{\\AA}^{3}`.")
      .def("errora", (double(UnitCell::*)() const) & UnitCell::errora, arg("self"),
           "Returns the error in the :math:`a` unit cell length.")
      .def("errorb", (double(UnitCell::*)() const) & UnitCell::errorb, arg("self"),
           "Returns the error in the :math:`b` unit cell length.")
      .def("errorc", (double(UnitCell::*)() const) & UnitCell::errorc, arg("self"),
           "Returns the error in the :math:`c` unit cell length.")
      .def("erroralpha", (double(UnitCell::*)(int const) const) & UnitCell::erroralpha,
           (arg("self"), arg("Unit") = static_cast<int>(angDegrees)),
           "Returns the error in the :math:`\\alpha` angle of the unit cell.")
      .def("errorbeta", (double(UnitCell::*)(int const) const) & UnitCell::errorbeta,
           (arg("self"), arg("Unit") = static_cast<int>(angDegrees)),
           "Returns the error in :math:`\\beta` angle of the unit cell.")
      .def("errorgamma", (double(UnitCell::*)(int const) const) & UnitCell::errorgamma,
           (arg("self"), arg("Unit") = static_cast<int>(angDegrees)),
           "Returns the error in :math:`\\gamma` angle of the unit cell.")
      .def("gamma", (double(UnitCell::*)() const) & UnitCell::gamma, arg("self"),
           "Returns the :math:`\\gamma` angle for this unit cell in degrees.")
      .def("gammastar", (double(UnitCell::*)() const) & UnitCell::gammastar, arg("self"),
           "Returns the :math:`\\gamma^{*}` angle for this unit cell in "
           "degrees.")
      .def("recAngle",
           (double(UnitCell::*)(double, double, double, double, double, double, int const) const) & UnitCell::recAngle,
           (arg("self"), arg("h1"), arg("k1"), arg("l1"), arg("h2"), arg("k2"), arg("l2"),
            arg("Unit") = static_cast<int>(angDegrees)),
           "Returns the angle in reciprocal space between vectors given by "
           "(:math:`h_1, k_1, l_1`) and (:math:`h_2, k_2, l_2`) (in "
           "degrees or radians). The optional parameter ``Unit`` controls "
           "the units for the angles, and can have the value of ``Degrees`` or "
           "``Radians``. By default Unit = Degrees")
      .def("recVolume", (double(UnitCell::*)() const) & UnitCell::recVolume, arg("self"),
           "Return the volume of the reciprocal unit cell (in "
           ":math:`\\rm{\\AA}^{-3}`)")
      .def("set", (void(UnitCell::*)(double, double, double, double, double, double, int const)) & UnitCell::set,
           (arg("self"), arg("_a"), arg("_b"), arg("_c"), arg("_alpha"), arg("_beta"), arg("_gamma"),
            arg("Unit") = static_cast<int>(angDegrees)),
           "Set the parameters of the unit cell. Angles can be set in either"
           "degrees or radians using the ``Unit`` parameter (0 = degrees, "
           "1 = radians)")
      .def("seta", (void(UnitCell::*)(double))(&UnitCell::seta), (arg("self"), arg("_a")),
           "Set the length of the :math:`a` direction of the unit cell.")
      .def("setalpha", (void(UnitCell::*)(double, int const))(&UnitCell::setalpha),
           (arg("self"), arg("_alpha"), arg("Unit") = static_cast<int>(angDegrees)),
           "Set the :math:`\\alpha` angle of the unit cell. The angle can be "
           "set "
           "either in degrees or radians using the ``Unit`` parameter.")
      .def("setb", (void(UnitCell::*)(double))(&UnitCell::setb), (arg("self"), arg("_b")),
           "Set the length of the :math:`b` direction of the unit cell.")
      .def("setbeta", (void(UnitCell::*)(double, int const))(&UnitCell::setbeta),
           (arg("self"), arg("_beta"), arg("Unit") = static_cast<int>(angDegrees)),
           "Set the :math:`\\beta` angle of the unit cell. The angle can be "
           "set "
           "either in degrees or radians using the ``Unit`` parameter.")
      .def("setc", (void(UnitCell::*)(double))(&UnitCell::setc), (arg("self"), arg("_c")),
           "Set the length of the :math:`c` direction of the unit cell.")
      .def("setgamma", (void(UnitCell::*)(double, int const))(&UnitCell::setgamma),
           (arg("self"), arg("_gamma"), arg("Unit") = static_cast<int>(angDegrees)),
           "Set the :math:`\\gamma` angle of the unit cell. The angle can be "
           "set "
           "either in degrees or radians using the ``Unit`` parameter.")
      .def("setError",
           (void(UnitCell::*)(double, double, double, double, double, double, int const)) & UnitCell::setError,
           (arg("self"), arg("_aerr"), arg("_berr"), arg("_cerr"), arg("_alphaerr"), arg("_betaerr"), arg("_gammaerr"),
            arg("Unit") = static_cast<int>(angDegrees)),
           "Set the errors in the unit cell parameters.")
      .def("setErrora", (void(UnitCell::*)(double))(&UnitCell::setErrora), (arg("self"), arg("_aerr")),
           "Set the error in the length of the :math:`a` direction of the unit "
           "cell.")
      .def("setErroralpha", (void(UnitCell::*)(double, int const))(&UnitCell::setErroralpha),
           (arg("self"), arg("_alphaerr"), arg("Unit") = static_cast<int>(angDegrees)),
           "Set the error in the :math:`\\alpha` angle of the unit cell.")
      .def("setErrorb", (void(UnitCell::*)(double))(&UnitCell::setErrorb), (arg("self"), arg("_berr")),
           "Set the error in the length of the :math:`b` direction of the unit "
           "cell.")
      .def("setErrorbeta", (void(UnitCell::*)(double, int const))(&UnitCell::setErrorbeta),
           (arg("self"), arg("_betaerr"), arg("Unit") = static_cast<int>(angDegrees)),
           "Set the error in the :math:`\\beta` angle of the unit cell using "
           "the "
           "``Unit`` parameter.")
      .def("setErrorc", (void(UnitCell::*)(double))(&UnitCell::setErrorc), (arg("self"), arg("_cerr")),
           "Set the error in the length of the :math:`c` direction of the unit "
           "cell.")
      .def("setErrorgamma", (void(UnitCell::*)(double, int const))(&UnitCell::setErrorgamma),
           (arg("self"), arg("_gammaerr"), arg("Unit") = static_cast<int>(angDegrees)),
           "Set the error in the :math:`\\gamma` angle of the unit cell using "
           "the "
           "``Unit`` parameter.")
      .def("setModVec1", (void(UnitCell::*)(const V3D &)) & UnitCell::setModVec1, (arg("self"), arg("vec")),
           "Set the first modulated structure vector")
      .def("setModVec2", (void(UnitCell::*)(const V3D &)) & UnitCell::setModVec2, (arg("self"), arg("vec")),
           "Set the second modulated structure vector")
      .def("setModVec3", (void(UnitCell::*)(const V3D &)) & UnitCell::setModVec3, (arg("self"), arg("vec")),
           "Set the third modulated structure vector")
      .def("setMaxOrder", &UnitCell::setMaxOrder, "Set the maximum order of modulated vectors searched")
      .def("volume", (double(UnitCell::*)() const) & UnitCell::volume, arg("self"),
           "Return the volume of the unit cell (in :math:`\\rm{\\AA}{^3}`)")
      .def("getG", &UnitCell::getG, arg("self"), return_readonly_numpy(),
           "Returns the metric tensor for the unit cell. This will return a "
           ":class:`numpy.ndarray` with shape ``(3,3)``.")
      .def("getGstar", &UnitCell::getGstar, arg("self"), return_readonly_numpy(),
           "Returns the metric tensor for the reciprocal unit cell. This will "
           "return a :class:`numpy.ndarray` with shape ``(3,3)``.")
      .def("getB", &UnitCell::getB, arg("self"), return_readonly_numpy(),
           "Returns the :math:`B` matrix for this unit cell. This will be in a "
           "right-handed coordinate system and using the Busing-Levy "
           "convention. This will return a :class:`numpy.ndarray` with shape "
           "``(3,3)``.")
      .def("getBinv", &UnitCell::getBinv, arg("self"), return_readonly_numpy(),
           "Returns the inverse of the :math:`B` matrix for this unit cell."
           "This will return a :class:`numpy.ndarray` with shape ``(3,3)``. "
           "See also :func:`~mantid.geometry.UnitCell.getB`.")
      .def("getModHKL", &UnitCell::getModHKL, arg("self"), return_readonly_numpy(),
           "Returns the :math:`ModHKL` matrix for this unit cell. This will be "
           "in a "
           "right-handed coordinate system and using the Busing-Levy "
           "convention. This will return a :class:`numpy.ndarray` with shape "
           "``(3,3)``.")
      .def("getMaxOrder", &UnitCell::getMaxOrder, arg("self"),
           "Returns the number of modulation vectors. This will return an "
           "int.")
      .def("getModVec", &UnitCell::getModVec, (arg("self"), arg("i")), "Returns the ith modulation vector")
      .def("recalculateFromGstar", &recalculateFromGstar, (arg("self"), arg("NewGstar")),
           "Recalculate the unit cell parameters from a metric tensor. This "
           "method accepts a :class:`numpy.ndarray` with shape ``(3,3)``.")
      .def("__str__", &__str__implementation)
      .def("__repr__", &__repr__implementation);

  scope().attr("deg2rad") = Mantid::Geometry::deg2rad;
  scope().attr("rad2deg") = Mantid::Geometry::rad2deg;
}
