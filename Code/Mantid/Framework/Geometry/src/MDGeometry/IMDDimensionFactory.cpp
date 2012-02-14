#include <Poco/DOM/Element.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeList.h> 
#include <Poco/DOM/NamedNodeMap.h>

#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
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

/// Constructor
IMDDimensionFactory::IMDDimensionFactory() : m_dimensionXML(NULL)
{
}

/// Destructor
IMDDimensionFactory::~IMDDimensionFactory()
{
}

/**Set the xml string from which the dimension will be generated.
 @param dimensionXMLString : xml string to generate the dimension from.
*/
void IMDDimensionFactory::setXMLString(const std::string& dimensionXMLString)
{
  Poco::XML::DOMParser pParser;
  Poco::XML::Document* pDoc = pParser.parseString(dimensionXMLString);
  Poco::XML::Element* pDimensionElement = pDoc->documentElement();
  m_dimensionXML = pDimensionElement;
}


/**Creation method of factory using xml as-is.
 @return IMDDimension generated.
*/
Mantid::Geometry::IMDDimension* IMDDimensionFactory::create() const
{
  return doCreate();
}


/**Creation method of factory using xml with overrides.
 @param nBins : overrriden number of bins
 @param min : overriden minimum
 @param max : overriden maximum
 @return IMDDimension generated.
*/
Mantid::Geometry::IMDDimension* IMDDimensionFactory::create(int nBins, double min, double max) const
{
  MDHistoDimension* product =  doCreate();
  product->setRange(nBins, static_cast<coord_t>(min), static_cast<coord_t>(max)); //Override the number of bins, min and max.
  return product;
}

/** Create the dimension as a MDHistogram dimension.
*/
Mantid::Geometry::MDHistoDimension* IMDDimensionFactory::doCreate() const
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

  std::string name = m_dimensionXML->getChildElement("Name")->innerText();
  Poco::XML::Element* unitsElement = m_dimensionXML->getChildElement("Units");
  std::string units = "None";
  if(NULL != unitsElement)
  {
   //Set units if they exist.
   units = unitsElement->innerText();
  }
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

  return new MDHistoDimension(name, id, units, static_cast<coord_t>(lowerBounds), static_cast<coord_t>(upperBounds), nBins);
}

/**
 Convenience service non-member function. Hides use of factory. Creates IMDDimension.
 @param dimensionXMLString :: Dimension xml.
 @return new IMDDimension in a shared pointer.
 */
Mantid::Geometry::IMDDimension_sptr createDimension(const std::string& dimensionXMLString)
 {
   IMDDimensionFactory factory = IMDDimensionFactory::createDimensionFactory(dimensionXMLString);
   return IMDDimension_sptr(factory.create());
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
   return IMDDimension_sptr(factory.create(nBins, min, max));
 }


} // namespace
} // namespace
