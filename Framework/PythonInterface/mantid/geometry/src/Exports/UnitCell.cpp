#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidPythonInterface/kernel/Converters/MatrixToNDArray.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToMatrix.h"
#include "MantidPythonInterface/kernel/Policies/MatrixToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/self.hpp>
#include <boost/python/operators.hpp>

using Mantid::Geometry::UnitCell;
using Mantid::Geometry::AngleUnits;
using Mantid::Geometry::angRadians;
using Mantid::Geometry::angDegrees;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::V3D;
using namespace boost::python;

// Functions purely to aid with wrapping
namespace //<unnamed>
    {
using namespace Mantid::PythonInterface;

/// Pass-through function to set the unit cell from a 2D numpy array
void recalculateFromGstar(UnitCell &self, object values) {
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
}

void export_UnitCell() {
  enum_<AngleUnits>("AngleUnits")
      .value("Degrees", angDegrees)
      .value("Radians", angRadians)
      .export_values();

  /// return_value_policy for read-only numpy array
  typedef return_value_policy<Policies::MatrixToNumpy<Converters::WrapReadOnly>>
      return_readonly_numpy;

  class_<UnitCell>("UnitCell", init<>())
      .def(init<UnitCell const &>((arg("self"), arg("other"))))
      .def(init<double, double, double>(
          (arg("self"), arg("_a"), arg("_b"), arg("_c"))))
      .def(init<double, double, double, double, double, double, optional<int>>(
          (arg("self"), arg("_a"), arg("_b"), arg("_c"), arg("_alpha"),
           arg("_beta"), arg("_gamma"),
           arg("Unit") = static_cast<int>(angDegrees))))
      .def("a", (double (UnitCell::*)() const) & UnitCell::a, arg("self"),
           "Returns the length of the ``a`` direction of the unit cell in "
           "Angstroms.")
      .def("a1", (double (UnitCell::*)() const) & UnitCell::a1, arg("self"),
           "Returns the length of the ``a`` direction of the unit cell. "
           "This is an alias for :func:`~mantid.geometry.UnitCell.a`")
      .def("a2", (double (UnitCell::*)() const) & UnitCell::a2, arg("self"),
           "Returns the length of the ``b`` direction of the unit cell. "
           "This is an alias for :func:`~mantid.geometry.UnitCell.b`")
      .def("a3", (double (UnitCell::*)() const) & UnitCell::a3, arg("self"),
           "Returns the length of the ``c`` direction of the unit cell. "
           "This is an alias for :func:`~mantid.geometry.UnitCell.c`")
      .def("alpha", (double (UnitCell::*)() const) & UnitCell::alpha,
           arg("self"),
           "Returns the ``alpha`` angle for this unit cell. The angle will "
           "either be in degrees or radians depending on the ``Unit`` "
           "parameter passed to the constructor.")
      .def("alpha1", (double (UnitCell::*)() const) & UnitCell::alpha1,
           arg("self"),
           "Returns the ``alpha`` angle of the unit cell. "
           "This is an alias for :func:`~mantid.geometry.UnitCell.alpha`")
      .def("alpha2", (double (UnitCell::*)() const) & UnitCell::alpha2,
           arg("self"),
           "Returns the ``beta`` angle of the unit cell. "
           "This is an alias for :func:`~mantid.geometry.UnitCell.beta`")
      .def("alpha3", (double (UnitCell::*)() const) & UnitCell::alpha3,
           arg("self"),
           "Returns the ``gamma`` angle of the unit cell. "
           "This is an alias for :func:`~mantid.geometry.UnitCell.gamma`")
      .def("alphastar", (double (UnitCell::*)() const) & UnitCell::alphastar,
           arg("self"),
           "Returns the reciprocal ``alpha`` angle for this unit cell. The "
           "angle "
           "will either be in degrees or radians depending on the ``Unit`` "
           "parameter passed to the constructor.")
      .def("astar", (double (UnitCell::*)() const) & UnitCell::astar,
           arg("self"),
           "Returns the length of the reciprocal ``a`` direction for this "
           "unit cell in reciprocal Angstroms.")
      .def("b", (double (UnitCell::*)() const) & UnitCell::b, arg("self"),
           "Returns the length of the ``b`` direction of the unit cell in "
           "Angstroms.")
      .def("b1", (double (UnitCell::*)() const) & UnitCell::b1, arg("self"),
           "Returns the length of the reciprocal ``a`` direction of the unit "
           "cell. This is an alias for :func:`~mantid.geometry.UnitCell.astar`")
      .def("b2", (double (UnitCell::*)() const) & UnitCell::b2, arg("self"),
           "Returns the length of the reciprocal ``b`` direction of the unit "
           "cell. This is an alias for :func:`~mantid.geometry.UnitCell.bstar`")
      .def("b3", (double (UnitCell::*)() const) & UnitCell::b3, arg("self"),
           "Returns the length of the reciprocal ``c`` direction of the unit "
           "cell. This is an alias for :func:`~mantid.geometry.UnitCell.cstar`")
      .def("beta", (double (UnitCell::*)() const) & UnitCell::beta, arg("self"),
           "Returns the ``beta`` angle for this unit cell. The angle will "
           "either be in degrees or radians depending on the ``Unit`` "
           "parameter passed to the constructor.")
      .def("beta1", (double (UnitCell::*)() const) & UnitCell::beta1,
           arg("self"),
           "Returns the reciprocal ``alpha`` angle of the unit cell. "
           "This is an alias for :func:`~mantid.geometry.UnitCell.alphastar`")
      .def("beta2", (double (UnitCell::*)() const) & UnitCell::beta2,
           arg("self"),
           "Returns the reciprocal ``beta`` angle of the unit cell. "
           "This is an alias for :func:`~mantid.geometry.UnitCell.betastar`")
      .def("beta3", (double (UnitCell::*)() const) & UnitCell::beta3,
           arg("self"),
           "Returns the reciprocal ``gamma`` angle of the unit cell. "
           "This is an alias for :func:`~mantid.geometry.UnitCell.gammastar`")
      .def("betastar", (double (UnitCell::*)() const) & UnitCell::betastar,
           arg("self"),
           "Returns the reciprocal ``beta`` angle for this unit cell. The "
           "angle "
           "will either be in degrees or radians depending on the ``Unit`` "
           "parameter passed to the constructor.")
      .def("bstar", (double (UnitCell::*)() const) & UnitCell::bstar,
           arg("self"),
           "Returns the length of the reciprocal ``b`` direction for this "
           "unit cell in reciprocal Angstroms.")
      .def("c", (double (UnitCell::*)() const) & UnitCell::c, arg("self"),
           "Returns the length of the ``c`` direction of the unit cell in "
           "Angstroms.")
      .def("cstar", (double (UnitCell::*)() const) & UnitCell::cstar,
           arg("self"),
           "Returns the length of the reciprocal ``c`` direction for this "
           "unit cell in reciprocal Angstroms.")
      .def("d",
           (double (UnitCell::*)(double, double, double) const) & UnitCell::d,
           (arg("self"), arg("h"), arg("k"), arg("l")),
           "Returns d-spacing for a given H, K, L coordinate in Angstroms.")
      .def("d", (double (UnitCell::*)(const V3D &) const) & UnitCell::d,
           (arg("self"), arg("hkl")),
           "Returns d-spacing for a given H, K, L coordinate in Angstroms.")
      .def("dstar", (double (UnitCell::*)(double, double, double) const) &
                        UnitCell::dstar,
           (arg("self"), arg("h"), arg("k"), arg("l")),
           "Returns reciprocal d-spacing for a given H, K, L coordinate in "
           "reciprocal Angstroms.")
      .def("errora", (double (UnitCell::*)() const) & UnitCell::errora,
           arg("self"), "Returns the error in the ``a`` unit cell length.")
      .def("errorb", (double (UnitCell::*)() const) & UnitCell::errorb,
           arg("self"), "Returns the error in the ``b`` unit cell length.")
      .def("errorc", (double (UnitCell::*)() const) & UnitCell::errorc,
           arg("self"), "Returns the error in the ``c`` unit cell length.")
      .def("erroralpha",
           (double (UnitCell::*)(int const) const) & UnitCell::erroralpha,
           (arg("self"), arg("Unit") = static_cast<int>(angDegrees)),
           "Returns the error in the ``alpha`` angle of the unit cell.")
      .def("errorbeta",
           (double (UnitCell::*)(int const) const) & UnitCell::errorbeta,
           (arg("self"), arg("Unit") = static_cast<int>(angDegrees)),
           "Returns the error in ``beta`` angle of the unit cell.")
      .def("errorgamma",
           (double (UnitCell::*)(int const) const) & UnitCell::errorgamma,
           (arg("self"), arg("Unit") = static_cast<int>(angDegrees)),
           "Returns the error in ``gamma`` angle of the unit cell.")
      .def("gamma", (double (UnitCell::*)() const) & UnitCell::gamma,
           arg("self"),
           "Returns the ``gamma`` angle for this unit cell. The angle will "
           "either be in degrees or radians depending on the ``Unit`` "
           "parameter passed to the constructor.")
      .def("gammastar", (double (UnitCell::*)() const) & UnitCell::gammastar,
           arg("self"),
           "Returns the reciprocal ``gamma`` angle for this unit cell. The "
           "angle "
           "will either be in degrees or radians depending on the ``Unit`` "
           "parameter passed to the constructor.")
      .def("recAngle", (double (UnitCell::*)(double, double, double, double,
                                             double, double, int const) const) &
                           UnitCell::recAngle,
           (arg("self"), arg("h1"), arg("k1"), arg("l1"), arg("h2"), arg("k2"),
            arg("l2"), arg("Unit") = static_cast<int>(angDegrees)),
           "Returns the angle between two HKL reflections in reciprocal space.")
      .def("recVolume", (double (UnitCell::*)() const) & UnitCell::recVolume,
           arg("self"),
           "Returns the volume of the reciprocal cell defined by this unit "
           "cell.")
      .def("set", (void (UnitCell::*)(double, double, double, double, double,
                                      double, int const)) &
                      UnitCell::set,
           (arg("self"), arg("_a"), arg("_b"), arg("_c"), arg("_alpha"),
            arg("_beta"), arg("_gamma"),
            arg("Unit") = static_cast<int>(angDegrees)),
           "Set the parameters of the unit cell. Angles can be set in either"
           "degrees or radians using the ``Unit`` parameter (0 = degrees, "
           "1 = radians)")
      .def("seta", (void (UnitCell::*)(double))(&UnitCell::seta),
           (arg("self"), arg("_a")),
           "Set the length of the ``a`` direction of the unit cell.")
      .def("setalpha",
           (void (UnitCell::*)(double, int const))(&UnitCell::setalpha),
           (arg("self"), arg("_alpha"),
            arg("Unit") = static_cast<int>(angDegrees)),
           "Set the ``alpha`` angle of the unit cell. The angle can be set "
           "either in degrees or radians using the ``Unit`` parameter.")
      .def("setb", (void (UnitCell::*)(double))(&UnitCell::setb),
           (arg("self"), arg("_b")),
           "Set the length of the ``b`` direction of the unit cell.")
      .def("setbeta",
           (void (UnitCell::*)(double, int const))(&UnitCell::setbeta),
           (arg("self"), arg("_beta"),
            arg("Unit") = static_cast<int>(angDegrees)),
           "Set the ``beta`` angle of the unit cell. The angle can be set "
           "either in degrees or radians using the ``Unit`` parameter.")
      .def("setc", (void (UnitCell::*)(double))(&UnitCell::setc),
           (arg("self"), arg("_c")),
           "Set the length of the ``c`` direction of the unit cell.")
      .def("setgamma",
           (void (UnitCell::*)(double, int const))(&UnitCell::setgamma),
           (arg("self"), arg("_gamma"),
            arg("Unit") = static_cast<int>(angDegrees)),
           "Set the ``gamma`` angle of the unit cell. The angle can be set "
           "either in degrees or radians using the ``Unit`` parameter.")
      .def("setError", (void (UnitCell::*)(double, double, double, double,
                                           double, double, int const)) &
                           UnitCell::setError,
           (arg("self"), arg("_aerr"), arg("_berr"), arg("_cerr"),
            arg("_alphaerr"), arg("_betaerr"), arg("_gammaerr"),
            arg("Unit") = static_cast<int>(angDegrees)),
           "Set the errors in the unit cell parameters.")
      .def("setErrora", (void (UnitCell::*)(double))(&UnitCell::setErrora),
           (arg("self"), arg("_aerr")),
           "Set the error in the length of the ``a`` direction of the unit "
           "cell.")
      .def("setErroralpha",
           (void (UnitCell::*)(double, int const))(&UnitCell::setErroralpha),
           (arg("self"), arg("_alphaerr"),
            arg("Unit") = static_cast<int>(angDegrees)),
           "Set the error in the ``alpha`` angle of the unit cell.")
      .def("setErrorb", (void (UnitCell::*)(double))(&UnitCell::setErrorb),
           (arg("self"), arg("_berr")),
           "Set the error in the length of the ``b`` direction of the unit "
           "cell.")
      .def("setErrorbeta",
           (void (UnitCell::*)(double, int const))(&UnitCell::setErrorbeta),
           (arg("self"), arg("_betaerr"),
            arg("Unit") = static_cast<int>(angDegrees)),
           "Set the error in the ``beta`` angle of the unit cell using the "
           "``Unit`` parameter.")
      .def("setErrorc", (void (UnitCell::*)(double))(&UnitCell::setErrorc),
           (arg("self"), arg("_cerr")),
           "Set the error in the length of the ``c`` direction of the unit "
           "cell.")
      .def("setErrorgamma",
           (void (UnitCell::*)(double, int const))(&UnitCell::setErrorgamma),
           (arg("self"), arg("_gammaerr"),
            arg("Unit") = static_cast<int>(angDegrees)),
           "Set the error in the ``gamma`` angle of the unit cell using the "
           "``Unit`` parameter.")
      .def("volume", (double (UnitCell::*)() const) & UnitCell::volume,
           arg("self"), "Returns the volume of the unit cell.")
      .def("getG", &UnitCell::getG, arg("self"), return_readonly_numpy(),
           "Returns the metric tensor for the unit cell. This will return a "
           ":class:`numpy.ndarray` with shape ``(3,3)``.")
      .def("getGstar", &UnitCell::getGstar, arg("self"),
           return_readonly_numpy(),
           "Returns the metric tensor for the reciprocal unit cell. This will "
           "return a :class:`numpy.ndarray` with shape ``(3,3)``.")
      .def("getB", &UnitCell::getB, arg("self"), return_readonly_numpy(),
           "Returns the B matrix for this unit cell. This will be in a "
           "right-handed coordinate system and using the Busing-Levy "
           "convention. This will return a :class:`numpy.ndarray` with shape "
           "``(3,3)``.")
      .def("getBinv", &UnitCell::getBinv, arg("self"), return_readonly_numpy(),
           "Returns the inverse of the B matrix for this unit cell."
           "This will return a :class:`numpy.ndarray` with shape ``(3,3)``. "
           "See also :func:`~mantid.geometry.UnitCell.getB`.")
      .def("recalculateFromGstar", &recalculateFromGstar,
           (arg("self"), arg("NewGstar")),
           "Recalculate the unit cell parameters from a metric tensor. This "
           "method accepts a :class:`numpy.ndarray` with shape ``(3,3)``.")
      .def("__str__", &__str__implementation)
      .def("__repr__", &__repr__implementation);

  scope().attr("deg2rad") = Mantid::Geometry::deg2rad;
  scope().attr("rad2deg") = Mantid::Geometry::rad2deg;
}
