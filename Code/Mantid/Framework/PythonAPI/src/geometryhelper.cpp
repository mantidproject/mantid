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

}
}
