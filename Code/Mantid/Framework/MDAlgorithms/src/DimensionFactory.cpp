#include <Poco/DOM/Element.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NamedNodeMap.h>

#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidGeometry/MDGeometry/MDDimension.h"
#include "MantidGeometry/MDGeometry/MDDimensionRes.h"
#include "MantidMDAlgorithms/DimensionFactory.h"

#include <boost/scoped_ptr.hpp>
#include <boost/regex.hpp>

namespace Mantid
{
namespace MDAlgorithms
{

DimensionFactory DimensionFactory::createDimensionFactory(const std::string& dimensionXMLString)
{
  //Exception safe usage.
  DimensionFactory factory;
  factory.setXMLString(dimensionXMLString);
  return factory;
}

DimensionFactory::DimensionFactory(Poco::XML::Element* dimensionXML) :
  m_dimensionXML(dimensionXML)
{
}

DimensionFactory::DimensionFactory(const DimensionFactory& other) : m_dimensionXML(other.m_dimensionXML)
{
}


DimensionFactory& DimensionFactory::operator=(const DimensionFactory& other)
{
  if(&other != this)
  {
    m_dimensionXML = other.m_dimensionXML;
  }
  return *this;
}


DimensionFactory::DimensionFactory() : m_dimensionXML(NULL)
{
}

DimensionFactory::~DimensionFactory()
{
}

void DimensionFactory::setXMLString(const std::string& dimensionXMLString)
{
  Poco::XML::DOMParser pParser;
  Poco::XML::Document* pDoc = pParser.parseString(dimensionXMLString);
  Poco::XML::Element* pDimensionElement = pDoc->documentElement();
  m_dimensionXML = pDimensionElement;
}

Mantid::Geometry::IMDDimension* DimensionFactory::create() const
{
  return createAsMDDimension();
}

Mantid::Geometry::MDDimension* DimensionFactory::createAsMDDimension() const
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

Mantid::Geometry::MDDimension* DimensionFactory::createRawDimension(
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

    if (regex_match(reciprocalMapping->innerText(), q1Match))
    {
      recipPrimitiveDirection = q1; //rec_dim::q1
    }
    else if (regex_match(reciprocalMapping->innerText(), q2Match))
    {
      recipPrimitiveDirection = q2; //rec_dim::q2
    }
    else
    {
      recipPrimitiveDirection = q3; //rec_dim::q3
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

}
}
