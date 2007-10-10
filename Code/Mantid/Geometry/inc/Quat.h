#ifndef MANTID_QUAT_H_
#define MANTID_QUAT_H_
#include <iostream> 

namespace Mantid
{
namespace Geometry
{
template <typename T> class Quat
{
	/** @class Quat Quat.h Geometry/Quat.h
			@brief Class for quaternions 
		    @version 1.0
		    @author Laurent C Chapon, ISIS RAL
		    @date 10/10/2007
		    
		    Templated class for quaternions. 
		    Quaternions are the 3D generalization of complex numbers
		    Quaternions are used for roations in 3D spaces and  
		    often implemented for computer graphics applications.
		    Quaternion can be written q=W+ai+bj+ck where 
		    w is the scalar part, and a, b, c the 3 imaginary parts.
		    Quaternion multiplication is non-commutative.
		    i*j=-j*i=k
		    j*k=-k*j=i
		    k*i=-i*k=j
		    Rotation of an angle theta around a normalized axis (u,v,w) can be simply
		    written W=cos(theta/2), a=u*sin(theta/2), b=v*sin(theta/2), c=w*sin(theta/2)
		    This class support all arithmetic operations for quaternions
		    @ignore 
		    Copyright � 2007 ???RAL???

		    This file is part of Mantid.

		    Mantid is free software; you can redistribute it and/or modify
		    it under the terms of the GNU General Public License as published by
		    the Free Software Foundation; either version 3 of the License, or
		    (at your ption) any later version.

		    Mantid is distributed in the hope that it will be useful,
		    but WITHOUT ANY WARRANTY; without even the implied warranty of
		    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
		    GNU General Public License for more details.

		    You should have received a copy of the GNU General Public License
		    along with this program.  If not, see <http://www.gnu.org/licenses/>.
		    
		    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
		*/
	public:
		Quat();       
		Quat(const T, const T, const T, const T); 
		Quat(const Quat&);
		Quat(const T&,const V3D<T>&);
		Quat(const M33<T>&);
		~Quat();
		void operator()(const T&, const T&, const T&, const T&);
		void operator()(const T, const V3D<T>&);
		void operator()(const M33<T>&);
		void set(const T&, const T&, const T&, const T&);
		void setAngleAxis(const T,const V3D<T>&);
		void setRotMatrix(const M33<T>&);
		double norm() const;
		double magnitude() const;
		void init();
		Quat normalize() const;
		Quat conjugate() const; 
		Quat inverse() const;
		T* toOpenGLMatrix() const;
		//! Overload operators
		Quat  operator+(const Quat&) const;
		Quat& operator+=(const Quat&);
		Quat  operator-(const Quat&) const;
		Quat& operator-=(const Quat&);
		Quat  operator*(const Quat&) const;
		Quat& operator*=(const Quat&);
		bool   operator==(const Quat&) const;
		const T operator[](int) const;
		T& operator[](int);
		void printSelf(std::ostream&) const;
	private:
		T _w, _a, _b, _c;
};

template<typename T> std::ostream& operator<<(std::ostream&, const Quat<T>&);

template <typename T>
Quat<T>::Quat():_w(static_cast<T>(1)),_a(static_cast<T>(0)),_b(static_cast<T>(0)),_c(static_cast<T>(0))
{
}
template <typename T>
Quat<T>::Quat(const T& _w,const T& _a, const T& _b, const T& _c)
{
	v[0]=_w;v[1]=_a;v[2]=_b;v[3]=_c;
}
template <typename T>
Quat<T>::Quat(const Quat& _q)
{
	v[0]=_q.v[0];v[1]=_q.v[1];v[2]=_q.v[2];v[3]=_q.v[3];
}
template <typename T>
Quat<T>::Quat(const T& _w, NV3D<T>& _v)
{
	v[0]=_w;v[1]=_v[0];v[2]=_v[1];v[3]=_v[2];
}
template <typename T>
Quat<T>::~Quat()
{}
//Overload operators
template <typename T>
Quat<T> Quat<T>::operator+(const Quat& _q)
{
	return Quat(v[0]+_q.v[0],v[1]+_q.v[1],v[2]+_q.v[2],v[4]+_q.v[4]);
}
template <typename T>
Quat<T>& Quat<T>::operator+=(const Quat<T>& _q)
{
	v[0]+=_q.v[0];v[1]+=_q.v[1];v[2]+=_q.v[2];v[3]+=_q.v[3];
	return *this;
}
template <typename T>
Quat<T> Quat<T>::operator-(const Quat<T>& _q)
{
	return Quat(v[0]-_q.v[0],v[1]-_q.v[1],v[2]-_q.v[2],v[3]-_q.v[3]);
}
template <typename T>
Quat<T>& Quat<T>::operator-=(const Quat <T>& _q)
{
	v[0]-=_q.v[0];v[1]-=_q.v[1];v[2]-=_q.v[2];v[3]-=_q.v[3];
	return *this;
}
template <typename T>
Quat<T> Quat<T>::operator*(const Quat<T>& _q)
{
	T w1,a1,b1,c1;
	w1=v[0]*_q.v[0]-v[1]*_q.v[1]-v[2]*_q.v[2]-v[3]*_q.v[3];
	a1=v[0]*_q.v[1]+_q.v[0]*v[1]+v[2]*_q.v[3]-_q.v[2]*v[3];
	b1=v[0]*_q.v[2]+_q.v[0]*v[2]-v[1]*_q.v[3]+v[3]*_q.v[1];
	c1=v[0]*_q.v[3]+_q.v[0]*v[3]+v[1]*_q.v[2]-_q.v[1]*v[2];
	return Quat(w1,a1,b1,c1);
}
template <typename T>
Quat<T>& Quat<T>::operator*=(const Quat<T>& _q)
{
	T w1,a1,b1,c1;
	w1=v[0]*_q.v[0]-v[1]*_q.v[1]-v[2]*_q.v[2]-v[3]*_q.v[3];
	a1=v[0]*_q.v[1]+_q.v[0]*v[1]+v[2]*_q.v[3]-_q.v[2]*v[3];
	b1=v[0]*_q.v[2]+_q.v[0]*v[2]-v[1]*_q.v[3]+v[3]*_q.v[1];
	c1=v[0]*_q.v[3]+_q.v[0]*v[3]+v[1]*_q.v[2]-_q.v[1]*v[2];
	v[0]=w1;v[1]=a1;v[2]=b1;v[3]=c1;
	return (*this);
}
template <typename T>
bool Quat<T>::operator==(const Quat<T>& _q)
{
	return (fabs(v[0]-_q.v[0])<eps && fabs(v[1]-_q.v[1])<eps &&
	        fabs(v[2]-_q.v[2])<eps && fabs(v[3]-_q.v[3])<eps);
} 
template <typename T>
bool Quat<T>::operator!=(const Quat<T>& _q)
{
	return (fabs(v[0]-_q.v[0])>eps || fabs(v[1]-_q.v[1])>eps ||
	        fabs(v[2]-_q.v[2])>eps || fabs(v[3]-_q.v[3])>eps);
} 
template <typename T>
void Quat<T>::Normalize()
{
	T norm=this->GetNorm();
	v[0]/=norm;v[1]/=norm;v[2]/=norm;v[3]/=norm;
}
template <typename T>
void Quat<T>::Conjugate()
{
	v[1]*=-1.0;v[2]*=-1.0;v[3]*=-1.0;
}
template <typename T>
T Quat<T>::GetNorm()
{
	return (v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
}
template <typename T>
T Quat<T>::GetMagnitude()
{
	return sqrt(GetNorm());
}
template <typename T>
void Quat<T>::Inverse()
{
	this->Conjugate();
	this->Normalize();
}
template <typename T>
void Quat<T>::SetAngleAxis(const T& _deg,const NV3D<T>& _axis)
{
	v[0]=cos(_deg*deg2rad/2.0);
	T s=sin(_deg*deg2rad/2.0);
	NV3D<T> temp=_axis;
	temp.Normalize();
	v[0]=s*temp[0];v[1]=s*temp[1];v[2]=s*temp[2];
}
//Member functions
template <typename T> 
void Quat<T>::Init()
{
	v[0]=1;v[1]=v[2]=v[3]=0;
}
template <typename T>
void Quat<T>::ToMatrix(double mat[16])
{
	double aa      = v[1] * v[1];
	double ab      = v[1] * v[2];
	double ac      = v[1] * v[3];
	double aw      = v[1] * v[0];
	double bb      = v[2] * v[2];
	double bc      = v[2] * v[3];
	double bw      = v[2] * v[0];
	double cc      = v[3] * v[3];
	double cw      = v[3] * v[0];
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
template <typename T> 
T& Quat<T>::operator[](int i)
{
	assert(i<3);return v[i];
}
} // Namespace Mantid
} // Namespace Geometry

#endif /*MANTID_QUAT_H_*/
