#include "MantidPythonAPI/geometryhelper.h"
#include "MantidPythonAPI/MantidVecHelper.h"
#include <MantidGeometry/Crystal/UnitCell.h>

namespace Mantid 
{
namespace PythonAPI
{
UnitCellWrapper::UnitCellWrapper(PyObject *self) : m_self(self)
{
  Py_INCREF(m_self);
}

UnitCellWrapper::~UnitCellWrapper()
{
  Py_DECREF(m_self);
}

PyObject* UnitCellWrapper::getG(UnitCell& self)
{
	return MantidVecHelper::createPythonWrapper(self.getG(), true);
}

PyObject* UnitCellWrapper::getGstar(UnitCell& self)
{
	return MantidVecHelper::createPythonWrapper(self.getGstar(), true);
}

PyObject* UnitCellWrapper::getB(UnitCell& self)
{
	return MantidVecHelper::createPythonWrapper(self.getB(), true);
}

void UnitCellWrapper::recalculateFromGstar(UnitCell& self,PyObject* p)
{
  Geometry::Matrix<double> m=MantidVecHelper::getMatrixFromArray(p);
  if ((m.numRows()!=3) || (m.numCols()!=3)) throw std::invalid_argument("Not 3x3 matrix"); 
  self.recalculateFromGstar(m); 
}

}
}
