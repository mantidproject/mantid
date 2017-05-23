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
      .def("a", (double (UnitCell::*)() const) & UnitCell::a, arg("self"))
      .def("a1", (double (UnitCell::*)() const) & UnitCell::a1, arg("self"))
      .def("a2", (double (UnitCell::*)() const) & UnitCell::a2, arg("self"))
      .def("a3", (double (UnitCell::*)() const) & UnitCell::a3, arg("self"))
      .def("alpha", (double (UnitCell::*)() const) & UnitCell::alpha,
           arg("self"))
      .def("alpha1", (double (UnitCell::*)() const) & UnitCell::alpha1,
           arg("self"))
      .def("alpha2", (double (UnitCell::*)() const) & UnitCell::alpha2,
           arg("self"))
      .def("alpha3", (double (UnitCell::*)() const) & UnitCell::alpha3,
           arg("self"))
      .def("alphastar", (double (UnitCell::*)() const) & UnitCell::alphastar,
           arg("self"))
      .def("astar", (double (UnitCell::*)() const) & UnitCell::astar,
           arg("self"))
      .def("b", (double (UnitCell::*)() const) & UnitCell::b, arg("self"))
      .def("b1", (double (UnitCell::*)() const) & UnitCell::b1, arg("self"))
      .def("b2", (double (UnitCell::*)() const) & UnitCell::b2, arg("self"))
      .def("b3", (double (UnitCell::*)() const) & UnitCell::b3, arg("self"))
      .def("beta", (double (UnitCell::*)() const) & UnitCell::beta, arg("self"))
      .def("beta1", (double (UnitCell::*)() const) & UnitCell::beta1,
           arg("self"))
      .def("beta2", (double (UnitCell::*)() const) & UnitCell::beta2,
           arg("self"))
      .def("beta3", (double (UnitCell::*)() const) & UnitCell::beta3,
           arg("self"))
      .def("betastar", (double (UnitCell::*)() const) & UnitCell::betastar,
           arg("self"))
      .def("bstar", (double (UnitCell::*)() const) & UnitCell::bstar,
           arg("self"))
      .def("c", (double (UnitCell::*)() const) & UnitCell::c, arg("self"))
      .def("cstar", (double (UnitCell::*)() const) & UnitCell::cstar,
           arg("self"))
      .def("d",
           (double (UnitCell::*)(double, double, double) const) & UnitCell::d,
           (arg("self"), arg("h"), arg("k"), arg("l")))
      .def("d", (double (UnitCell::*)(const V3D &) const) & UnitCell::d,
           (arg("self"), arg("hkl")))
      .def("dstar", (double (UnitCell::*)(double, double, double) const) &
                        UnitCell::dstar,
           (arg("self"), arg("h"), arg("k"), arg("l")))
      .def("errora", (double (UnitCell::*)() const) & UnitCell::errora,
           arg("self"))
      .def("errorb", (double (UnitCell::*)() const) & UnitCell::errorb,
           arg("self"))
      .def("errorc", (double (UnitCell::*)() const) & UnitCell::errorc,
           arg("self"))
      .def("erroralpha",
           (double (UnitCell::*)(int const) const) & UnitCell::erroralpha,
           (arg("self"), arg("Unit") = static_cast<int>(angDegrees)))
      .def("errorbeta",
           (double (UnitCell::*)(int const) const) & UnitCell::errorbeta,
           (arg("self"), arg("Unit") = static_cast<int>(angDegrees)))
      .def("errorgamma",
           (double (UnitCell::*)(int const) const) & UnitCell::errorgamma,
           (arg("self"), arg("Unit") = static_cast<int>(angDegrees)))
      .def("gamma", (double (UnitCell::*)() const) & UnitCell::gamma,
           arg("self"))
      .def("gammastar", (double (UnitCell::*)() const) & UnitCell::gammastar,
           arg("self"))
      .def("recAngle", (double (UnitCell::*)(double, double, double, double,
                                             double, double, int const) const) &
                           UnitCell::recAngle,
           (arg("self"), arg("h1"), arg("k1"), arg("l1"), arg("h2"), arg("k2"),
            arg("l2"), arg("Unit") = static_cast<int>(angDegrees)))
      .def("recVolume", (double (UnitCell::*)() const) & UnitCell::recVolume,
           arg("self"))
      .def("set", (void (UnitCell::*)(double, double, double, double, double,
                                      double, int const)) &
                      UnitCell::set,
           (arg("self"), arg("_a"), arg("_b"), arg("_c"), arg("_alpha"),
            arg("_beta"), arg("_gamma"),
            arg("Unit") = static_cast<int>(angDegrees)))
      .def("seta", (void (UnitCell::*)(double))(&UnitCell::seta),
           (arg("self"), arg("_a")))
      .def("setalpha",
           (void (UnitCell::*)(double, int const))(&UnitCell::setalpha),
           (arg("self"), arg("_alpha"),
            arg("Unit") = static_cast<int>(angDegrees)))
      .def("setb", (void (UnitCell::*)(double))(&UnitCell::setb),
           (arg("self"), arg("_b")))
      .def("setbeta",
           (void (UnitCell::*)(double, int const))(&UnitCell::setbeta),
           (arg("self"), arg("_beta"),
            arg("Unit") = static_cast<int>(angDegrees)))
      .def("setc", (void (UnitCell::*)(double))(&UnitCell::setc),
           (arg("self"), arg("_c")))
      .def("setgamma",
           (void (UnitCell::*)(double, int const))(&UnitCell::setgamma),
           (arg("self"), arg("_gamma"),
            arg("Unit") = static_cast<int>(angDegrees)))
      .def("setError", (void (UnitCell::*)(double, double, double, double,
                                           double, double, int const)) &
                           UnitCell::setError,
           (arg("self"), arg("_aerr"), arg("_berr"), arg("_cerr"),
            arg("_alphaerr"), arg("_betaerr"), arg("_gammaerr"),
            arg("Unit") = static_cast<int>(angDegrees)))
      .def("setErrora", (void (UnitCell::*)(double))(&UnitCell::setErrora),
           (arg("self"), arg("_aerr")))
      .def("setErroralpha",
           (void (UnitCell::*)(double, int const))(&UnitCell::setErroralpha),
           (arg("self"), arg("_alphaerr"),
            arg("Unit") = static_cast<int>(angDegrees)))
      .def("setErrorb", (void (UnitCell::*)(double))(&UnitCell::setErrorb),
           (arg("self"), arg("_berr")))
      .def("setErrorbeta",
           (void (UnitCell::*)(double, int const))(&UnitCell::setErrorbeta),
           (arg("self"), arg("_betaerr"),
            arg("Unit") = static_cast<int>(angDegrees)))
      .def("setErrorc", (void (UnitCell::*)(double))(&UnitCell::setErrorc),
           (arg("self"), arg("_cerr")))
      .def("setErrorgamma",
           (void (UnitCell::*)(double, int const))(&UnitCell::setErrorgamma),
           (arg("self"), arg("_gammaerr"),
            arg("Unit") = static_cast<int>(angDegrees)))
      .def("volume", (double (UnitCell::*)() const) & UnitCell::volume,
           arg("self"))
      .def("getG", &UnitCell::getG, arg("self"), return_readonly_numpy())
      .def("getGstar", &UnitCell::getGstar, arg("self"),
           return_readonly_numpy())
      .def("getB", &UnitCell::getB, arg("self"), return_readonly_numpy())
      .def("getBinv", &UnitCell::getBinv, arg("self"), return_readonly_numpy())
      .def("recalculateFromGstar", &recalculateFromGstar,
           (arg("self"), arg("NewGstar")))
      .def("__str__", &__str__implementation)
      .def("__repr__", &__repr__implementation);

  scope().attr("deg2rad") = Mantid::Geometry::deg2rad;
  scope().attr("rad2deg") = Mantid::Geometry::rad2deg;
}
