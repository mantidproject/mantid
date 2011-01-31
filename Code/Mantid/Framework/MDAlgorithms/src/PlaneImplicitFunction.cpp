#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidAPI/Point3D.h"
#include "MantidGeometry/V3D.h"
#include <cmath>
#include <vector>
#include <iostream>

namespace Mantid
{
namespace MDAlgorithms
{

PlaneImplicitFunction::PlaneImplicitFunction(NormalParameter& normal, OriginParameter& origin) :
  m_origin(origin),
  m_normal(normal)
{
}

bool PlaneImplicitFunction::evaluate(const Mantid::API::Point3D* pPoint) const
{
  std::vector<double> num;
  dotProduct<double> (pPoint->getX() - m_origin.getX(), pPoint->getY() - m_origin.getY(), pPoint->getZ()
      - m_origin.getZ(), m_normal.getX(), m_normal.getY(), m_normal.getZ(), num);
  //return num.at(0) + num.at(1) + num.at(2) / absolute(normalX, normalY, normalZ) <= 0; //Calculates distance, but magnituted of normal not important in this algorithm
  return num.at(0) + num.at(1) + num.at(2) <= 0;
}

bool PlaneImplicitFunction::operator==(const PlaneImplicitFunction &other) const
{
  return this->m_normal == other.m_normal && this->m_origin == other.m_origin;
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

  AutoPtr<Text> formatText = pDoc->createTextNode("%s%s");
  paramListElement->appendChild(formatText);
  functionElement->appendChild(paramListElement);

  std::stringstream xmlstream;

  DOMWriter writer;
  writer.writeNode(xmlstream, pDoc);

  std::string formattedXMLString = boost::str(boost::format(xmlstream.str().c_str())
      % m_normal.toXMLString().c_str() % m_origin.toXMLString().c_str());
  return formattedXMLString;
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
