#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidPythonInterface/kernel/Converters/MatrixToNDArray.h"
#include "MantidPythonInterface/kernel/Policies/MatrixToNumpy.h"
#include "MantidPythonInterface/kernel/NumpyConverters.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::OrientedLattice;
using Mantid::Geometry::UnitCell;
using Mantid::Geometry::angDegrees;
using namespace boost::python;

namespace //<unnamed>
{
  using namespace Mantid::PythonInterface;

  /// Set the U vector via a numpy array
  void setU(OrientedLattice & self, PyObject *data)
  {
    self.setU(Numpy::createDoubleMatrix(data));
  }

  /// Set the U vector via a numpy array
  void setUB(OrientedLattice & self, PyObject *data)
  {
    self.setUB(Numpy::createDoubleMatrix(data));
  }

  /// Set the U matrix from 2 Python objects representing a V3D type. This can be a V3D object, a list
  /// or a numpy array. If the arrays are used they must be of length 3
  void setUFromVectors(OrientedLattice & self, PyObject * vec1, PyObject *vec2)
  {
    self.setUFromVectors(Numpy::createV3D(vec1), Numpy::createV3D(vec2));
  }

}

void export_OrientedLattice()
{
  /// return_value_policy for read-only numpy array
  typedef return_value_policy<Policies::MatrixToNumpy<Converters::WrapReadOnly> > return_readonly_numpy;

  class_<OrientedLattice, bases<UnitCell> >("OrientedLattice", init< >())
    .def( init< OrientedLattice const & >(( arg("other") )) )
    .def( init< double, double, double >(( arg("_a"), arg("_b"), arg("_c") )) )
    .def( init< double, double, double, double, double, double,
                optional< int > >(( arg("_a"), arg("_b"), arg("_c"), arg("_alpha"), arg("_beta"), arg("_gamma"), arg("Unit")=(int)(angDegrees) )) )
    .def( init<UnitCell>( arg("uc") ) )
    .def( "getuVector", (&OrientedLattice::getuVector))
    .def( "getvVector", (&OrientedLattice::getvVector))
    .def( "getU", &OrientedLattice::getU, return_readonly_numpy() )
    .def( "setU", &setU )
    .def( "getUB",&OrientedLattice::getUB, return_readonly_numpy() )
    .def( "setUB",&setUB )
    .def( "setUFromVectors", &setUFromVectors)

    ;
}

