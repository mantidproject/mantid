// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//--------------------------------
// Includes
//--------------------------------
#include "MantidQtWidgets/InstrumentView/GLColor.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"

#include <ostream>

namespace MantidQt {
namespace MantidWidgets {

/**
* Default Constructor
* @param red :: The red component of the RGB colour
* @param green :: The green component of the RGB colour
* @param blue :: The blue component of the RGB colour
* @param alpha :: The alpha blending value

*/
GLColor::GLColor(float red, float green, float blue, float alpha) {
  m_rgba[0] = (unsigned char)(red * 255);
  m_rgba[1] = (unsigned char)(green * 255);
  m_rgba[2] = (unsigned char)(blue * 255);
  m_rgba[3] = (unsigned char)(alpha * 255);
}

GLColor::GLColor(int r, int g, int b, int a) {
  m_rgba[0] = (unsigned char)r;
  m_rgba[1] = (unsigned char)g;
  m_rgba[2] = (unsigned char)b;
  m_rgba[3] = (unsigned char)a;
}

/**
 * (virtual) Destructor
 */
GLColor::~GLColor() {}

/**
 * This method sets the Red, Green, Blue, Alpha values of the color
 * @param red :: Red component of color value between [0 - 1]
 * @param green :: Green Componenent of color value between [0 - 1]
 * @param blue :: Blue Componenent of color value between [0 - 1]
 * @param alpha :: Alpha componenet of color value between [0 - 1]
 */
void GLColor::set(float red, float green, float blue, float alpha) {
  m_rgba[0] = (unsigned char)(red * 255);
  m_rgba[1] = (unsigned char)(green * 255);
  m_rgba[2] = (unsigned char)(blue * 255);
  m_rgba[3] = (unsigned char)(alpha * 255);
}

/**
 * This method sets the Red, Green, Blue, Alpha values of the color
 * @param red :: Red component of color value between [0 - 1]
 * @param green :: Green Componenent of color value between [0 - 1]
 * @param blue :: Blue Componenent of color value between [0 - 1]
 * @param alpha :: Alpha componenet of color value between [0 - 1]
 */
void GLColor::get(float &red, float &green, float &blue, float &alpha) const {
  red = float(m_rgba[0]) / 255;
  green = float(m_rgba[1]) / 255;
  blue = float(m_rgba[2]) / 255;
  alpha = float(m_rgba[3]) / 255;
}

void GLColor::get(unsigned char &r, unsigned char &g, unsigned char &b) const {
  r = m_rgba[0];
  g = m_rgba[1];
  b = m_rgba[2];
}

/**
 * This method sets copies red,green, and blue color components into a provided
 * buffer
 * @param c :: Pointer to an array of unsigned chars big enough to accept 3
 * bytes
 */
void GLColor::getUB3(unsigned char *c) const {
  *c = m_rgba[0];
  *(c + 1) = m_rgba[1];
  *(c + 2) = m_rgba[2];
}

/**
 * This method executes opengl color commands based on the method provided.
 */
void GLColor::paint() const { glColor4ubv(m_rgba); }

std::ostream &operator<<(std::ostream &ostr, const GLColor &c) {
  ostr << '[' << c.red() << ',' << c.green() << ',' << c.blue() << ','
       << c.alpha() << ']';
  return ostr;
}
} // namespace MantidWidgets
} // namespace MantidQt
