#include "MantidMDAlgorithms/Box3DImplicitFunction.h"
#include "MantidAPI/Point3D.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <cmath>
#include <vector>

namespace Mantid
{
  namespace MDAlgorithms
  {

    Box3DImplicitFunction::Box3DImplicitFunction(WidthParameter& width, HeightParameter& height, DepthParameter& depth, OriginParameter& origin)
    : MDBoxImplicitFunction()
    {
      //Parameters describing box implicit function.
      m_origin = origin;
      m_depth = depth;
      m_height = height;
      m_width = width;

      //calculate cached values. Used for faster evaluation routine.
      min.resize(3);
      max.resize(3);

      //xbounds
      max[0] = origin.getX() + width.getValue()/2;
      min[0] = origin.getX() - width.getValue()/2;
      //ybounds.
      max[1] = origin.getY() + height.getValue()/2;
      min[1] = origin.getY() - height.getValue()/2;
      //zbounds
      max[2] = origin.getZ() + depth.getValue()/2;
      min[2] = origin.getZ() - depth.getValue()/2;

      this->construct(min,max);
    }


    bool Box3DImplicitFunction::operator==(const Box3DImplicitFunction &other) const
    {
      return
        this->m_width == other.m_width &&
        this->m_height == other.m_height &&
        this->m_depth == other.m_depth &&
        this->m_origin == other.m_origin;
    }

    bool Box3DImplicitFunction::operator!=(const Box3DImplicitFunction &other) const
    {
      return !(*this == other);
    }

    std::string Box3DImplicitFunction::getName() const
    {
      return functionName();
    }

    double Box3DImplicitFunction::getUpperX() const
    {
      return this->max[0];
    }

    double Box3DImplicitFunction::getLowerX() const
    {
      return this->min[0];
    }

    double Box3DImplicitFunction::getUpperY() const
    {
      return this->max[1];
    }

    double Box3DImplicitFunction::getLowerY() const
    {
      return this->min[1];
    }

    double Box3DImplicitFunction::getUpperZ() const
    {
      return this->max[2];
    }

    double Box3DImplicitFunction::getLowerZ() const
    {
      return this->min[2];
    }

    std::string Box3DImplicitFunction::toXMLString() const
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

    Box3DImplicitFunction::~Box3DImplicitFunction()
    {
    }

  }
}
