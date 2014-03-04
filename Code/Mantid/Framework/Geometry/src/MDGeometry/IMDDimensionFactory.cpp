#include <Poco/DOM/Element.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/AutoPtr.h>
#include <boost/make_shared.hpp>

#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"

namespace Mantid
{
namespace Geometry
{

/// Create a dimension object from the provided XML string.
IMDDimension_sptr createDimension(const std::string& dimensionXMLString)
{
  Poco::XML::DOMParser pParser;
  Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(dimensionXMLString);
  return createDimension(*pDoc->documentElement());
}

/// Create a dimension from the provided XML element.
IMDDimension_sptr createDimension(const Poco::XML::Element& dimensionXML)
{
  Poco::AutoPtr<Poco::XML::NamedNodeMap> attributes = dimensionXML.attributes();

  //First and only attribute is the dimension id.
  Poco::XML::Node* dimensionId = attributes->item(0);
  std::string id = dimensionId->innerText();

  std::string name = dimensionXML.getChildElement("Name")->innerText();
  Poco::XML::Element* unitsElement = dimensionXML.getChildElement("Units");
  std::string units = "None";
  if(NULL != unitsElement)
  {
   //Set units if they exist.
   units = unitsElement->innerText();
  }
  double upperBounds = atof(dimensionXML.getChildElement("UpperBounds")->innerText().c_str());
  double lowerBounds = atof(dimensionXML.getChildElement("LowerBounds")->innerText().c_str());
  unsigned int nBins = atoi(dimensionXML.getChildElement("NumberOfBins")->innerText().c_str());
  Poco::XML::Element* integrationXML = dimensionXML.getChildElement("Integrated");

  if (NULL != integrationXML)
  {
    double upperLimit = atof(integrationXML->getChildElement("UpperLimit")->innerText().c_str());
    double lowerLimit = atof(integrationXML->getChildElement("LowerLimit")->innerText().c_str());

    //As it is not currently possible to set integration ranges on a MDDimension or MDGeometryDescription, boundaries become integration ranges.
    upperBounds = upperLimit;
    lowerBounds = lowerLimit;
  }

  return boost::make_shared<MDHistoDimension>(name, id, units, static_cast<coord_t>(lowerBounds), static_cast<coord_t>(upperBounds), nBins);
}

/** Create a dimension object from the provided XML string, overriding certain attributes.
 *  @param dimensionXMLString The XML string from which to construct the dimension object.
 *  @param nBins              The number of bins to set on the dimension object.
 *  @param min                The minimum extent to set on the dimension.
 *  @param max                The maximum extent to set on the dimension.
 *  @return The created dimension.
 */
IMDDimension_sptr createDimension(const std::string& dimensionXMLString, int nBins, coord_t min, coord_t max)
{
  auto dimension = createDimension(dimensionXMLString);
  dimension->setRange(nBins,min,max);
  return dimension;
}

} // namespace
} // namespace
