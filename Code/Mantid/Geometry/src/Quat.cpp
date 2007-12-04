#include "Quat.h" 
#include "V3D.h"
#include <cmath>
#include <boost/test/floating_point_comparison.hpp>
#include <stdexcept> 


namespace Mantid
{
namespace Geometry
{

/// Use boost float comparison 	
boost::test_tools::close_at_tolerance<double> quat_tol(boost::test_tools::percent_tolerance(1e-6));


/*! Null Constructor
 * Initialize the quaternion with the identity q=1.0+0i+0j+0k;
 */
Quat::Quat():w(1),a(0),b(0),c(0)
{
}

//! Constructor with values
Quat::Quat(const double _w,const double _a, const double _b, const double _c):w(_w),a(_a),b(_b),c(_c)
{
}

//! Copy constructor 
Quat::Quat(const Quat& _q)
{
	w=_q.w;
	a=_q.a;
	b=_q.b;
	c=_q.c;
}
 
/*! Constructor from an angle and axis.
 * This construct a  quaternion to represent a rotation
 * of an angle _deg around the _axis. The _axis does not need to be a unit vector
 *
 * \param _deg :: angle of rotation
 * \param _axis :: axis to rotate about
 * */
Quat::Quat(const double _deg,const V3D& _axis)
{
	setAngleAxis(_deg,_axis);
}

/** Assignment Operator
 * @param q the Quat to copy
 * @returns a pointer to this
 */
Quat& Quat::operator=(const Quat& q)
{
	w=q.w;
	a=q.a;
	b=q.b;
	c=q.c;
	return *this;
}

/** Sets the quat values from four doubles
 * @param ww the value for w
 * @param aa the value for a
 * @param bb the value for b
 * @param cc the value for c
 */
void Quat::set(const double ww, const double aa, const double bb, const double cc)
{
	w=ww;
	a=aa;
	b=bb;
	c=cc;
	return;
}

/*! Constructor from an angle and axis.
 * \param _deg :: angle of rotation
 * \param _axis :: axis to rotate about
 * 
 * This construct a  quaternion to represent a rotation
 * of an angle _deg around the _axis. The _axis does not need to be a unit vector
 * */
void Quat::setAngleAxis(const double _deg, const V3D& _axis)
{
	double deg2rad=M_PI/180.0;
	w=cos(0.5*_deg*deg2rad);
	double s=sin(0.5*_deg*deg2rad);
	V3D temp(_axis);
	temp.normalize();
	a=s*temp[0];
	b=s*temp[1];
	c=s*temp[2];	
	return;
}

/** Sets the quat values from four doubles
 * @param ww the value for w
 * @param aa the value for a
 * @param bb the value for b
 * @param cc the value for c
 */
void Quat::operator()(const double ww, const double aa, const double bb, const double cc)
{
	this->set(ww,aa,bb,cc);
}

/** Sets the quat values from an angle and a vector
 * @param angle the numbers of degrees
 * @param axis the axis of rotation
 */
void Quat::operator()(const double angle, const V3D& axis)
{
	this->setAngleAxis(angle,axis);
} 

//! Destructor
Quat::~Quat()
{}


/*! Re-initialise a quaternion to identity.
 */
void Quat::init() 
{
	w=1.0;
	a=b=c=0.0;
	return;
}

/*! Quaternion addition operator
 * \param _q :: the quaternion to add
 * \return *this+_q
 */
Quat Quat::operator+(const Quat& _q) const
{
	return Quat(w+_q.w,a+_q.a,b+_q.b,c+_q.c);
}
 
/*! Quaternion self-addition operator
 * \param _q :: the quaternion to add
 * \return *this+=_q
 */
Quat& Quat::operator+=(const Quat& _q)
{
	w+=_q.w;a+=_q.a;b+=_q.b;c+=_q.c;
	return *this;
}
 
/*! Quaternion subtraction operator
 * \param _q :: the quaternion to add
 * \return *this-_q
 */
Quat Quat::operator-(const Quat& _q) const
{
	return Quat(w-_q.w,a-_q.a,b-_q.b,c-_q.c);
}
 
/*! Quaternion self-substraction operator
 * \param _q :: the quaternion to add
 * \return *this-=_q
 */
Quat& Quat::operator-=(const Quat& _q)
{
	w-=_q.w;
	a-=_q.a;
	b-=_q.b;
	c-=_q.c;
	return *this;
}
 
/*! Quaternion multiplication operator
 * \param _q :: the quaternion to multiply
 * \return *this*_q
 * 
 *  Quaternion multiplication is non commutative
 *  in the same way multiplication of rotation matrices 
 *  isn't.
 */
Quat Quat::operator*(const Quat& _q) const
{
	double w1,a1,b1,c1;
	w1=w*_q.w-a*_q.a-b*_q.b-c*_q.c;
	a1=w*_q.a+_q.w*a+b*_q.c-_q.b*c;
	b1=w*_q.b+_q.w*b-a*_q.c+c*_q.a;
	c1=w*_q.c+_q.w*c+a*_q.b-_q.a*b;
	return Quat(w1,a1,b1,c1);
}
 
/*! Quaternion self-multiplication operator
 * \param _q :: the quaternion to multiply
 * \return *this*=_q
 */
Quat& Quat::operator*=(const Quat& _q) 
{
	double w1,a1,b1,c1;
	w1=w*_q.w-a*_q.a-b*_q.b-c*_q.c;
	a1=w*_q.a+_q.w*a+b*_q.c-_q.b*c;
	b1=w*_q.b+_q.w*b-a*_q.c+c*_q.a;
	c1=w*_q.c+_q.w*c+a*_q.b-_q.a*b;
	w=w1;a=a1;b=b1;c=c1;
	return (*this);
}
 
/*! Quaternion equal operator
 * \param q :: the quaternion to compare
 * 
 * Compare two quaternions at 1e-6%tolerance.
 * Use boost close_at_tolerance method
 */
bool Quat::operator==(const Quat& q) const 
{
	return (quat_tol(w,q.w) && quat_tol(a,q.a) && quat_tol(b,q.b) && quat_tol(c,q.c));
} 
 
/*! Quaternion non-equal operator
 * \param _q :: the quaternion to compare
 * 
 * Compare two quaternions at 1e-6%tolerance.
 *  Use boost close_at_tolerance method
 */
bool Quat::operator!=(const Quat& _q) const
{
	return (!operator==(_q));
} 
 
/*! Quaternion normalization
 * 
 * Divide all elements by the quaternion norm
 */
void Quat::normalize()
{
	double overnorm=1.0/len2();
	w*=overnorm;
	a*=overnorm;
	b*=overnorm;
	c*=overnorm;
	return;
}
 
/*! Quaternion complex conjugate
 * 
 *  Reverse the sign of the 3 imaginary components of the 
 *  quaternion
 */
void Quat::conjugate()
{
	a*=-1.0;
	b*=-1.0;
	c*=-1.0;
	return;
}
 
/*! Quaternion length
 *  
 */
double Quat::len() const
{
	return sqrt(len2());
}

/*! Quaternion norm (length squared) 
 *    
 */
double Quat::len2() const
{
	return (w*w+a*a+b*b+c*c);
}
 
/*! Inverse a quaternion
 *  
 */
void Quat::inverse()  
{
	conjugate();
	normalize();
	return;
}

/*! 	Rotate a vector. 
 *  \param v :: the vector to be rotated
 *  
 *   The quaternion needs to be normalized beforehand to
 *   represent a rotation. If q is thequaternion, the rotation
 *   is represented by q.v.q-1 where q-1 is the inverse of 
 *   v. 
 */
void Quat::rotate(V3D& v) const
 {
 	Quat qinvert(*this);
 	qinvert.inverse();
 	Quat pos(0.0,v[0],v[1],v[2]);
 	pos*=qinvert;
 	pos=(*this)*pos;
 	v[0]=pos[1];
 	v[1]=pos[2];
 	v[2]=pos[3];
 }

/*! Convert quaternion rotation to an OpenGL matrix [4x4] matrix 
 * stored as an linear array of 16 double
 * The function glRotated must be called
 * param mat The output matrix
 */
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
	return;
}
  
/** Bracket operator overload
 * returns the internal representation values based on an index
 * @param Index the index of the value required 0=w, 1=a, 2=b, 3=c
 * @returns a double of the value requested
 */
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

/** Bracket operator overload
 * returns the internal representation values based on an index
 * @param Index the index of the value required 0=w, 1=a, 2=b, 3=c
 * @returns a double of the value requested
 */
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

/** Prints a string representation of itself
 * @param os the stream to output to
 */
void Quat::printSelf(std::ostream& os) const
{
	os << "[" << w << "," << a << "," << b << "," << c << "]";
	return;

}

/** Prints a string representation
 * @param os the stream to output to
 * @param q the quat to output
 * @returns the stream
 */
std::ostream& operator<<(std::ostream& os,const Quat& q)
{
	q.printSelf(os);
	return os;
}

} // Namespace Geometry

} // Namespce Mantid
