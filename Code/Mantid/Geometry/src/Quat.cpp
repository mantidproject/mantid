#include "Quat.h" 
#include <cmath>
#include <boost/test/floating_point_comparison.hpp>


namespace Mantid
{
namespace Geometry
{

boost::test_tools::close_at_tolerance<double> tol(1e-6);


Quat::Quat():w(0),a(0),b(0),c(0)
{
}
Quat::Quat(const double _w,const double _a, const double _b, const double _c):w(_w),a(_a),b(_b),c(_c)
{
}
 
Quat::Quat(const Quat& _q)
{
	w=_q.w;a=_q.a;b=_q.b;c=_q.c;
}
 
Quat::Quat(const double _deg,const V3D& _axis)
{
	setAngleAxis(_deg,_axis);
}

void Quat::setAngleAxis(const double _deg, const V3D& _axis)
{
	double deg2rad=M_PI/180.0;
	w=cos(0.5*_deg*deg2rad);
	double s=sin(0.5*_deg*deg2rad);
	V3D temp(_axis);
	temp.normalize();
	w=s*temp[0];a=s*temp[1];b=s*temp[2];
}
 
Quat::~Quat()
{}

void Quat::init() 
{
	w=1.0;a=b=c=0.0;
}

Quat Quat::operator+(const Quat& _q) const
{
	return Quat(w+_q.w,a+_q.a,b+_q.b,c+_q.c);
}
 
Quat& Quat::operator+=(const Quat& _q)
{
	w+=_q.w;a+=_q.a;b+=_q.b;c+=_q.c;
	return *this;
}
 
Quat Quat::operator-(const Quat& _q) const
{
	return Quat(w-_q.w,a-_q.a,b-_q.b,c-_q.c);
}
 
Quat& Quat::operator-=(const Quat& _q)
{
	w-=_q.w;a-=_q.a;b-=_q.b;c-=_q.c;
	return *this;
}
 
Quat Quat::operator*(const Quat& _q) const
{
	T w1,a1,b1,c1;
	w1=w*_q.w-a*_q.a-b*_q.b-c*_q.c;
	a1=w*_q.a+_q.w*a+b*_q.c-_q.b*c;
	b1=w*_q.b+_q.w*b-a*_q.c+c*_q.a;
	c1=w*_q.c+_q.w*c+a*_q.b-_q.a*b;
	return Quat(w1,a1,b1,c1);
}
 
Quat& Quat::operator*=(const Quat& _q) 
{
	T w1,a1,b1,c1;
	w1=w*_q.w-a*_q.a-b*_q.b-c*_q.c;
	a1=w*_q.a+_q.w*a+b*_q.c-_q.b*c;
	b1=w*_q.b+_q.w*b-a*_q.c+c*_q.a;
	c1=w*_q.c+_q.w*c+a*_q.b-_q.a*b;
	w=w1;a=a1;b=b1;c=c1;
	return (*this);
}
 
bool Quat::operator==(const Quat& q) const 
{
	return (tol(w,q.w) && tol(a,q.a) && tol(b,q.b) && tol(c,q.c));
} 
 
bool Quat::operator!=(const Quat& _q) const
{
	return (!operator==(_q));
} 
 
void Quat::normalize()
{
	double overnorm=1.0/norm();
	w*=overnorm;
	a*=overnorm;
	b*=overnorm;
	c*=overnorm;
}
 
void Quat::conjugate()
{
	a*=-1.0;
	b*=-1.0;
	c*=-1.0;
}
 
double Quat::norm() const
{
	return sqrt(norm2());
}

double Quat::norm2() const
{
	return (w*w+a*a+b*b+c*c);
}
 
void Quat::inverse() const 
{
	conjugate();
	normalize();
}
 
void Quat::GLMatrix(double mat[16])  
{
	double aa      = a * a;
	double ab      = a * b;
	double ac      = a * c;
	double aw      = a * w;
	double bb      = b * b;
	double bc      = b * c;
	double bw      = b * w;
	double cc      = c * c;
	double cw      = c * w;
	mat[0]  = 1.0 - 2.0 * ( bb + cc );
	mat[4]  =     2.0 * ( ab - cw );
	mat[8]  =     2.0 * ( ac + bw );
	mat[1]  =     2.0 * ( ab + cw );
	mat[5]  = 1.0 - 2.0 * ( aa + cc );
	mat[9]  =     2.0 * ( bc - aw );
	mat[2]  =     2.0 * ( ac - bw );
	mat[6]  =     2.0 * ( bc + aw );
	mat[10] = 1.0 - 2.0 * ( aa + bb );
	mat[12]  = mat[13] = mat[14] = mat[3] = mat[7] = mat[11] = 0.0;
	mat[15] = 1.0;
}
  
double& Quat::operator[](const int Index) const
{
	switch (Index)
	    {
	    case 0: return w;
	    case 1: return a;
	    case 2: return b;
	    case 3: return c;
	    default:
	      throw std::runtime_error("Quat::operator[] range error");
	}
}

double& Quat::operator[](const int Index) 
{
	switch (Index)
	    {
	    case 0: return w;
	    case 1: return a;
	    case 2: return b;
	    case 3: return c;
	    default:
	      throw std::runtime_error("Quat::operator[] range error");
	}
}

} // Namespace Geometry

} // Namespce Mantid