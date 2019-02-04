// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_OPENGL_HEADERS_H_
#define MANTID_GEOMETRY_OPENGL_HEADERS_H_

/* Use this header to include OpenGL. It handles the fact that the path on the
   Mac is different.

   @author Russell Taylor, Tessella
   @date 06/09/2010
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
