#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidAPI/Point3D.h"
#include "MantidGeometry/V3D.h"
#include <cmath>
#include <vector>
#include <stdio.h>

namespace Mantid
{
namespace MDAlgorithms
{

PlaneImplicitFunction::PlaneImplicitFunction(NormalParameter& normal, OriginParameter& origin, UpParameter& up, WidthParameter& width) :
  m_origin(origin),
  m_normal(normal),
  m_up(up),
  m_width(width)
{

}

inline double PlaneImplicitFunction::calculateNormContributionAlongAxisComponent(const Mantid::Geometry::V3D& axis) const
{
  using Mantid::Geometry::V3D;

  NormalParameter normalUnit =m_normal.asUnitVector();
  const V3D normal(normalUnit.getX(), normalUnit.getY(), normalUnit.getZ());

  const double hyp = m_width.getValue()/2;

  //Simple trigonometry. Essentially calculates adjacent along axis specified.
  return  hyp*dotProduct(normal, axis);
}

inline NormalParameter PlaneImplicitFunction::calculateEffectiveNormal(const OriginParameter& forwardOrigin) const
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

inline bool PlaneImplicitFunction::isBoundedByPlane(const OriginParameter& origin, const NormalParameter& normal, const Mantid::API::Point3D* pPoint) const
{
  return dotProduct(pPoint->getX() - origin.getX(), pPoint->getY() - origin.getY(), pPoint->getZ()
        - origin.getZ(), normal.getX(), normal.getY(), normal.getZ()) <= 0;
}


bool PlaneImplicitFunction::evaluate(const Mantid::API::Point3D* pPoint) const
{
  using Mantid::Geometry::V3D;
  //TODO: consider caching calculation parameters that share lifetime with object.

  //Create virtual planes separated by (absolute) width from from actual origin. Origins are key.
  const V3D xAxis(1, 0, 0);
  const V3D yAxis(0, 1, 0);
  const V3D zAxis(0, 0, 1);

  const double deltaX = calculateNormContributionAlongAxisComponent(xAxis);
  const double deltaY = calculateNormContributionAlongAxisComponent(yAxis);
  const double deltaZ = calculateNormContributionAlongAxisComponent(zAxis);

  //Virtual forward origin (+width/2 separated along normal)
  const OriginParameter vForwardOrigin(m_origin.getX() + deltaX, m_origin.getY() + deltaY, m_origin.getZ() + deltaZ);

  //invert the normal if the normals are defined in such a way that the origin does not appear in the bounded region of the forward plane.
  const NormalParameter& normal = calculateEffectiveNormal(vForwardOrigin);

  //Virtual backward origin (-width/2 separated along normal)
  const OriginParameter vBackwardOrigin(m_origin.getX() - deltaX, m_origin.getY() - deltaY, m_origin.getZ() - deltaZ);

  bool isBoundedByForwardPlane = isBoundedByPlane(vForwardOrigin, normal, pPoint);

  NormalParameter rNormal = normal.reflect();
  bool isBoundedByBackwardPlane = isBoundedByPlane(vBackwardOrigin, rNormal, pPoint);

  //Only if a point is bounded by both the forward and backward planes can it be considered inside the plane with thickness.
  return isBoundedByForwardPlane && isBoundedByBackwardPlane;
}

bool PlaneImplicitFunction::operator==(const PlaneImplicitFunction &other) const
{
  return this->m_normal == other.m_normal
      && this->m_origin == other.m_origin
      && this->m_width == other.m_width
      && this->m_up == other.m_up;
}

bool PlaneImplicitFunction::operator!=(const PlaneImplicitFunction &other) const
{
  return !(*this == other);
}

std::string PlaneImplicitFunction::getName() const
{
  return functionName();
}

double PlaneImplicitFunction::getOriginX() const
{
  return this->m_origin.getX();
}

double PlaneImplicitFunction::getOriginY() const
{
  return this->m_origin.getY();
}

double PlaneImplicitFunction::getOriginZ() const
{
  return this->m_origin.getZ();
}

double PlaneImplicitFunction::getNormalX() const
{
  return this->m_normal.getX();
}

double PlaneImplicitFunction::getNormalY() const
{
  return this->m_normal.getY();
}

double PlaneImplicitFunction::getNormalZ() const
{
  return this->m_normal.getZ();
}

double PlaneImplicitFunction::getUpX() const
{
  return this->m_up.getX();
}

double PlaneImplicitFunction::getUpY() const
{
  return this->m_up.getY();
}

double PlaneImplicitFunction::getUpZ() const
{
  return this->m_up.getZ();
}

double PlaneImplicitFunction::getPerpendicularX() const
{
  lazyInitalizePerpendicular();
  return this->m_perpendicular.getX();
}

double PlaneImplicitFunction::getPerpendicularY() const
{
  lazyInitalizePerpendicular();
  return this->m_perpendicular.getY();
}

double PlaneImplicitFunction::getPerpendicularZ() const
{
  lazyInitalizePerpendicular();
  return this->m_perpendicular.getZ();
}

double PlaneImplicitFunction::getWidth() const
{
  return this->m_width.getValue();
}

double PlaneImplicitFunction::getAngleMadeWithXAxis() const
{
  using Mantid::Geometry::V3D;
  NormalParameter unitVecNormal = m_normal.asUnitVector();
  V3D vNormal(unitVecNormal.getX(), unitVecNormal.getY(), unitVecNormal.getZ());
  V3D unitVecXAxis(1, 0, 0);
  return std::acos(unitVecXAxis.scalar_prod(vNormal));
}

double PlaneImplicitFunction::getAngleMadeWithYAxis() const
{
  using Mantid::Geometry::V3D;
  NormalParameter unitVecNormal = m_normal.asUnitVector();
  V3D vNormal(unitVecNormal.getX(), unitVecNormal.getY(), unitVecNormal.getZ());
  V3D unitVecYAxis(0, 1, 0);
  return std::acos(unitVecYAxis.scalar_prod(vNormal));
}

double PlaneImplicitFunction::getAngleMadeWithZAxis() const
{
  using Mantid::Geometry::V3D;
  NormalParameter unitVecNormal = m_normal.asUnitVector();
  V3D vNormal(unitVecNormal.getX(), unitVecNormal.getY(), unitVecNormal.getZ());
  V3D unitVecZAxis(0, 0, 1);
  return std::acos(unitVecZAxis.scalar_prod(vNormal));
}

std::string PlaneImplicitFunction::toXMLString() const
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

