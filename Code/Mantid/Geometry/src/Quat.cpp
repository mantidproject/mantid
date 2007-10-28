#include "../inc/Quat.h" 
#include "../inc/V3D.h"
#include <cmath>
#include <boost/test/floating_point_comparison.hpp>
#include <stdexcept> 

namespace Mantid
{
namespace Geometry
{

// Use boost float comparison 	
boost::test_tools::close_at_tolerance<double> quat_tol(boost::test_tools::percent_tolerance(1e-6));


Quat::Quat():w(1),a(0),b(0),c(0)
/*! Null Constructor
 * Initialize the quaternion with the identity q=1.0+0i+0j+0k;
 */
{
}
Quat::Quat(const double _w,const double _a, const double _b, const double _c):w(_w),a(_a),b(_b),c(_c)
//! Constructor with values
{
}
 
Quat::Quat(const Quat& _q)
//! Copy constructor
{
	w=_q.w;
	a=_q.a;
	b=_q.b;
	c=_q.c;
}
 
Quat::Quat(const double _deg,const V3D& _axis)
/*! Constructor from an angle and axis.
 * \param _deg :: angle of rotation
 * \param _axis :: axis to rotate about
 * 
 * This construct a  quaternion to represent a rotation
 * of an angle _deg around the _axis. The _axis does not need to be a unit vector
 * */
{
	setAngleAxis(_deg,_axis);
}

void Quat::setAngleAxis(const double _deg, const V3D& _axis)
/*! Constructor from an angle and axis.
 * \param _deg :: angle of rotation
 * \param _axis :: axis to rotate about
 * 
 * This construct a  quaternion to represent a rotation
 * of an angle _deg around the _axis. The _axis does not need to be a unit vector
 * */
{
	double deg2rad=M_PI/180.0;
	w=cos(0.5*_deg*deg2rad);
	double s=sin(0.5*_deg*deg2rad);
	V3D temp(_axis);
	temp.normalize();
	w=s*temp[0];a=s*temp[1];b=s*temp[2];
	return;
}
 
Quat::~Quat()
//! Destructor
{}

void Quat::init() 
/*! Re-initialise a quaternion to identity.
 */
{
	w=1.0;
	a=b=c=0.0;
	return;
}

Quat Quat::operator+(const Quat& _q) const
/*! Quaternion addition operator
 * \param _q :: the quaternion to add
 * \return *this+_q
 */
{
	return Quat(w+_q.w,a+_q.a,b+_q.b,c+_q.c);
}
 
Quat& Quat::operator+=(const Quat& _q)
/*! Quaternion self-addition operator
 * \param _q :: the quaternion to add
 * \return *this+=_q
 */
{
	w+=_q.w;a+=_q.a;b+=_q.b;c+=_q.c;
	return *this;
}
 
Quat Quat::operator-(const Quat& _q) const
/*! Quaternion subtraction operator
 * \param _q :: the quaternion to add
 * \return *this-_q
 */

{
	return Quat(w-_q.w,a-_q.a,b-_q.b,c-_q.c);
}
 
Quat& Quat::operator-=(const Quat& _q)
/*! Quaternion self-substraction operator
 * \param _q :: the quaternion to add
 * \return *this-=_q
 */
{
	w-=_q.w;
	a-=_q.a;
	b-=_q.b;
	c-=_q.c;
	return *this;
}
 
Quat Quat::operator*(const Quat& _q) const
/*! Quaternion multiplication operator
 * \param _q :: the quaternion to multiply
 * \return *this*_q
 * 
 *  Quaternion multiplication is non commutative
 *  in the same way multiplication of rotation matrices 
 *  isn't.
 */
{
	double w1,a1,b1,c1;
	w1=w*_q.w-a*_q.a-b*_q.b-c*_q.c;
	a1=w*_q.a+_q.w*a+b*_q.c-_q.b*c;
	b1=w*_q.b+_q.w*b-a*_q.c+c*_q.a;
	c1=w*_q.c+_q.w*c+a*_q.b-_q.a*b;
	return Quat(w1,a1,b1,c1);
}
 
Quat& Quat::operator*=(const Quat& _q) 
/*! Quaternion self-multiplication operator
 * \param _q :: the quaternion to multiply
 * \return *this*=_q
 */
{
	double w1,a1,b1,c1;
	w1=w*_q.w-a*_q.a-b*_q.b-c*_q.c;
	a1=w*_q.a+_q.w*a+b*_q.c-_q.b*c;
	b1=w*_q.b+_q.w*b-a*_q.c+c*_q.a;
	c1=w*_q.c+_q.w*c+a*_q.b-_q.a*b;
	w=w1;a=a1;b=b1;c=c1;
	return (*this);
}
 
bool Quat::operator==(const Quat& q) const 
/*! Quaternion equal operator
 * \param _q :: the quaternion to compare
 * 
 * Compare two quaternions at 1e-6%tolerance.
 * Use boost close_at_tolerance method
 */
{
	return (quat_tol(w,q.w) && quat_tol(a,q.a) && quat_tol(b,q.b) && quat_tol(c,q.c));
} 
 
bool Quat::operator!=(const Quat& _q) const
{
/*! Quaternion non-equal operator
 * \param _q :: the quaternion to compare
 * 
 * Compare two quaternions at 1e-6%tolerance.
 *  Use boost close_at_tolerance method
 */
	return (!operator==(_q));
} 
 
void Quat::normalize()
/*! Quaternion normalization
 * 
 * Divide all elements by the quaternion norm
 */
{
	double overnorm=1.0/norm();
	w*=overnorm;
	a*=overnorm;
	b*=overnorm;
	c*=overnorm;
	return;
}
 
void Quat::conjugate()
/*! Quaternion complex conjugate
 * 
 *  Reverse the sign of the 3 imaginary components of the 
 *  quaternion
 */
{
	a*=-1.0;
	b*=-1.0;
	c*=-1.0;
	return;
}
 
double Quat::norm() const
/*! Quaternion norm (length)
 *  
 */
{
	return sqrt(norm2());
}

double Quat::norm2() const
/*! Quaternion norm squared 
 *    
 */
{
	return (w*w+a*a+b*b+c*c);
}
 
void Quat::inverse()  
/*! Inverse a quaternion
 *  
 */
{
	conjugate();
	normalize();
	return;
}
 
void Quat::GLMatrix(double mat[16])  
/*! 
 */
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
	return;
}
  
const double& Quat::operator[](const int Index) const
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

void Quat::printSelf(std::ostream& os) const
{
	os << "[" << w << "," << a << "," << b << "," << c << "]";
	return;

}

std::ostream& operator<<(std::ostream& os,const Quat& q)
{
	q.printSelf(os);
	return os;
}

} // Namespace Geometry

} // Namespce Mantid