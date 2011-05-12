#include "MantidGeometry/Quat.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Tolerance.h"
#include <cmath>
#include <stdexcept>
#include <cstdlib>
#include <float.h>


namespace Mantid
{
namespace Geometry
{

  Kernel::Logger& Quat::quatG_log = Kernel::Logger::get("Geometry:Quat");
/** Null Constructor
 * Initialize the quaternion with the identity q=1.0+0i+0j+0k;
 */
Quat::Quat():w(1),a(0),b(0),c(0)
{
}


/**
 * Construct a Quat between two vectors; 
 * The angle between them is defined differently from usual if vectors are not unit or the same length vectors, so quat would be not consistent
 *
 * v=(src+des)/�src+des�
 * w=v.des
 * (a,b,c)=(v x des)
 * @param src :: the source position
 * @param des :: the destination position
 */
Quat::Quat(const V3D& src,const V3D& des)
{
 
  V3D v = (src+des);
  v.normalize();
  
  V3D cross  =v.cross_prod(des);
 
  if (cross.nullVector())
  {
    w = 1.;
    a = b = c = 0.;
  }
  else
  {
    w = v.scalar_prod(des);
	
    a = cross[0];
    b = cross[1];
    c = cross[2];

	double norm = a*a+b*b+c*c+w*w;
	if(fabs(norm-1)>FLT_EPSILON){
		norm = sqrt(norm);
		w/=norm;
		a/=norm;
		b/=norm;
		c/=norm;
	}
  }
}
Quat::Quat(const Geometry::MantidMat &RotMat)
{
	this->setQuat(RotMat);
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

/** Constructor from an angle and axis.
 * This construct a  quaternion to represent a rotation
 * of an angle _deg around the _axis. The _axis does not need to be a unit vector
 *
 * @param _deg :: angle of rotation
 * @param _axis :: axis to rotate about
 * */
Quat::Quat(const double _deg,const V3D& _axis)
{
  setAngleAxis(_deg,_axis);
}

/**
 * Construct a Quaternion that performs a reference frame rotation.
 *  Specify the X,Y,Z vectors of the rotated reference frame, assuming that
 *  the initial X,Y,Z vectors are aligned as expected: X=(1,0,0), Y=(0,1,0), Z=(0,0,1).
 *  The resuting quaternion rotates XYZ axes onto the provided rX, rY, rZ.
 *
 * @param rX :: rotated X reference axis; unit vector.
 * @param rY :: rotated Y reference axis; unit vector.
 * @param rZ :: rotated Z reference axis; unit vector.
 */
Quat::Quat(const V3D& rX, const V3D& rY, const V3D& rZ)
{
  //Call the operator to do the setting
  this->operator()(rX, rY, rZ);
}

/** Assignment Operator
 * @param q :: the Quat to copy
 * @returns a pointer to this
 */
Quat& Quat::operator=(const Quat& q)
{
	if (this!=&q)
	{
		w=q.w;
		a=q.a;
		b=q.b;
		c=q.c;
	}
	return *this;
}

/** Sets the quat values from four doubles
 * @param ww :: the value for w
 * @param aa :: the value for a
 * @param bb :: the value for b
 * @param cc :: the value for c
 */
void Quat::set(const double ww, const double aa, const double bb, const double cc)
{
	w=ww;
	a=aa;
	b=bb;
	c=cc;
	return;
}

/** Constructor from an angle and axis.
 * @param _deg :: angle of rotation
 * @param _axis :: axis to rotate about
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

bool Quat::isNull(const double tolerance) const
{
  using namespace std;
	double pw=fabs(w)-1;
	return (fabs(pw)<tolerance);
}

///Extracts the angle of roatation and axis
/// @param _deg :: the angle of rotation
/// @param _ax0 :: The first component of the axis
/// @param _ax1 :: The second component of the axis
/// @param _ax2 :: The third component of the axis
void Quat::getAngleAxis(double& _deg,double& _ax0, double& _ax1, double& _ax2) const
{
	// If it represents a rotation of 0(2\pi), get an angle of 0 and axis (0,0,1)
	if (isNull(1e-5))
	{
		_deg=0;
		_ax0=0;
		_ax1=0;
		_ax2=1.0;
		return;
	}
	// Semi-angle in radians
	_deg=acos(w);
	// Prefactor for the axis part
	double s=sin(_deg);
	// Angle in degrees
	_deg*=360.0/M_PI;
	_ax0=a/s;_ax1=b/s;_ax2=c/s;
	return;
}

/** Set the rotation (both don't change rotation axis). This method has an error
 * @param deg :: angle of rotation
 */
void Quat::setRotation(const double deg)
{
	double deg2rad = M_PI/180.0;
	w = cos(0.5*deg*deg2rad);
}

/** Sets the quat values from four doubles
 * @param ww :: the value for w
 * @param aa :: the value for a
 * @param bb :: the value for b
 * @param cc :: the value for c
 */
void Quat::operator()(const double ww, const double aa, const double bb, const double cc)
{
	this->set(ww,aa,bb,cc);
}

/** Sets the quat values from an angle and a vector
 * @param angle :: the numbers of degrees
 * @param axis :: the axis of rotation
 */
void Quat::operator()(const double angle, const V3D& axis)
{
	this->setAngleAxis(angle,axis);
}

/** Sets the quat values from another Quat
 * @param q :: the quat to copy
 */
void Quat::operator()(const Quat& q)
{
	w=q.w;
	a=q.a;
	b=q.b;
	c=q.c;
	return;
}

/**
 * Set a Quaternion that performs a reference frame rotation.
 *  Specify the X,Y,Z vectors of the rotated reference frame, assuming that
 *  the initial X,Y,Z vectors are aligned as expected: X=(1,0,0), Y=(0,1,0), Z=(0,0,1).
 *  The resuting quaternion rotates XYZ axes onto the provided rX, rY, rZ.
 *
 * @param rX :: rotated X reference axis; unit vector.
 * @param rY :: rotated Y reference axis; unit vector.
 * @param rZ :: rotated Z reference axis; unit vector.
 */
void Quat::operator()(const V3D& rX, const V3D& rY, const V3D& rZ)
{
  //The quaternion will combine two quaternions.
  (void) rZ; //Avoid compiler warning

  //These are the original axes
  V3D oX = V3D(1.,0.,0.);
  V3D oY = V3D(0.,1.,0.);
  V3D oZ = V3D(0.,0.,1.);

  //Axis that rotates X
  V3D ax1 = oX.cross_prod(rX);
  //Rotation angle from oX to rX
  double angle1 = oX.angle(rX);

  //Create the first quaternion
  Quat Q1(angle1 * 180.0/M_PI, ax1);

  //Now we rotate the original Y using Q1
  V3D roY = oY;
  Q1.rotate(roY);
  //Find the axis that rotates oYr onto rY
  V3D ax2 = roY.cross_prod(rY);
  double angle2 = roY.angle(rY);
  double sign = 1.0;
  if (ax2.scalar_prod(rX) < 0) { sign = -1.0; };
  Quat Q2(angle2 * 180.0/M_PI, ax2);

  //Final = those two rotations in succession; Q1 is done first.
  Quat final = Q2 * Q1;

  //Set it
  this->operator()(final);

 /*
  std::cout << "Angle1 is: " << angle1 << "; axis " << ax1 << " ... ";
  std::cout << "Q1 is: " << Q1 << "\n";
  std::cout << "Angle2 is: " << angle2 << "; axis " << ax2 << " ... ";
  std::cout << "Q2 is: " << Q2 << "\n";
  std::cout << "Final is: " << final << "\n";
 */
}

//! Destructor
Quat::~Quat()
{}


/** Re-initialise a quaternion to identity.
 */
void Quat::init()
{
	w=1.0;
	a=b=c=0.0;
	return;
}

/** Quaternion addition operator
 * @param _q :: the quaternion to add
 * @return *this+_q
 */
Quat Quat::operator+(const Quat& _q) const
{
	return Quat(w+_q.w,a+_q.a,b+_q.b,c+_q.c);
}

/** Quaternion self-addition operator
 * @param _q :: the quaternion to add
 * @return *this+=_q
 */
Quat& Quat::operator+=(const Quat& _q)
{
	w+=_q.w;a+=_q.a;b+=_q.b;c+=_q.c;
	return *this;
}

/** Quaternion subtraction operator
 * @param _q :: the quaternion to add
 * @return *this-_q
 */
Quat Quat::operator-(const Quat& _q) const
{
	return Quat(w-_q.w,a-_q.a,b-_q.b,c-_q.c);
}

/** Quaternion self-substraction operator
 * @param _q :: the quaternion to add
 * @return *this-=_q
 */
Quat& Quat::operator-=(const Quat& _q)
{
	w-=_q.w;
	a-=_q.a;
	b-=_q.b;
	c-=_q.c;
	return *this;
}

/** Quaternion multiplication operator
 * @param _q :: the quaternion to multiply
 * @return *this*_q
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

/** Quaternion self-multiplication operator
 * @param _q :: the quaternion to multiply
 * @return *this*=_q
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

/** Quaternion equal operator
 * @param q :: the quaternion to compare
 *
 * Compare two quaternions at 1e-6%tolerance.
 * Use boost close_at_tolerance method
 * @return true if equal
 */
bool Quat::operator==(const Quat& q) const
{
  using namespace std;
  return (fabs(w-q.w)>Tolerance ||
          fabs(a-q.a)>Tolerance ||
          fabs(b-q.b)>Tolerance ||
          fabs(c-q.c)>Tolerance)  ?
          false : true;

	//return (quat_tol(w,q.w) && quat_tol(a,q.a) && quat_tol(b,q.b) && quat_tol(c,q.c));
}

/** Quaternion non-equal operator
 * @param _q :: the quaternion to compare
 *
 * Compare two quaternions at 1e-6%tolerance.
 *  Use boost close_at_tolerance method
 * @return true if not equal
 */
bool Quat::operator!=(const Quat& _q) const
{
	return (!operator==(_q));
}

/** Quaternion normalization
 *
 * Divide all elements by the quaternion norm
 */
void Quat::normalize()
{
	double overnorm;
	if(len2()==0)
		overnorm=1.0;
	else
		overnorm=1.0/len();
	w*=overnorm;
	a*=overnorm;
	b*=overnorm;
	c*=overnorm;
	return;
}

/** Quaternion complex conjugate
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

/** Quaternion length
 * @return the length
 */
double Quat::len() const
{
	return sqrt(len2());
}

/** Quaternion norm (length squared)
 * @return the length squared
 */
double Quat::len2() const
{
	return (w*w+a*a+b*b+c*c);
}

/** Inverse a quaternion
 *
 */
void Quat::inverse()
{
	conjugate();
	double overnorm = len2();
	if(overnorm==0)
		overnorm=1.0;
	else
		overnorm=1.0/overnorm;
	w*=overnorm;
	a*=overnorm;
	b*=overnorm;
	c*=overnorm;
}

/** 	Rotate a vector.
 *  @param v :: the vector to be rotated
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

/** Convert quaternion rotation to an OpenGL matrix [4x4] matrix
 * stored as an linear array of 16 double
 * The function glRotated must be called
 * @param mat :: The output matrix
 */
void Quat::GLMatrix(double* mat) const
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
	*mat  = 1.0 - 2.0 * ( bb + cc );++mat;
	*mat  =     2.0 * ( ab + cw );++mat;
	*mat  =     2.0 * ( ac - bw );++mat;
	*mat  =0;++mat;
	*mat  =     2.0 * ( ab - cw );++mat;
	*mat  = 1.0 - 2.0 * ( aa + cc );++mat;
	*mat  =     2.0 * ( bc + aw );++mat;
	*mat  = 0;++mat;
	*mat  =     2.0 * ( ac + bw );mat++;
	*mat  =     2.0 * ( bc - aw );mat++;
	*mat = 1.0 - 2.0 * ( aa + bb );mat++;
	for (int i=0;i<4;++i)
	{
		*mat=0;mat++;
	}
	*mat=1.0;
	return;
}
//
std::vector<double> 
Quat::getRotation(bool check_normalisation,bool throw_on_errors)const
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
	if(check_normalisation){
		double normSq=aa+bb+cc+w*w;
		if(fabs(normSq-1)>FLT_EPSILON){
			if(throw_on_errors){
				quatG_log.error()<<" A non-unit quaternion used to obtain a rotation matrix; need to notmalize it first\n";
				throw(std::invalid_argument("Attempt to use non-normalized quaternion to define rotation matrix; need to notmalize it first"));
			}else{
				quatG_log.information()<<" Warning; a non-unit quaternion used to obtain the rotation matrix; using normalized quat\n";
				aa/=normSq;
				ab/=normSq;
				ac/=normSq;
				aw/=normSq;
				bb/=normSq;
				bc/=normSq;
				bw/=normSq;
				cc/=normSq;
				cw/=normSq;
			}
		}
	}
	std::vector<double> out(9);

