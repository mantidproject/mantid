#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidPythonInterface/kernel/Converters/MatrixToNDArray.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToV3D.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToMatrix.h"

#include "MantidPythonInterface/kernel/Policies/MatrixToNumpy.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::Goniometer;
using namespace boost::python;

namespace //<unnamed>
{

}

void export_Goniometer()
{

  class_<Goniometer>("Goniometer", init< >())
    .def( init< Goniometer const & >(( arg("other") )) )
    .def( init< Mantid::Kernel::DblMatrix >(( arg("rot") )) )
    .def( "getEulerAngles", (&Goniometer::getEulerAngles))

    ;
}

