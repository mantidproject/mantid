#include "TexObject.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/Object.h"

#include <algorithm>

TexObject::TexObject(const boost::shared_ptr<const Mantid::Geometry::IComponent> obj,bool withDisplayList):
MantidObject(boost::dynamic_pointer_cast<const Mantid::Geometry::IObjComponent>(obj)->shape(),withDisplayList),
m_id(0),
m_n(boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(obj)->nelements()),
m_pick_data()
{
  m_data = new unsigned char[m_n*3];

  unsigned int pos = 0;
  for(int i = 0; i < m_n; ++i)
  {
//    m_index_to_detID_map[i] = boost::dynamic_pointer_cast<Mantid::Geometry::IDetector>((*obj)[i])->getID();
    m_data[pos]   = rand() % 256;
    m_data[pos+1] = rand() % 256;
    m_data[pos+2] = rand() % 256;
    pos += 3;
  }

  generateTexture();

}

TexObject::~TexObject()
{
  delete [] m_data;
  if (m_pick_data)
  {
    delete [] m_pick_data;
  }
}

void TexObject::generateTexture()
{
  if (m_id > 0)
  {
    glDeleteTextures(1,&m_id);
  }

  bool vertical = true; // depends on the tex coordinates of the shape object

  int width = m_n;
  int height = 1;
  if (vertical)
  {
    width = 1;
    height = m_n;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &m_id);					// Create The Texture
  //std::cerr<<"m_id="<<m_id<<' '<<m_n<<'\n';
  GLuint err=glGetError(); if (err) std::cerr<<err<<" GL 1d error1\n";
  glBindTexture(GL_TEXTURE_2D, m_id);
  err=glGetError(); if (err) std::cerr<<err<<" GL 1d error2\n";

  GLint texParam = GL_NEAREST;
  glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, m_data);
  err=glGetError(); if (err) std::cerr<<err<<" GL 1d error3\n";
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,texParam);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,texParam);
}

/// Method that defines ObjCompAssembly geometry. Calls ObjCompAssembly draw method
void TexObject::define()
{
  // Because texture colours are combined with the geometry colour
  // make sure the current colour is white
  glColor3f(1.0f,1.0f,1.0f);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_id);
  Obj->draw();
  glBindTexture(GL_TEXTURE_2D, 0);
}

/**
  * Set colour to a detector.
  * @param i Index of the detector in ObjCompAssembly
  * @param c The colour
  */
void TexObject::setDetectorColor(int i,GLColor c)
{
    int pos = 3*i;
    float r,g,b,a;
    c.get(r,g,b,a);
    m_data[pos]   = (unsigned char)(r*255);
    m_data[pos+1] = (unsigned char)(g*255);
    m_data[pos+2] = (unsigned char)(b*255);
}

void TexObject::swap()
{
  if (!m_pick_data)
  {
    m_pick_data = new  unsigned char[m_n*3];
  }
  unsigned char* tmp = m_data;
  m_data = m_pick_data;
  m_pick_data = tmp;
}

const unsigned char* TexObject::getColor(int i)const
{
  return &m_data[3*i];
}
