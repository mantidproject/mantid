// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/Converters/MatrixToNDArray.h"
#include "MantidPythonInterface/core/Converters/PyObjectToMatrix.h"
#include "MantidPythonInterface/core/Converters/PyObjectToV3D.h"

#include "MantidPythonInterface/core/Policies/MatrixToNumpy.h"
#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/overloads.hpp>

using Mantid::Geometry::Goniometer;
using Mantid::Kernel::DblMatrix;
using namespace Mantid::PythonInterface;
using namespace boost::python;

namespace //<unnamed>
{
///@cond
GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
// define overloaded functions
// cppcheck-suppress unknownMacro
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getEulerAngles_overloads, Goniometer::getEulerAngles, 0, 1)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")
///@endcond

/// Set the U vector via a numpy array
void setR(Goniometer &self, const object &data) { self.setR(Converters::PyObjectToMatrix(data)()); }

/// calc goniometer with V3D sample position input
void calcFromQSampleAndWavelength(Goniometer &self, const object &position, double wavelength, bool flip_x,
                                  bool inner) {
  self.calcFromQSampleAndWavelength(Converters::PyObjectToV3D(position)(), wavelength, flip_x, inner);
}

boost::python::dict getAxis(const Goniometer &self, size_t axisnumber) {
  const auto &axis = self.getAxis(axisnumber);

  boost::python::dict pythonAxis;
  pythonAxis["rotationaxis"] = axis.rotationaxis;
  pythonAxis["angle"] = axis.angle;
  pythonAxis["sense"] = axis.sense;

  return pythonAxis;
}
} // namespace

void export_Goniometer() {

  // return_value_policy for read-only numpy array
  using return_readonly_numpy = return_value_policy<Policies::MatrixRefToNumpy<Converters::WrapReadOnly>>;

  class_<Goniometer>("Goniometer", init<>(arg("self")))
      .def(init<Goniometer const &>((arg("self"), arg("other"))))
      .def(init<DblMatrix>((arg("self"), arg("rot"))))
      .def("getEulerAngles", (&Goniometer::getEulerAngles),
           getEulerAngles_overloads(args("self", "convention"), "Default convention is \'YZX\'. Universal "
                                                                "goniometer is \'YZY\'"))
      .def("getR", &Goniometer::getR, arg("self"), return_readonly_numpy())
      .def("setR", &setR, (arg("self"), arg("rot")))
      .def("getNumberAxes", &Goniometer::getNumberAxes, arg("self"))
      .def("getAxis", &getAxis, (arg("self"), arg("axisnumber")))
      .def("calcFromQSampleAndWavelength", &calcFromQSampleAndWavelength,
           (arg("self"), arg("positions"), arg("wavelength"), arg("flip_x") = false, arg("inner") = false));
}
