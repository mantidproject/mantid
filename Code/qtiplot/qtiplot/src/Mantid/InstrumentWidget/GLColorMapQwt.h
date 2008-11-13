#ifndef GLCOLORMAPQWT_H_
#define GLCOLORMAPQWT_H_
#include "boost/shared_ptr.hpp"
#include "qwt_color_map.h"
#include "GLColorMap.h"
#include <string>
/*!
  \class  GLColorMapQwt
  \brief  class handling GL Colormaps and Qwt Colormap
  \author Srikanth Nagella
  \date   November 2008
  \version 1.0

  GLColorMapQwt class handles the colormap compatible with Qwt

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

class GLColorMapQwt:public GLColorMap, public QwtColorMap
{
public:
	GLColorMapQwt();  ///< Default constructor
	~GLColorMapQwt();
	QwtColorMap* copy()const;
	QRgb  rgb(const QwtDoubleInterval &interval,double value)const;
	unsigned char colorIndex(const QwtDoubleInterval &interval,double value)const;
	QVector<QRgb> colorTable(const QwtDoubleInterval &)const;
};

#endif /*GLCOLORMAP_H_*/

