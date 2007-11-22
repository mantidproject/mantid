#include "Histogram1D.h"
#include "Exception.h"

#include <iostream> 

namespace Mantid
{
namespace DataObjects
{


Histogram1D::Histogram1D()
{
	_X=parray(new std::vector<double>);
	_Y=parray(new std::vector<double>);
	_E=parray(new std::vector<double>);
	_nxbin=0;
	_nybin=0;
}

Histogram1D::~Histogram1D() //Nothing to do since _X, _Y, and _E are managed ptr
{
}
Histogram1D::Histogram1D(const Histogram1D& r)
{
	_X=r._X;
	_Y=r._Y;
	_E=r._E;
	_nxbin=_X->size();
	_nybin=_Y->size();
}
Histogram1D& Histogram1D::operator=(const Histogram1D& r)
{
	_X=r._X;
	_Y=r._Y;
	_E=r._E;
	_nxbin=_X->size();
	_nybin=_Y->size();
	return *this;
}
void Histogram1D::setX(const std::vector<double>& x)
{
	if (!_X.unique()) //Either not defined of multiple reference
	{
		_X.reset();
		_X=parray(new std::vector<double>);
	}
	*(_X)=x;
	_nxbin=_X->size();
	return;
}
void Histogram1D::setX(const parray& x)
{
	_X=x;
	_nxbin=x->size();
	return;
}
void Histogram1D::copyX(const Histogram1D& h)
{
	if (!_X.unique()) //Either not defined of multiple reference
	{
		_X.reset();
		_X=parray(new std::vector<double>);
	}
	*_X=*(h._X);
	_nxbin=_X->size();
	return;
}
void Histogram1D::setData(const std::vector<double>& y)
{
	if (!_Y.unique()) //Either not defined of multiple reference
		{
			_Y.reset();
			_Y=parray(new std::vector<double>);
		}
	*_Y=y;
	_nybin=_Y->size();
	return;
}
void Histogram1D::setData(const std::vector<double>& y, const std::vector<double>& e)
{
	if (y.size()!=e.size()) throw std::invalid_argument("Histogram1D::setData, Y and E should be of same sizes");

	if (!_Y.unique()) //Either not defined of multiple reference
		{
			_Y.reset();
			_Y=parray(new std::vector<double>);
		}
	*_Y=y;
	_nybin=_Y->size();
	if (!_E.unique()) //Either not defined of multiple reference
		{
			_E.reset();
			_E=parray(new std::vector<double>);
		}
	*_E=e;
	return;
}
void Histogram1D::setData(const parray& y)
{
	_Y=y;
	_nybin=y->size();
}
void Histogram1D::setData(const parray& y, const parray& e)
{
	if (y->size()!=e->size()) throw std::invalid_argument("Histogram1D::setData, Y and E should be of same sizes");
	_Y=y;	
	_E=e;
	_nybin=y->size();
}
int Histogram1D::nxbin() const 
{
	return _nxbin;
}
int Histogram1D::nybin() const
{
  return _nybin;
}
const std::vector<double>& Histogram1D::getX() const 
{
	return *_X;
}

const std::vector<double>& Histogram1D::getY() const 
{
	return *_Y;
}

const std::vector<double>& Histogram1D::getE() const 
{
	return *_E;
}

double Histogram1D::getX(size_t bin) const
{
	if (bin<0 || bin>_X->size()-1) throw std::range_error("Histogram1D::getX, range error");
	return (*_X)[bin];
}	

double Histogram1D::getY(size_t bin) const
{
	if (bin<0 || bin>_Y->size()-1) throw std::range_error("Histogram1D::getY, range error");
	return (*_Y)[bin];
}
double Histogram1D::getE(size_t bin) const
{
	if (bin<0 || bin>_E->size()-1) throw std::range_error("Histogram1D::getE, range error");
	return (*_E)[bin];
}

double* Histogram1D::operator[](size_t bin) const
{
	if (bin<0 || bin>_X->size()-1) throw std::range_error("Histogram1D::operator[], range error");
	double* temp=new double[3];
	*temp=(*_X)[bin];	*(temp+1)=(*_Y)[bin];
	if (isError()) *(temp+2)=(*_E)[bin];
	return temp;
}
bool Histogram1D::isError() const
{
	return _E->size()!=0;	
}

} // namespace DataObjects
} // namespace Mantid
