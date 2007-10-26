#include "../inc/Workspace1D.h"

DECLARE_WORKSPACE(Workspace1D)

namespace Mantid
{

Workspace1D::Workspace1D()
{
}

Workspace1D::Workspace1D(const Workspace1D& w)
{
	_data=w._data;
}
Workspace1D& Workspace1D::operator=(const Workspace1D& w)
{
  _data=w._data;
  return *this;
}Workspace1D::~Workspace1D()
{
}

long int Workspace1D::getMemorySize() const 
{
	long int msize=
	sizeof(double);
	int tt=1;
	if (_data.isError()) tt=2;
	msize*=(_data.nxbin()+_data.nybin()*tt);
	return msize;
}

void Workspace1D::setX(const std::vector<double>& v)
{
	_data.setX(v);
}

void Workspace1D::setData(const std::vector<double>& y)
{
	_data.setData(y);
}

void Workspace1D::setData(const std::vector<double>& y, const std::vector<double>& e)
{
	_data.setData(y,e);
}

void Workspace1D::setX(const Mantid::Histogram1D::parray& v)
{
	_data.setX(v);
}

void Workspace1D::setData(const Mantid::Histogram1D::parray& y)
{
	_data.setData(y);
}

void Workspace1D::setData(const Mantid::Histogram1D::parray& y, const Mantid::Histogram1D::parray& e)
{
	_data.setData(y,e);
}
const std::vector<double>& Workspace1D::getX() const
{
	return _data.getX();
}
const std::vector<double>& Workspace1D::getY() const
{
	return _data.getY();
}
const std::vector<double>& Workspace1D::getE() const
{
	return _data.getE();
}


} //NamespaceMantid
