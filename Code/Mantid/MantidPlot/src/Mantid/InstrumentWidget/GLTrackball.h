#ifndef GLTRACKBALL_H_
#define GLTRACKBALL_H_
#include "MantidKernel/System.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h"

/**
  \class  GLTrackball
  \brief  An class that implements virtual trackball using mouse interactions
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0

  This class is an implementation of virtual trackball using mouse interactions. It uses Quaternions for
  performing the rotations.

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*/

/// Forward declaration of class GLViewport
class GLViewport;
///
class GLTrackball
{
public:
	GLTrackball(GLViewport* parent=0);
	virtual ~GLTrackball();
	//! Call when the mouse button is pressed to initiate rotation
	void initRotationFrom(int,int);
	//! Call when the mouse is moving during a rotation
	void generateRotationTo(int,int);
	//! Change the rotation speed
	void setRotationSpeed(double);
	//! To be called in the application drawing the OpenGL Scene
	void IssueRotation() const;
  //! Associate the Trackball to a new viewport.
  void setViewport(GLViewport*);
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
	void setRotation(const Mantid::Geometry::Quat& quat);
	//! Set Model center
	void setModelCenter(const Mantid::Geometry::V3D& center);
	//! Get Model center
	Mantid::Geometry::V3D getModelCenter() const;
	//! Reset Trackball
	void reset();
	//! Rotates a bounding box
	void rotateBoundingBox(double& xmin,double& xmax,double& ymin,double& ymax,double& zmin,double& zmax);
private:
	//! Pointer to the viewport to which the trackball is attached
  GLViewport* _viewport;
  //! Project a point on the trackball sphere from viewport coordinates x,y
	void projectOnSphere(int x,int y, Mantid::Geometry::V3D& p);
	//! Generate a 3D point coordinates from coordinates on the viewport.
	void generateTranslationPoint(int x,int y, Mantid::Geometry::V3D& p);
	//! Previous point selected on sphere
	Mantid::Geometry::V3D _lastpoint;
	//! Rotation matrix stored as a quaternion
	Mantid::Geometry::Quat _quaternion;
	//! Rotation matrix (4x4 stored as linear array) used in OpenGL
  double _rotationmatrix[16];
  //! Rotation speed of the trackball
	double _rotationspeed;
	//! Center of rotation
	Mantid::Geometry::V3D  _modelCenter;
	//! Is the centre of rotation offcentered
	bool hasOffset;
};


#endif /*GLTRACKBALL_H_*/
