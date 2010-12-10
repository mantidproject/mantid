#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include "MantidAPI/Point3D.h"
#include <cmath>
#include <vector>

namespace Mantid
{
  namespace MDAlgorithms
  {

    BoxImplicitFunction::BoxImplicitFunction(WidthParameter& width, HeightParameter& height, DepthParameter& depth, OriginParameter& origin)
    {
      //Parameters describing box implicit function.
      m_origin = origin;
      m_depth = depth;
      m_height = height;
      m_width = width;

      //calculate cached values. Used for faster evaluation routine.
      m_upperX = origin.getX() + width.getValue()/2;
      m_lowerX = origin.getX() - width.getValue()/2;
      m_upperY = origin.getY() + height.getValue()/2;
      m_lowerY = origin.getY() - height.getValue()/2;
      m_upperZ = origin.getZ() + depth.getValue()/2;
      m_lowerZ = origin.getZ() - depth.getValue()/2;
    }

    bool BoxImplicitFunction::evaluate(const Mantid::API::Point3D* pPoint) const
    {
      return pPoint->getX() <= m_upperX && 
        pPoint->getX() >= m_lowerX && 
        pPoint->getY() <= m_upperY && 
        pPoint->getY() >= m_lowerY && 
        pPoint->getZ() <= m_upperZ && 
        pPoint->getZ() >= m_lowerZ;

    }

    bool BoxImplicitFunction::operator==(const BoxImplicitFunction &other) const
    {
      return 
        this->m_width == other.m_width &&
        this->m_height == other.m_height &&
        this->m_depth == other.m_depth &&
        this->m_origin == other.m_origin;
    }

    bool BoxImplicitFunction::operator!=(const BoxImplicitFunction &other) const
    {
      return !(*this == other);
    }

    std::string BoxImplicitFunction::getName() const
    {
      return functionName();
    }

    double BoxImplicitFunction::getUpperX() const
    {
      return this->m_upperX;
    }

    double BoxImplicitFunction::getLowerX() const
    {
      return this->m_lowerX;
    }

    double BoxImplicitFunction::getUpperY() const
    {
      return this->m_upperY;
    }

    double BoxImplicitFunction::getLowerY() const
    {

      return this->m_lowerY;
    }

    double BoxImplicitFunction::getUpperZ() const
    {
      return this->m_upperZ;
    }

    double BoxImplicitFunction::getLowerZ() const
    {
      return this->m_lowerZ;
    }

    std::string BoxImplicitFunction::toXMLString() const
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
        % m_width.toXMLString().c_str() % m_height.toXMLString() % m_depth.toXMLString() % m_origin.toXMLString().c_str());
      return formattedXMLString;
    }

    BoxImplicitFunction::~BoxImplicitFunction()
    {
    }

  }
}
