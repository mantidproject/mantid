#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidPythonInterface/kernel/Converters/MatrixToNDArray.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToV3D.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToMatrix.h"

#include "MantidPythonInterface/kernel/Policies/MatrixToNumpy.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using Mantid::Geometry::Goniometer;
using namespace boost::python;

namespace //<unnamed>
{
  ///@cond
  // define overloaded functions
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getEulerAngles_overloads, Goniometer::getEulerAngles, 0, 1);
  ///@endcond

  /// Set the U vector via a numpy array
  void setR(Goniometer & self, const object& data)
  {
    self.setR(Mantid::PythonInterface::Converters::PyObjectToMatrix(data)());
  }
}

void export_Goniometer()
{

  // return_value_policy for read-only numpy array
  typedef return_value_policy<Mantid::PythonInterface::Policies::MatrixToNumpy< Mantid::PythonInterface::Converters::WrapReadOnly> > return_readonly_numpy;

  class_<Goniometer>("Goniometer", init< >(arg("self")))
    .def( init< Goniometer const & >(( arg("self"), arg("other") )) )
    .def( init< Mantid::Kernel::DblMatrix >(( arg("self"), arg("rot") )) )
    .def( "getEulerAngles", (&Goniometer::getEulerAngles),
          getEulerAngles_overloads(args("self", "convention"),
                                   "Default convention is \'YZX\'. Universal goniometer is \'YZY\'"))
	.def("getR",&Goniometer::getR, return_readonly_numpy())
	.def("setR", &setR)
    ;

}

