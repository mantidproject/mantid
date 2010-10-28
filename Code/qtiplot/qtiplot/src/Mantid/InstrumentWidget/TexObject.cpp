#include "TexObject.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Objects/Object.h"

TexObject::TexObject(const boost::shared_ptr<const Mantid::Geometry::ObjCompAssembly> obj,bool withDisplayList):
MantidObject(obj->shape(),withDisplayList)
{
  bool vertical = false; // depends on the tex coordinates of the shape object

  int width = m_n;
  int height = 1;
  if (vertical)
  {
    width = 1;
    height = m_n;
  }

  m_data = new unsigned char[width*height*3];

  unsigned int pos = 0;
  for(unsigned int i = 0; i < m_n; ++i)
  {
    m_data[pos] = rand() % 256;
    m_data[pos+1] = rand() % 256;
    m_data[pos+2] = rand() % 256;
    pos += 3;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &m_id);					// Create The Texture
  std::cerr<<"m_id="<<m_id<<'\n';
  GLuint err=glGetError(); if (err) std::cerr<<err<<" GL 1d error1\n";
  glBindTexture(GL_TEXTURE_2D, m_id);
  err=glGetError(); if (err) std::cerr<<err<<" GL 1d error2\n";

  glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, m_data);
  err=glGetError(); if (err) std::cerr<<err<<" GL 1d error3\n";
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
}

TexObject::~TexObject()
{
}

/// Method that defines ObjComponent geometry. Calls ObjComponent draw method
void TexObject::define()
{
  Obj->draw();
}

void TexObject::setDetectorColor(int i,GLColor c)
{

}
