#ifndef GLTRACKBALL_H_
#define GLTRACKBALL_H_
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Quat.h"

/**
  \class  GLTrackball
  \brief  An class that implements virtual trackball using mouse interactions
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0

  This class is an implementation of virtual trackball using mouse interactions. It uses Quaternions for
  performing the rotations.

*/

/// Forward declaration of class GLViewport
class Viewport;
///
class GLTrackball
{
public:
	GLTrackball(Viewport* viewport);
	//! Call when the mouse button is pressed to initiate rotation
	void initRotationFrom(int,int);
	//! Call when the mouse is moving during a rotation
	void generateRotationTo(int,int);
	//! Change the rotation speed
	void setRotationSpeed(double);
	//! To be called in the application drawing the OpenGL Scene
	void IssueRotation() const;
  //! Associate the Trackball to a new viewport.
  //void setViewport(GLViewport*);
	//! Call when the mouse button is pressed to initiate zoom
	void initZoomFrom(int,int);
	//! Call when the mouse motion to issue zoom
	void generateZoomTo(int,int);
	//! Call when the mouse button is pressed to translate
	void initTranslateFrom(int,int);
	//! Call when the mouse is moving during a translation
	void generateTranslationTo(int,int);
	//! Call to set the View to X+ direction
	void setViewToXPositive();
	//! Call to set the View to Y+ direction
	void setViewToYPositive();
	//! Call to set the View to Z+ direction
	void setViewToZPositive();
	//! Call to set the View to X- direction
	void setViewToXNegative();
	//! Call to set the View to Y- direction
	void setViewToYNegative();
	//! Call to set the View to Z- direction
	void setViewToZNegative();
	//! Set Rotation
	void setRotation(const Mantid::Kernel::Quat& quat);
	//! Set Model center
	void setModelCenter(const Mantid::Kernel::V3D& center);
	//! Get Model center
	Mantid::Kernel::V3D getModelCenter() const;
	//! Reset Trackball
	void reset();
	//! Rotates a bounding box
	void rotateBoundingBox(double& xmin,double& xmax,double& ymin,double& ymax,double& zmin,double& zmax);
  Mantid::Kernel::Quat getRotation() const {return m_quaternion;}
private:
  //! Project a point on the trackball sphere from viewport coordinates x,y
	void projectOnSphere(int x,int y, Mantid::Kernel::V3D& p);
	//! Generate a 3D point coordinates from coordinates on the viewport.
	void generateTranslationPoint(int x,int y, Mantid::Kernel::V3D& p);

  //! Pointer to the viewport to which the trackball is attached
  Viewport* m_viewport;
	//! Previous point selected on sphere
	Mantid::Kernel::V3D m_lastpoint;
	//! Rotation matrix stored as a quaternion
	Mantid::Kernel::Quat m_quaternion;
	//! Rotation matrix (4x4 stored as linear array) used in OpenGL
  double m_rotationmatrix[16];
  //! Rotation speed of the trackball
	double m_rotationspeed;
	//! Center of rotation
	Mantid::Kernel::V3D  m_modelCenter;
	//! Is the centre of rotation offcentered
	bool m_hasOffset;
};


#endif /*GLTRACKBALL_H_*/