  AutoPtr<Text> formatText = pDoc->createTextNode("%s%s%s%s");
  paramListElement->appendChild(formatText);
  functionElement->appendChild(paramListElement);

  std::stringstream xmlstream;

  DOMWriter writer;
  writer.writeNode(xmlstream, pDoc);

  std::string formattedXMLString = boost::str(boost::format(xmlstream.str().c_str())
      % m_normal.toXMLString().c_str() % m_origin.toXMLString().c_str() % m_up.toXMLString() % m_width.toXMLString());
  return formattedXMLString;
}

void PlaneImplicitFunction::lazyInitalizePerpendicular() const
{
  //Perpendicular parameter is invalid by default. Create in lazy fashion.
  if(!m_perpendicular.isValid())
  {
      Mantid::Geometry::V3D normalVector(m_normal.getX(), m_normal.getY(), m_normal.getZ());
      Mantid::Geometry::V3D upVector(m_up.getX(), m_up.getY(), m_up.getZ());
      Mantid::Geometry::V3D perpendicularVector = crossProduct(normalVector, upVector);

      //Calculate the perpendicular parameter needed internally.
      m_perpendicular = PerpendicularParameter(perpendicularVector.X(), perpendicularVector.Y(), perpendicularVector.Z());
  }
}


std::vector<double> PlaneImplicitFunction::asRotationMatrixVector() const
{
  using namespace std;
  const double phi = getAngleMadeWithXAxis();
  const double theta = getAngleMadeWithYAxis();
  const double psi =getAngleMadeWithZAxis();

  double rotateAroundXYZArry[9] = {
  cos(theta)*cos(psi),
  -cos(phi)*sin(psi) + sin(phi)*sin(theta)*cos(psi),
  sin(phi)*sin(psi) + cos(phi)*sin(theta)*cos(psi),
  cos(theta)*sin(psi),
  cos(phi)*cos(psi) + sin(phi)*sin(theta)*sin(psi),
  -sin(phi)*cos(psi) + cos(phi)*sin(theta)*sin(psi),
  -sin(theta),
  sin(phi)*cos(theta),
  cos(phi)*cos(theta)};

  return std::vector<double>(rotateAroundXYZArry, rotateAroundXYZArry+9);
}

/// Convenience method. Translates rotation result into Mantid Matrix form.
Mantid::Geometry::Matrix<double> extractRotationMatrix(const PlaneImplicitFunction& plane)
{
  std::vector<double> rotationMatrixVector = plane.asRotationMatrixVector();
  Mantid::Geometry::Matrix<double> rotationMatrix(3,3);
  rotationMatrix[0][0] = rotationMatrixVector[0];
  rotationMatrix[0][1] = rotationMatrixVector[1];
  rotationMatrix[0][2] = rotationMatrixVector[2];
  rotationMatrix[1][0] = rotationMatrixVector[3];
  rotationMatrix[1][1] = rotationMatrixVector[4];
  rotationMatrix[1][2] = rotationMatrixVector[5];
  rotationMatrix[2][0] = rotationMatrixVector[6];
  rotationMatrix[2][1] = rotationMatrixVector[7];
  rotationMatrix[2][2] = rotationMatrixVector[8];
  return rotationMatrix;
}

PlaneImplicitFunction::~PlaneImplicitFunction()
{
}
}

}
