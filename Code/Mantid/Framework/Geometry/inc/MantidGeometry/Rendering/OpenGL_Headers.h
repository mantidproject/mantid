#ifndef MANTID_GEOMETRY_OPENGL_HEADERS_H_
#define MANTID_GEOMETRY_OPENGL_HEADERS_H_

/* Use this header to include OpenGL. It handles the fact that the path on the Mac is different.

   @author Russell Taylor, Tessella
   @date 06/09/2010

   Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

   File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
   Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

#ifdef __APPLE__

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#else

// On Windows, this has to be included before the OpenGL headers
#ifdef _WIN32
  #include "windows.h"
#endif

#include <GL/gl.h>
#include <GL/glu.h>

#endif

#endif /* MANTID_GEOMETRY_OPENGL_HEADERS_H_ */