	out[0]= (1.0 - 2.0 * ( bb + cc ));
	out[1]= 2.0 * ( ab + cw );
    out[2]= 2.0 * ( ac - bw );
	
	out[3]= 2.0 * ( ab - cw );
	out[4]= (1.0 - 2.0 * ( aa + cc ));
	out[5]= 2.0 * ( bc + aw );
	
	out[6]=2.0 * ( ac + bw );
	out[7]=2.0 * ( bc - aw );
	out[8]=(1.0 - 2.0 * ( aa + bb ));
	return out;
}

/**
 * Converts the GL Matrix into Quat
 */
void Quat::setQuat(double mat[16])
{
	double tr,s,q[4];
	int i,j,k;
	int nxt[3]={1,2,0};
	tr=mat[0]+mat[5]+mat[10];
	if(tr>0.0)
	{
		s=sqrt(tr+1.0);
		w=s/2.0;
		s=0.5/s;
		a=(mat[6]-mat[9])*s;
		b=(mat[8]-mat[2])*s;
		c=(mat[1]-mat[4])*s;
	}else{
		i=0;
		if(mat[5]>mat[0])i=1;
		if(mat[10]>mat[i*5])i=2;
		j=nxt[i];
		k=nxt[j];
		s=sqrt(mat[i*5]-(mat[j*5]+mat[k*5])+1.0);
		q[i]=s*0.5;
		if(s!=0.0)s=0.5/s;
		q[3]=(mat[j*4+k]-mat[k*4+j])*s;
		q[j]=(mat[i*4+j]+mat[j*4+i])*s;
		q[k]=(mat[i*4+k]+mat[k*4+i])*s;
		a=q[0];
		b=q[1];
		c=q[2];
		w=q[3];
	}
}
//
void 
Quat::setQuat(const Geometry::MantidMat &rMat)
{
	double MAT[16];
	for(int i=0;i<16;i++)MAT[i]=0;
	
	MAT[0]= rMat[0][0];
	MAT[1]= rMat[0][1];
	MAT[2]= rMat[0][2];

	MAT[4]= rMat[1][0];
	MAT[5]= rMat[1][1];
	MAT[6]= rMat[1][2];

	MAT[8] = rMat[2][0];
	MAT[9] = rMat[2][1];
	MAT[10]= rMat[2][2];

	MAT[15]= 1;
	this->setQuat(MAT);





}
/** Bracket operator overload
 * returns the internal representation values based on an index
 * @param Index :: the index of the value required 0=w, 1=a, 2=b, 3=c
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
 * @param Index :: the index of the value required 0=w, 1=a, 2=b, 3=c
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
 * @param os :: the stream to output to
 */
void Quat::printSelf(std::ostream& os) const
{
	os << "[" << w << "," << a << "," << b << "," << c << "]";
	return;

}

/**  Read data from a stream in the format returned by printSelf ("[w,a,b,c]").
 *   @param IX :: Input Stream
 *   @throw std::runtime_error if the input is of wrong format
*/
void Quat::readPrinted(std::istream& IX)
{
    std::string in;
    std::getline(IX,in);
    size_t i = in.find_first_of('[');
    if (i == std::string::npos) throw std::runtime_error("Wrong format for Quat input: "+in);
    size_t j = in.find_last_of(']');
    if (j == std::string::npos || j < i + 8) throw std::runtime_error("Wrong format for Quat input: "+in);

    size_t c1 = in.find_first_of(',');
    size_t c2 = in.find_first_of(',',c1+1);
    size_t c3 = in.find_first_of(',',c2+1);
    if (c1 == std::string::npos || c2 == std::string::npos || c3 == std::string::npos)
        throw std::runtime_error("Wrong format for Quat input: ["+in+"]");

    w = atof(in.substr(i+1,c1-i-1).c_str());
    a = atof(in.substr(c1+1,c2-c1-1).c_str());
    b = atof(in.substr(c2+1,c3-c2-1).c_str());
    c = atof(in.substr(c3+1,j-c3-1).c_str());

    return;
}

/** Prints a string representation
 * @param os :: the stream to output to
 * @param q :: the quat to output
 * @returns the stream
 */
std::ostream& operator<<(std::ostream& os,const Quat& q)
{
	q.printSelf(os);
	return os;
}

/**  Reads in a quat from an input stream
 *   @param ins :: The input stream
 *   @param q :: The quat
 */
std::istream& operator>>(std::istream& ins,Quat& q)
{
    q.readPrinted(ins);
    return ins;
}

void Quat::rotateBB(double& xmin, double& ymin, double& zmin, double& xmax, double& ymax, double& zmax) const
{
	// Defensive
	if (xmin>xmax) std::swap(xmin,xmax);
	if (ymin>ymax) std::swap(ymin,ymax);
	if (zmin>zmax) std::swap(zmin,zmax);
	// Get the min and max of the cube, and remove centring offset
	Mantid::Geometry::V3D minT(xmin,ymin,zmin), maxT(xmax,ymax,zmax);
	// Get the rotation matrix
	double rotMatr[16];
	GLMatrix(&rotMatr[0]);
	// Now calculate new min and max depending on the sign of matrix components
	// Much faster than creating 8 points and rotate them. The new min (max)
	// can only be obtained by summing the smallest (largest) components
	//
	Mantid::Geometry::V3D minV, maxV;
	// Looping on rows of matrix
	int index;
	for (int i=0;i<3;i++)
	{
		for (int j=0;j<3;j++)
		{
			index=j+i*4; // The OpenGL matrix is linear and represent a 4x4 matrix but only the 3x3 upper-left inner part
			// contains the rotation
			minV[j]+=(rotMatr[index]>0)?rotMatr[index]*minT[i]:rotMatr[index]*maxT[i];
			maxV[j]+=(rotMatr[index]>0)?rotMatr[index]*maxT[i]:rotMatr[index]*minT[i];
		}
	}
	// Adjust value.
	xmin=minV[0]; ymin=minV[1]; zmin=minV[2];
	xmax=maxV[0]; ymax=maxV[1]; zmax=maxV[2];
	return;
}

} // Namespace Geometry

} // Namespce Mantid
