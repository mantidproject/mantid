/*!________________________________________________
* Library        : NTK
* Name           : GLViewport.h
* Author         : L.C.Chapon
* Date           : 9 Nov 2006
* Descritption   : Base class for viewport with method
*                  to resize and get viewport information. 
*                  
*________________________________________________
*/
#ifndef GLVIEWPORT_H_
#define GLVIEWPORT_H_
/*!
  \class  GLViewport
  \brief  class handling OpenGL Viewport
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0

  Base class for viewport with method to resize and get viewport information as well as the projection system it uses and its dimensions. 

  Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
class GLViewport
{
public:
	enum ProjectionType{ ORTHO, PERSPECTIVE};
	GLViewport(int w=0,int h=0); ///< Constructor with Width (w) and Height(h) as inputs
	~GLViewport();               ///< Destructor
    //!Called by the display device when viewport is resized
    void resize(int,int);
    //!Called by client such as GLTrackball when _width and _height are needed
    void getViewport(int*, int*) const;
	ProjectionType getProjectionType()const;
	void getProjection(double&,double&,double&,double&,double&,double&);
	void issueGL() const;
	void setOrtho(double,double,double,double,double,double);
	void setPrespective(double,double,double,double,double,double);

protected:
    int _width         ///< Width of the viewport
		, _height;     ///< Height of the viewport
	ProjectionType projection; ///< Type of display projection
	double Left   ///< Ortho/Prespective Projection xmin value (Left side of the x axis)
		,Right    ///< Ortho/Prespective Projection xmax value (Right side of the x axis)
		,Bottom   ///< Ortho/Prespective Projection ymin value (Bottom side of the y axis)
		,Top      ///< Ortho/Prespective Projection ymax value (Top side of the y axis)
		,Near     ///< Ortho/Prespective Projection zmin value (Near side of the z axis)
		,Far;     ///< Ortho/Prespective Projection zmax value (Far side of the z axis)

};

#endif /*GLVIEWPORT_H_*/
