#include "Workspace2D.h"
#include <stdexcept> 

namespace Mantid
{

Workspace2D::Workspace2D()
{
}

Workspace2D::Workspace2D(const Workspace2D& w)
{
	_data=w._data;
}
Workspace2D& Workspace2D::operator=(const Workspace2D& w)
{
  _data=w._data;
  return *this;
}
Workspace2D::~Workspace2D()
{
}

void Workspace2D::setHistogramNumber(int nhist)
{
	if (nhist<0) throw std::runtime_error("Workspace2D::setHistogramNumber, invalid histograms number <0");
	_data.resize(nhist);
	
}

long int Workspace2D::getMemorySize() const 
{ //to be changed
	return 0;
}

void Workspace2D::setX(int histnumber, const std::vector<double>& v)
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::runtime_error("Workspace2D::setX, histogram number out of range");
	_data[histnumber].setX(v);
}

void Workspace2D::setData(int histnumber, const std::vector<double>& y)
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::runtime_error("Workspace2D::setData, histogram number out of range");
	_data[histnumber].setData(y);
}

void Workspace2D::setData(int histnumber, const std::vector<double>& y, const std::vector<double>& e)
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::runtime_error("Workspace2D::setData, histogram number out of range");	
	_data[histnumber].setData(y,e);
}

void Workspace2D::setX(int histnumber, const Mantid::Histogram1D::parray& v)
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::runtime_error("Workspace2D::setX, histogram number out of range");
	_data[histnumber].setX(v);
}

void Workspace2D::setData(int histnumber, const Mantid::Histogram1D::parray& y)
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::runtime_error("Workspace2D::setData, histogram number out of range");
	_data[histnumber].setData(y);
}

void Workspace2D::setData(int histnumber, const Mantid::Histogram1D::parray& y, const Mantid::Histogram1D::parray& e)
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::runtime_error("Workspace2D::setData, histogram number out of range");
	_data[histnumber].setData(y,e);
}
const std::vector<double> Workspace2D::getX(int histnumber) const
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::runtime_error("Workspace2D::getX, histogram number out of range");
	return _data[histnumber].getX();
}
const std::vector<double> Workspace2D::getY(int histnumber) const
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::runtime_error("Workspace2D::getY, histogram number out of range");
	return _data[histnumber].getY();
}
const std::vector<double> Workspace2D::getE(int histnumber) const
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::runtime_error("Workspace2D::setX, histogram number out of range");
	return _data[histnumber].getE();	
}


} //NamespaceMantid
