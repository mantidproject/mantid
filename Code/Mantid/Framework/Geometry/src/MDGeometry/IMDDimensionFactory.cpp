#include <Poco/DOM/Element.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeList.h> 
#include <Poco/DOM/NamedNodeMap.h>

#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidGeometry/MDGeometry/MDDimension.h"
#include "MantidGeometry/MDGeometry/MDDimensionRes.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"

#include <boost/scoped_ptr.hpp>
#include <boost/regex.hpp>

namespace Mantid
{
namespace Geometry
{

IMDDimensionFactory IMDDimensionFactory::createDimensionFactory(const std::string& dimensionXMLString)
{
  //Exception safe usage.
  IMDDimensionFactory factory;
  factory.setXMLString(dimensionXMLString);
  return factory;
}

IMDDimensionFactory::IMDDimensionFactory(Poco::XML::Element* dimensionXML) :
  m_dimensionXML(dimensionXML)
{
}

IMDDimensionFactory::IMDDimensionFactory(const IMDDimensionFactory& other) : m_dimensionXML(other.m_dimensionXML)
{
}


IMDDimensionFactory& IMDDimensionFactory::operator=(const IMDDimensionFactory& other)
{
  if(&other != this)
  {
    m_dimensionXML = other.m_dimensionXML;
  }
  return *this;
}


IMDDimensionFactory::IMDDimensionFactory() : m_dimensionXML(NULL)
{
}

IMDDimensionFactory::~IMDDimensionFactory()
{
}

void IMDDimensionFactory::setXMLString(const std::string& dimensionXMLString)
{
  Poco::XML::DOMParser pParser;
  Poco::XML::Document* pDoc = pParser.parseString(dimensionXMLString);
  Poco::XML::Element* pDimensionElement = pDoc->documentElement();
  m_dimensionXML = pDimensionElement;
}

Mantid::Geometry::IMDDimension* IMDDimensionFactory::create() const
{
  return createAsMDDimension();
}

Mantid::Geometry::MDDimension* IMDDimensionFactory::createAsMDDimension() const
{
  using namespace Mantid::Geometry;

  if(m_dimensionXML == NULL)
  {
    throw std::runtime_error("Must provide dimension xml before creation");
  }

  Poco::XML::NamedNodeMap* attributes = m_dimensionXML->attributes();

  //First and only attribute is the dimension id.
  Poco::XML::Node* dimensionId = attributes->item(0);
  std::string id = dimensionId->innerText();

  Poco::XML::Element* reciprocalMapping = m_dimensionXML->getChildElement("ReciprocalDimensionMapping");
  MDDimension* mdDimension = createRawDimension(reciprocalMapping, id);
  std::string name = m_dimensionXML->getChildElement("Name")->innerText();
  mdDimension->setName(name);

  double upperBounds = atof(m_dimensionXML->getChildElement("UpperBounds")->innerText().c_str());
  double lowerBounds = atof(m_dimensionXML->getChildElement("LowerBounds")->innerText().c_str());
  unsigned int nBins = atoi(m_dimensionXML->getChildElement("NumberOfBins")->innerText().c_str());
  Poco::XML::Element* integrationXML = m_dimensionXML->getChildElement("Integrated");

  if (NULL != integrationXML)
  {
    double upperLimit = atof(integrationXML->getChildElement("UpperLimit")->innerText().c_str());
    double lowerLimit = atof(integrationXML->getChildElement("LowerLimit")->innerText().c_str());

    //As it is not currently possible to set integration ranges on a MDDimension or MDGeometryDescription, boundaries become integration ranges.
    upperBounds = upperLimit;
    lowerBounds = lowerLimit;
  }

  mdDimension->setRange(lowerBounds, upperBounds, nBins);
  return mdDimension;
}

Mantid::Geometry::MDDimension* IMDDimensionFactory::createRawDimension(
    Poco::XML::Element* reciprocalMapping, const std::string& id) const
{
  using namespace Mantid::Geometry;
  MDDimension* mdDimension;

  if (NULL != reciprocalMapping)
  {
    rec_dim recipPrimitiveDirection;

    //Reciprocal dimensions are either q1, q2, or  q3.
    static const boost::regex q1Match("(q1)|(qx)");
    static const boost::regex q2Match("(q2)|(qy)");
    static const boost::regex q3Match("(q3)|(qz)");

    if (regex_match(reciprocalMapping->innerText(), q1Match))
    {
      recipPrimitiveDirection = q1; //rec_dim::q1
    }
    else if (regex_match(reciprocalMapping->innerText(), q2Match))
    {
      recipPrimitiveDirection = q2; //rec_dim::q2
    }
    else if (regex_match(reciprocalMapping->innerText(), q3Match))
    {
      recipPrimitiveDirection = q3; //rec_dim::q3
    }
    else
    {
      throw std::runtime_error("IMDDimensionFactory cannot recognize reciprocal dimension");
    }
    //Create the dimension as a reciprocal dimension
    mdDimension = new MDDimensionRes(id, recipPrimitiveDirection);
  }
  else
  {
    //Create the dimension as an orthogonal dimension.
    mdDimension = new MDDimension(id);
  }
  return mdDimension;
}

/**
 Convenience service non-member function. Hides use of factory. Creates IMDDimension.
 @param dimensionXMLString :: Dimension xml.
 @return new IMDDimension in a shared pointer.
 */
Mantid::Geometry::IMDDimension_sptr createDimension(const std::string& dimensionXMLString)
 {
   IMDDimensionFactory factory = IMDDimensionFactory::createDimensionFactory(dimensionXMLString);
   return Mantid::Geometry::IMDDimension_sptr(factory.create());
 }

/**
 Convenience service non-member function. Hides use of factory. Creates IMDDimension. Also sets min max and number of bins on the dimension.
 @param dimensionXMLString :: Dimension xml.
 @param nBins :: Number of bins.
 @param min :: Minimum
 @param max :: Maximum
 @return new IMDDimension in a shared pointer.
 */
 Mantid::Geometry::IMDDimension_sptr createDimension(const std::string& dimensionXMLString, int nBins, double min, double max)
 {
   IMDDimensionFactory factory = IMDDimensionFactory::createDimensionFactory(dimensionXMLString);
   Mantid::Geometry::MDDimension* dimension = factory.createAsMDDimension();
   double currentMin = min;
   double currentMax = max;
   //Set the number of bins to use for a given dimension.
   dimension->setRange(currentMin, currentMax, nBins);
   return Mantid::Geometry::IMDDimension_sptr(dimension);
 }

}
}
