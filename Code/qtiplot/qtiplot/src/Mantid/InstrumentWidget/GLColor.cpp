#include "GLColor.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

GLColor::GLColor(float R, float G, float B, float alpha)
{
   _v[0]=R;_v[1]=G;_v[2]=B;_v[3]=alpha;
}
GLColor::GLColor(const GLColor& color)
{
    _v[0]=color._v[0];_v[1]=color._v[1];_v[2]=color._v[2];_v[3]=color._v[3];
}
GLColor::~GLColor()
{
}

/**
 * This method sets the Red, Green, Blue, Alpha values of the color
 * @param R Red component of color value between [0 - 1]
 * @param G Green Componenent of color value between [0 - 1]
 * @param B Blue Componenent of color value between [0 - 1]
 * @param alpha Alpha componenet of color value between [0 - 1]
 */
void GLColor::set(float R, float G, float B, float alpha)
{
    _v[0]=R;_v[1]=G;_v[2]=B;_v[3]=alpha;
}

/**
 * This method executes opengl color commands based on the method provided.
 * @param method type of opengl color to be used
 */
void GLColor::paint(paintMethod method)
{
    if (method==PLAIN) glColor4fv(_v);
	else if (method==MATERIAL){
//		GLfloat mat_specular[]={1.0,1.0,1.0,1.0};
		glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,_v); 
		glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,20.0);
//		glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,mat_specular);
	}else glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,_v);
}
