#include "ObjCompAssemblyActor.h"
#include "ObjComponentActor.h"
#include "InstrumentActor.h"
#include "OpenGLError.h"

#include "MantidKernel/V3D.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidKernel/Exception.h"
#include <cfloat>
using namespace Mantid;
using namespace Geometry;

ObjCompAssemblyActor::ObjCompAssemblyActor(const InstrumentActor& instrActor,Mantid::Geometry::ComponentID compID):
  ICompAssemblyActor(instrActor,compID),
  m_idData(0),
  m_idPick(0),
  m_n(getObjCompAssembly()->nelements()),
  m_pick_data(),
  m_texturesGenerated(false)
{

  ObjCompAssembly_const_sptr objAss = getObjCompAssembly();
  mNumberOfDetectors = objAss->nelements();
  assert(m_n == mNumberOfDetectors);
  m_data = new unsigned char[m_n*3];
  m_pick_data = new unsigned char[m_n*3];
  for(int i=0;i<getNumberOfDetectors();++i)
  {
    IDetector_const_sptr det = boost::dynamic_pointer_cast<const IDetector>(objAss->getChild(i));
    assert(det);
    detid_t id = det->getID();
    m_detIDs.push_back(id);
    size_t pickID = instrActor.pushBackDetid(id);
    setDetectorColor(m_pick_data,i,GLActor::makePickColor(pickID));
  }
  Mantid::Geometry::BoundingBox boundBox;
  objAss->getBoundingBox(boundBox);
  minBoundBox[0]=boundBox.xMin(); minBoundBox[1]=boundBox.yMin(); minBoundBox[2]=boundBox.zMin();
  maxBoundBox[0]=boundBox.xMax(); maxBoundBox[1]=boundBox.yMax(); maxBoundBox[2]=boundBox.zMax();

}

/**
* Destructor which removes the actors created by this object
*/
ObjCompAssemblyActor::~ObjCompAssemblyActor()
{
  if (m_data)
  {
    delete[] m_data;
    delete[] m_pick_data;
  }
  if ( m_texturesGenerated )
  {
    glDeleteTextures(1,&m_idData);
    glDeleteTextures(1,&m_idPick);
  }
}

/**
* This function is concrete implementation that renders the Child ObjComponents and Child CompAssembly's
*/
void ObjCompAssemblyActor::draw(bool picking)const
{
  OpenGLError::check("ObjCompAssemblyActor::draw(0)");

  if ( !m_texturesGenerated )
  {
    setDataColors();
    setPickColors();
    m_texturesGenerated = true;
  }

  ObjCompAssembly_const_sptr objAss = getObjCompAssembly();
  glPushMatrix();

  unsigned int texID = picking? m_idPick : m_idData;
  // Because texture colours are combined with the geometry colour
  // make sure the current colour is white
  glColor3f(1.0f,1.0f,1.0f);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texID);
  objAss->draw();
  glBindTexture(GL_TEXTURE_2D, 0);
  OpenGLError::check("ObjCompAssemblyActor::draw()");

  glPopMatrix();
}

void ObjCompAssemblyActor::generateTexture(unsigned char* data, unsigned int& id)const
{
  if (id > 0)
  {
    glDeleteTextures(1,&id);
    OpenGLError::check("TexObject::generateTexture()[delete texture] ");
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
  glGenTextures(1, &id);					// Create The Texture
  OpenGLError::check("TexObject::generateTexture()[generate] ");
  glBindTexture(GL_TEXTURE_2D, id);
  OpenGLError::check("TexObject::generateTexture()[bind] ");

  GLint texParam = GL_NEAREST;
  glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
  OpenGLError::check("TexObject::generateTexture()[set data] ");
  /* If the above call to glTexImage2D has generated an error, it is likely as a result
   * of outline="yes" being set in the IDF. If this is enabled then the texture above
   * is generated with a width being equal to the number of points that make up the
   * outline. However, some OpenGL implementations only support textures with a 2^n size.
   * On the machines tested (Ubuntu 14.04, Windows 7, and RHEL6), this was not an issue,
   * but we can't guarantee that a user wont try this on a system that doesn't support
   * non power of 2 textures. In that case, the best thing to do would be to create a
   * texture with a width of the next 2^n up, and adjust the texture coordinates
   * accordingly. However, this is not a trivial change to make, and as far as we can tell
   * no one has ever run into this issue, so it's being left for now. If this does prove
   * problematic in the future, hopefully this note will save you some time figuring out
   * the problem.
   */
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,texParam);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,texParam);
  OpenGLError::check("TexObject::generateTexture()[parameters] ");
}

/**
  * Set colour to a detector.
  * @param data :: pointer to color array
  * @param i :: Index of the detector in ObjCompAssembly
  * @param c :: The colour
  */
void ObjCompAssemblyActor::setDetectorColor(unsigned char* data,size_t i,GLColor c) const
{
    size_t pos = 3*i;
    float r,g,b,a;
    c.get(r,g,b,a);
    data[pos]   = (unsigned char)(r*255);
    data[pos+1] = (unsigned char)(g*255);
    data[pos+2] = (unsigned char)(b*255);
}

void ObjCompAssemblyActor::swap()
{
  if (!m_pick_data)
  {
    m_pick_data = new  unsigned char[m_n*3];
  }
  unsigned char* tmp = m_data;
  m_data = m_pick_data;
  m_pick_data = tmp;
}

const unsigned char* ObjCompAssemblyActor::getColor(int i)const
{
  return &m_data[3*i];
}

void ObjCompAssemblyActor::setColors()
{
  setDataColors();
}

void ObjCompAssemblyActor::setDataColors() const
{
  for(size_t i = 0; i < size_t(m_n); ++i)
  {
    GLColor c = m_instrActor.getColor(m_detIDs[i]);
    setDetectorColor(m_data,i,c);
  }
  generateTexture(m_data,m_idData);
}

void ObjCompAssemblyActor::setPickColors() const
{
  generateTexture(m_pick_data,m_idPick);
}

bool ObjCompAssemblyActor::accept(GLActorVisitor &visitor, GLActor::VisitorAcceptRule )
{
    return visitor.visit(this);
}

bool ObjCompAssemblyActor::accept(GLActorConstVisitor &visitor, GLActor::VisitorAcceptRule) const
{
    return visitor.visit(this);
}

