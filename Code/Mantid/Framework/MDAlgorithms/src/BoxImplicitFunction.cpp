#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include "MantidAPI/Point3D.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <cmath>
#include <vector>

namespace Mantid
{
  namespace MDAlgorithms
  {

    BoxImplicitFunction::BoxImplicitFunction(WidthParameter& width, HeightParameter& height, DepthParameter& depth, OriginParameter& origin) : m_bounds(3)
    {
      //Parameters describing box implicit function.
      m_origin = origin;
      m_depth = depth;
      m_height = height;
      m_width = width;

      //calculate cached values. Used for faster evaluation routine.
      
      //xbounds
      m_bounds[0].upper = origin.getX() + width.getValue()/2;
      m_bounds[0].lower = origin.getX() - width.getValue()/2;
      //ybounds.
      m_bounds[1].upper = origin.getY() + height.getValue()/2;
      m_bounds[1].lower = origin.getY() - height.getValue()/2;
      //zbounds
      m_bounds[2].upper = origin.getZ() + depth.getValue()/2;
      m_bounds[2].lower = origin.getZ() - depth.getValue()/2;
    }

    bool BoxImplicitFunction::evaluate(const Mantid::API::Point3D* pPoint) const
    {
      return pPoint->getX() <= m_bounds[0].upper && 
        pPoint->getX() >= m_bounds[0].lower && 
        pPoint->getY() <= m_bounds[1].upper && 
        pPoint->getY() >= m_bounds[1].lower && 
        pPoint->getZ() <= m_bounds[2].upper && 
        pPoint->getZ() >=m_bounds[2].lower;

    }

    /* Evalauate the implicit function.
    @param coords : coordinates to evaluate function against.
    @param masks : Masking. false for non-masked, otherwise masked.
    @param nDims : Number of dimensions masked + non-masked.
    @return true if all non-masked dimensions are inside defined box.
    */
    bool BoxImplicitFunction::evaluate(const Mantid::coord_t* coords, const bool * masks, const size_t nDims) const
    {
      bool bInside = true;
      bool bTemp = false;
      size_t boxDimension = 0;
      for(size_t i = 0; i < nDims; i++)
      {
        if(!masks[i])
        {
          if(boxDimension > 2)
          {
            throw std::runtime_error("Too many dimensions. Only 3 unmasked dimensions should have been provided!");
          }
          bTemp = coords[i] <= m_bounds[boxDimension].upper && coords[i] >= m_bounds[boxDimension].lower;
          if(!bTemp)
          {
            bInside = false;
          }
          boxDimension++;
        }
      }
      if(boxDimension != 3)
      {
        throw std::runtime_error("Too few dimensions. Only 3 unmasked dimensions should have been provided!");
      }

      return bInside;
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
      return this->m_bounds[0].upper;
    }

    double BoxImplicitFunction::getLowerX() const
    {
      return this->m_bounds[0].lower;
    }

    double BoxImplicitFunction::getUpperY() const
    {
      return this->m_bounds[1].upper;
    }

    double BoxImplicitFunction::getLowerY() const
    {
      return this->m_bounds[1].lower;
    }

    double BoxImplicitFunction::getUpperZ() const
    {
      return this->m_bounds[2].upper;
    }

    double BoxImplicitFunction::getLowerZ() const
    {
      return this->m_bounds[2].lower;
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
