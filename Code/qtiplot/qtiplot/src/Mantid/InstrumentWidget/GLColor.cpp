//--------------------------------
// Includes
//--------------------------------
#include "GLColor.h"

// On Windows, the windows header file is needed before the OpenGL header
#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/gl.h>

/**
 * Default Constructor
 * @param red The red component of the RGB colour
 * @param green The green component of the RGB colour
 * @param blue The blue component of the RGB colour
 * @param alpha The alpha blending value
 
 */
GLColor::GLColor(float red, float green, float blue, float alpha)
{
  m_rgba[0] = red;
  m_rgba[1] = green;
  m_rgba[2] = blue;
  m_rgba[3] = alpha;
}

/**
 * (virtual) Destructor
 */
GLColor::~GLColor()
{
}

/**
 * This method sets the Red, Green, Blue, Alpha values of the color
 * @param red Red component of color value between [0 - 1]
 * @param green Green Componenent of color value between [0 - 1]
 * @param blue Blue Componenent of color value between [0 - 1]
 * @param alpha Alpha componenet of color value between [0 - 1]
 */
void GLColor::set(float red, float green, float blue, float alpha)
{
  m_rgba[0] = red;
  m_rgba[1] = green;
  m_rgba[2] = blue;
  m_rgba[3] = alpha;
}

/**
 * This method sets the Red, Green, Blue, Alpha values of the color
 * @param red Red component of color value between [0 - 1]
 * @param green Green Componenent of color value between [0 - 1]
 * @param blue Blue Componenent of color value between [0 - 1]
 * @param alpha Alpha componenet of color value between [0 - 1]
 */
void GLColor::get(float& red, float& green, float& blue, float& alpha)
{
  red = m_rgba[0];
  green = m_rgba[1];
  blue = m_rgba[2];
  alpha = m_rgba[3];
}

/**
 * This method executes opengl color commands based on the method provided.
 * @param pm type of opengl color to be used
 */
void GLColor::paint(GLColor::PaintMethod pm)
{
  if(pm == PLAIN) 
  {
    glColor4fv(m_rgba);
  }
  else if(pm == MATERIAL)
  {
    //GLfloat mat_specular[]={1.0,1.0,1.0,1.0};
    glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,m_rgba); 
    glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,20.0);
    //glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,mat_specular);
  }
  else 
  {
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,m_rgba);
  }
}
