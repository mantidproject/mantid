#ifndef GLCOLORMAP_H_
#define GLCOLORMAP_H_
#include "boost/shared_ptr.hpp"
#include <string>
/*!
  \class  GLColorMap
  \brief  class handling Colormaps
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0

  GLColorMap class handles the loading of colormap and returning color based on the index.

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
class GLColor;
class GLColorMap
{
public:
	GLColorMap();  ///< Default constructor
	void setColorMapFile(std::string name); ///< Load the color map from file
	boost::shared_ptr<GLColor> getColor(int id); ///< get color corresponding to id
private:
    int color[256][3]; ///< Color map storage
	void defaultColormap(); ///< Sets default color map
};

#endif /*GLCOLORMAP_H_*/

