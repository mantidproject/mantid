#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/point3D.h"
namespace Mantid{
       namespace MDDataObjects{
///
point3D::point3D(void)
{
}

point3D::~point3D()
{
}

point3D::point3D(double x, double y, double z)
{
	this->position.x = x;
	this->position.y = y;
	this->position.z = z;
}

inline double point3D::GetX() const
{
	return this->position.x;
}

inline double point3D::GetY() const
{
	return this->position.y;
}

inline double point3D::GetZ() const
{
	return this->position.z;
}

}
}