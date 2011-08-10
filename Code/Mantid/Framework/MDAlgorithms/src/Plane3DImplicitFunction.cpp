#include "MantidGeometry/MDGeometry/MDPlane.h"
#include "MantidKernel/V3D.h"
#include "MantidMDAlgorithms/Plane3DImplicitFunction.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <cmath>
#include <vector>

using namespace Mantid::Geometry;

namespace Mantid
{
namespace MDAlgorithms
{

Plane3DImplicitFunction::Plane3DImplicitFunction(NormalParameter& normal, OriginParameter& origin, WidthParameter& width) :
  m_origin(origin),
  m_normal(normal),
  m_width(width)
{
  //Create virtual planes separated by (absolute) width from from actual origin. Origins are key.
  using Mantid::Kernel::V3D;
  const V3D xAxis(1, 0, 0);
  const V3D yAxis(0, 1, 0);
  const V3D zAxis(0, 0, 1);

  const double deltaX = calculateNormContributionAlongAxisComponent(xAxis);
  const double deltaY = calculateNormContributionAlongAxisComponent(yAxis);
  const double deltaZ = calculateNormContributionAlongAxisComponent(zAxis);

  /// Cached calculation forward origin
  OriginParameter m_calculationForwardOrigin;
  OriginParameter m_calculationBackwardOrigin;
  NormalParameter m_calculationNormal;

  //Virtual forward origin (+width/2 separated along normal)
  m_calculationForwardOrigin = OriginParameter(m_origin.getX() + deltaX, m_origin.getY() + deltaY, m_origin.getZ() + deltaZ);

  //invert the normal if the normals are defined in such a way that the origin does not appear in the bounded region of the forward plane.
  m_calculationNormal = calculateEffectiveNormal(m_calculationForwardOrigin);

  //Virtual backward origin (-width/2 separated along normal)
  m_calculationBackwardOrigin = OriginParameter (m_origin.getX() - deltaX, m_origin.getY() - deltaY, m_origin.getZ() - deltaZ);

  // OK, now we build the MDPlanes that represent these 3D planes

  // TODO: Handle mapping to the nd dimensions of the MDEventWorkspace
  size_t nd=3;

  std::vector<coord_t> normalForward(nd,0);
  std::vector<coord_t> pointForward(nd,0);
  std::vector<coord_t> normalBackward(nd,0);
  std::vector<coord_t> pointBackward(nd,0);

  // Create the normal and origin
  for (size_t d=0; d<nd; d++)
  {
    normalForward[d] = -m_calculationNormal[int(d)];
    normalBackward[d] = m_calculationNormal[int(d)];
    pointForward[d] = m_calculationForwardOrigin[int(d)];
    pointBackward[d] = m_calculationBackwardOrigin[int(d)];
  }

  // Make the planes
  MDPlane planeForward(normalForward, pointForward);
  MDPlane planeBackward(normalBackward, pointBackward);

  // Add the 2 planes to the MDImplicitFunction.
  this->addPlane(planeForward);
  this->addPlane(planeBackward);

}

inline double Plane3DImplicitFunction::calculateNormContributionAlongAxisComponent(const Mantid::Kernel::V3D& axis) const
{
  using Mantid::Kernel::V3D;

  NormalParameter normalUnit =m_normal.asUnitVector();
  const V3D normal(normalUnit.getX(), normalUnit.getY(), normalUnit.getZ());

  const double hyp = m_width.getValue()/2;

  //Simple trigonometry. Essentially calculates adjacent along axis specified.
  return  hyp*dotProduct(normal, axis);
}

inline NormalParameter Plane3DImplicitFunction::calculateEffectiveNormal(const OriginParameter& forwardOrigin) const
{
    //Figure out whether the origin is bounded by the forward plane.
    bool planesOutwardLooking = dotProduct(m_origin.getX() - forwardOrigin.getX(), m_origin.getY() - forwardOrigin.getY(), m_origin.getZ()
        - forwardOrigin.getZ(), m_normal.getX(), m_normal.getY(), m_normal.getZ()) <= 0;
    //Fix orientation if necessary.
    if(planesOutwardLooking)
    {
      return m_normal;
    }
    else // Inward looking virtual planes.
    {
      return m_normal.reflect();
    }
}

bool Plane3DImplicitFunction::operator==(const Plane3DImplicitFunction &other) const
{
  return this->m_normal == other.m_normal
      && this->m_origin == other.m_origin
      && this->m_width == other.m_width;
}

bool Plane3DImplicitFunction::operator!=(const Plane3DImplicitFunction &other) const
{
  return !(*this == other);
}

std::string Plane3DImplicitFunction::getName() const
{
  return functionName();
}

double Plane3DImplicitFunction::getOriginX() const
{
  return this->m_origin.getX();
}

double Plane3DImplicitFunction::getOriginY() const
{
  return this->m_origin.getY();
}

double Plane3DImplicitFunction::getOriginZ() const
{
  return this->m_origin.getZ();
}

double Plane3DImplicitFunction::getNormalX() const
{
  return this->m_normal.getX();
}

double Plane3DImplicitFunction::getNormalY() const
{
  return this->m_normal.getY();
}

double Plane3DImplicitFunction::getNormalZ() const
{
  return this->m_normal.getZ();
}

double Plane3DImplicitFunction::getWidth() const
{
  return this->m_width.getValue();
}

std::string Plane3DImplicitFunction::toXMLString() const
{
  using namespace Poco::XML;

  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> functionElement = pDoc->createElement("Function");
  pDoc->appendChild(functionElement);
  AutoPtr<Element> typeElement = pDoc->createElement("Type");
  AutoPtr<Text> typeText = pDoc->createTextNode(this->getName());
  typeElement->appendChild(typeText);
  functionElement->appendChild(typeElement);

  AutoPtr<Element> paramListElement = pDoc->createElement("ParameterList");

  AutoPtr<Text> formatText = pDoc->createTextNode("%s%s%s");
  paramListElement->appendChild(formatText);
  functionElement->appendChild(paramListElement);

  std::stringstream xmlstream;

  DOMWriter writer;
  writer.writeNode(xmlstream, pDoc);

  std::string formattedXMLString = boost::str(boost::format(xmlstream.str().c_str())
      % m_normal.toXMLString().c_str() % m_origin.toXMLString().c_str() % m_width.toXMLString());
  return formattedXMLString;
}



Plane3DImplicitFunction::~Plane3DImplicitFunction()
{
}

}

}
