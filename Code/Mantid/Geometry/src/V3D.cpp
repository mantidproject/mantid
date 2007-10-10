#include <cmath> 
#include <stdexcept>
#include "../inc/V3D.h"
#include <vector>

namespace Mantid
{
namespace Geometry
{
const double precision=1e-7; 


V3D::V3D():_x(0),_y(0),_z(0)
{
}
V3D::V3D(const V3D& v):_x(v._x),_y(v._y),_z(v._z)
{
}
V3D& V3D::operator=(const V3D& v)
   /*!
      Assignment operator
      \param v :: V3D to copy 
      \return this
    */
{
	_x=v._x;
	_y=v._y;
	_z=v._z;
	return *this;
}
V3D::V3D(const double x, const double y, const double z):_x(x),_y(y),_z(z)
{
}
V3D::V3D(double* v):_x(v[0]),_y(v[1]),_z(v[2])
{
}
V3D::~V3D()
{
}

V3D V3D::operator+(const V3D& v) const
{
	V3D out(*this);
	out+=v;
	return out;
}
V3D V3D::operator-(const V3D& v) const
{
	V3D out(*this);
	out-=v;
	return out;
}
V3D V3D::operator*(const V3D& v) const
{
	V3D out(*this);
	out*=v;
	return out;
}
V3D V3D::operator/(const V3D& v) const
{
	V3D out(*this);
	out/=v;
	return out;
}
V3D& V3D::operator+=(const V3D& v) 
{
	_x+=v._x;
	_y+=v._y;
	_z+=v._z;
	return *this;
}
V3D& V3D::operator-=(const V3D& v) 
{
	_x-=v._x;
	_y-=v._y;
	_z-=v._z;
	return *this;
}
V3D& V3D::operator*=(const V3D& v) 
{
	_x*=v._x;
	_y*=v._y;
	_z*=v._z;
	return *this;
}
V3D& V3D::operator/=(const V3D& v) 
{
	_x/=v._x;
	_y/=v._y;
	_z/=v._z;
	return *this;
}
V3D V3D::operator*(const double n) const
{
	V3D out(*this);
	out._x*=n;
	out._y*=n;
	out._z*=n;
	return out;
}
V3D V3D::operator/(const double n) const
{
	V3D out(*this);
	out._x/=n;
	out._y/=n;
	out._z/=n;
	return out;
}
V3D& V3D::operator*=(const double n) 
{
	_x*=n;
	_y*=n;
	_z*=n;
	return *this;
}
V3D& V3D::operator/=(const double n) 
{
	_x/=n;
	_y/=n;
	_z/=n;
	return *this;
}
bool V3D::operator==(const V3D& v) const
   /*!
      Equals operator with tolerance factor
      \param v :: V3D for comparison
      */
{
	if (fabs(_x-v._x)>precision) return false;
	if (fabs(_y-v._y)>precision) return false;
	if (fabs(_z-v._z)>precision) return false;
	return true;
}
bool V3D::operator<(const V3D& v) const
{
	if (_x<v._x) return true;
	if (_y<v._y) return true;
	if (_z<v._z) return true;
	return false;
}
void V3D::operator()(const double x, const double y, const double z)
{
	_x=x;
	_y=y;
	_z=z;
}
double V3D::X() const 
{
	return _x;
}
double V3D::Y() const 
{
	return _y;
}
double V3D::Z() const 
{
	return _z;
}
const double V3D::operator[](const int i) const
{
	if (i==0) return _x;
	if (i==1) return _y;
	if (i==2) return _z;
	if (i<0 || i>2) throw std::runtime_error("V3D::operator[] range error");
}
double& V3D::operator[](const int i) 
{
	if (i==0) return _x;
	if (i==1) return _y;
	if (i==2) return _z;
	if (i<0 || i>2) throw std::runtime_error("V3D::operator[] range error");
}
double V3D::norm() const
{
	return sqrt(_x*_x+_y*_y+_z*_z);
}
double V3D::norm2() const
{
	return (_x*_x+_y*_y+_z*_z);
}
V3D V3D::normalize()
{
	V3D out(*this);
	out/=norm();
	return out;
}
double V3D::scalar_prod(const V3D& v) const
{
	return (_x*v._x+_y*v._y+_z*v._z);
}
V3D V3D::cross_prod(const V3D& v) const
{
	V3D out;
	out._x=_y*v._z-_z*v._y;
	out._y=_z*v._x-_x*v._z;
	out._z=_x*v._y-_y*v._x;
	return out;
}
double V3D::distance(const V3D& v) const
{
	V3D dif(*this);
	dif-=v;
	return dif.norm();
}
void V3D::printSelf(std::ostream& os) const
{
	os << "[" << _x << "," << _y << "," << _z << "]";
	return;
}

std::ostream& operator<<(std::ostream& os, const V3D& v)
{
	v.printSelf(os);
	return os;
}

} // Namespace Geometry
} // Namespace Mantid
