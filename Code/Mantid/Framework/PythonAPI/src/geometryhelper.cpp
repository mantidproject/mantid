#include "MantidPythonAPI/geometryhelper.h"
#include "MantidPythonAPI/MantidVecHelper.h"

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
  Kernel::DblMatrix m=MantidVecHelper::getMatrixFromArray(p);
  if ((m.numRows()!=3) || (m.numCols()!=3)) throw std::invalid_argument("Not 3x3 matrix"); 
  self.recalculateFromGstar(m); 
}


OrientedLatticeWrapper::OrientedLatticeWrapper(PyObject *self) : m_self(self)
{
  Py_INCREF(m_self);
}

OrientedLatticeWrapper::OrientedLatticeWrapper(PyObject *self,PyObject* p) : m_self(self)
{
  Py_INCREF(m_self);
  //Geometry::Matrix<double> m=MantidVecHelper::getMatrixFromArray(p);
  //if ((m.numRows()!=3) || (m.numCols()!=3)) throw std::invalid_argument("Not 3x3 matrix"); 
  this->setU(*this,p);
}

OrientedLatticeWrapper::~OrientedLatticeWrapper()
{
  Py_DECREF(m_self);
}

PyObject* OrientedLatticeWrapper::getG(OrientedLattice& self)
{
	return MantidVecHelper::createPythonWrapper(self.getG(), true);
}

PyObject* OrientedLatticeWrapper::getGstar(OrientedLattice& self)
{
	return MantidVecHelper::createPythonWrapper(self.getGstar(), true);
}

PyObject* OrientedLatticeWrapper::getB(OrientedLattice& self)
{
	return MantidVecHelper::createPythonWrapper(self.getB(), true);
}

PyObject* OrientedLatticeWrapper::getU(OrientedLattice& self)
{
	return MantidVecHelper::createPythonWrapper(self.getU(), true);
}

PyObject* OrientedLatticeWrapper::getUB(OrientedLattice& self)
{
	return MantidVecHelper::createPythonWrapper(self.getUB(), true);
}

void OrientedLatticeWrapper::recalculateFromGstar(OrientedLattice& self,PyObject* p)
{
  Kernel::DblMatrix m=MantidVecHelper::getMatrixFromArray(p),unity(3,3,true);
  if ((m.numRows()!=3) || (m.numCols()!=3)) throw std::invalid_argument("Not 3x3 matrix"); 
  self.recalculateFromGstar(m); 
  self.setU(unity);
}

void OrientedLatticeWrapper::setU(OrientedLattice& self,PyObject* p)
{
  Kernel::DblMatrix m=MantidVecHelper::getMatrixFromArray(p);
  if ((m.numRows()!=3) || (m.numCols()!=3)) throw std::invalid_argument("Not 3x3 matrix"); 
  self.setU(m); 
}

void OrientedLatticeWrapper::setUB(OrientedLattice& self,PyObject* p)
{
  Kernel::DblMatrix m=MantidVecHelper::getMatrixFromArray(p);
  if ((m.numRows()!=3) || (m.numCols()!=3)) throw std::invalid_argument("Not 3x3 matrix"); 
  self.setUB(m); 
}

}
}
