#include "MantidPythonAPI/geometryhelper.h"
#include "MantidPythonAPI/MantidVecHelper.h"
#include "MantidKernel/V3D.h"
#include <numpy/arrayobject.h>
#include <boost/python/extract.hpp>
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

void UnitCellWrapper::recalculateFromGStar(UnitCell& self,PyObject* p)
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

//void OrientedLatticeWrapper::recalculateFromGstar(OrientedLattice& self,PyObject* p)
//{
//  Kernel::DblMatrix m=MantidVecHelper::getMatrixFromArray(p),unity(3,3,true);
//  if ((m.numRows()!=3) || (m.numCols()!=3)) throw std::invalid_argument("Not 3x3 matrix");
//  self.recalculateFromGstar(m);
//  self.setU(unity);
//}

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

void OrientedLatticeWrapper::setUFromVectors(OrientedLattice& self,PyObject* p1, PyObject* p2)
{
  std::string type1(p1->ob_type->tp_name),type2(p2->ob_type->tp_name);
  Kernel::V3D u,v;
  _import_array();

  if (type1=="list")
  {
    boost::python::list l1=boost::python::extract<boost::python::list>(p1);
    if (len(l1)==3)
    {
      u[0]=boost::python::extract< double >(l1[0]);
      u[1]=boost::python::extract< double >(l1[1]);
      u[2]=boost::python::extract< double >(l1[2]);
    }
    else throw std::invalid_argument("List1 not of length 3");
  }
  else if (type1=="V3D")
  {
    u=boost::python::extract< Kernel::V3D >(p1);
  }
  else if (PyArray_Check(p1)==1)
  {
    boost::python::numeric::array a1=boost::python::extract<boost::python::numeric::array>(p1);
    a1=(boost::python::numeric::array) a1.astype('d');//force the array to be of double type (in case it was int)
    if (PyArray_Size(a1.ptr())!=3) throw std::invalid_argument("Parameter 1 length is not 3");
    u[0]=boost::python::extract< double >(a1[0]);
    u[1]=boost::python::extract< double >(a1[1]);
    u[2]=boost::python::extract< double >(a1[2]);
  }
  else throw std::invalid_argument("Type of parameter 1 is unknown");
  if (type2=="list")
  {
    boost::python::list l2=boost::python::extract<boost::python::list>(p2);
    if (len(l2)==3)
    {
      v[0]=boost::python::extract< double >(l2[0]);
      v[1]=boost::python::extract< double >(l2[1]);
      v[2]=boost::python::extract< double >(l2[2]);
    }
    else throw std::invalid_argument("List2 not of length 3");
  }
  else if (type2=="V3D")
  {
    v=boost::python::extract< Kernel::V3D >(p2);
  }
  else if (PyArray_Check(p2)==1)
  {
    boost::python::numeric::array a2=boost::python::extract<boost::python::numeric::array>(p2);
    a2=(boost::python::numeric::array) a2.astype('d');//force the array to be of double type (in case it was int)
    if (PyArray_Size(a2.ptr())!=3) throw std::invalid_argument("Parameter 2 length is not 3");
    v[0]=boost::python::extract< double >(a2[0]);
    v[1]=boost::python::extract< double >(a2[1]);
    v[2]=boost::python::extract< double >(a2[2]);
  }
  else throw std::invalid_argument("Type of parameter 2 is unknown");
  self.setUFromVectors(u,v);
}

}//PythonAPI
}//Mantid
