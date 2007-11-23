#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"

DECLARE_WORKSPACE(Workspace2D)

namespace Mantid
{
namespace DataObjects
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
	if (nhist<0) throw std::invalid_argument("Workspace2D::setHistogramNumber, invalid histograms number <0");
	_data.resize(nhist);
	_nhistogram=nhist; 
}

long int Workspace2D::getMemorySize() const 
{ //to be changed
	return 0;
}

void Workspace2D::setX(int histnumber, const std::vector<double>& v)
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::range_error("Workspace2D::setX, histogram number out of range");
	_data[histnumber].setX(v);
}

void Workspace2D::setData(int histnumber, const std::vector<double>& y)
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::range_error("Workspace2D::setData, histogram number out of range");
	_data[histnumber].setData(y);
}

void Workspace2D::setData(int histnumber, const std::vector<double>& y, const std::vector<double>& e)
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::range_error("Workspace2D::setData, histogram number out of range");	
	_data[histnumber].setData(y,e);
}

void Workspace2D::setX(int histnumber, const Histogram1D::parray& v)
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::range_error("Workspace2D::setX, histogram number out of range");
	_data[histnumber].setX(v);
}

void Workspace2D::setData(int histnumber, const Histogram1D::parray& y)
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::range_error("Workspace2D::setData, histogram number out of range");
	_data[histnumber].setData(y);
}

void Workspace2D::setData(int histnumber, const Histogram1D::parray& y, const Histogram1D::parray& e)
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::range_error("Workspace2D::setData, histogram number out of range");
	_data[histnumber].setData(y,e);
}

const int Workspace2D::getHistogramNumber() const
{
  return _nhistogram;
}

const std::vector<double>& Workspace2D::getX(int histnumber) const
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::range_error("Workspace2D::getX, histogram number out of range");
	return _data[histnumber].getX();
}
const std::vector<double>& Workspace2D::getY(int histnumber) const
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::range_error("Workspace2D::getY, histogram number out of range");
	return _data[histnumber].getY();
}
const std::vector<double>& Workspace2D::getE(int histnumber) const
{
	if (histnumber<0 || histnumber>_nhistogram-1) throw std::range_error("Workspace2D::setX, histogram number out of range");
	return _data[histnumber].getE();	
}

} // namespace DataObjects
} //NamespaceMantid
