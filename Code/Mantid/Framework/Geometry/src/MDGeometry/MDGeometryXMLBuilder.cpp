#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"

#include <boost/functional/hash.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <algorithm>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>
#include <sstream>
namespace Mantid
{
namespace Geometry
{

 struct CompareIMDDimension_const_sptr: public std::unary_function<IMDDimension_const_sptr, bool>
{
private:
   IMDDimension_const_sptr _a;
public:
  CompareIMDDimension_const_sptr(IMDDimension_const_sptr a) : _a(a) {}
  bool operator()(IMDDimension_const_sptr b)
  {
    return _a->getDimensionId() == b->getDimensionId();
  }
};


/**
 Add an ordinary dimension.
 @param dimensionToAdd :: The dimension to add to the geometry.
 @return true if addition was successful.
 */
bool MDGeometryBuilderXML::addOrdinaryDimension(IMDDimension_const_sptr dimensionToAdd) const
{
  bool bAdded = false; //Addition fails by default.
  if(dimensionToAdd.get() != NULL)
  {
    CompareIMDDimension_const_sptr comparitor(dimensionToAdd);
    DimensionContainerType::iterator location = std::find_if(m_vecDimensions.begin(), m_vecDimensions.end(), comparitor);
    if(location == m_vecDimensions.end())
    {
      m_vecDimensions.push_back(dimensionToAdd);
      bAdded = true;
      m_changed = true;
    }
  }
  return bAdded;
}

/**
 Add an x dimension.
 @param dimensionToAdd :: The dimension to add to the geometry.
 @return true if addition was successful.
 */
bool MDGeometryBuilderXML::addXDimension(IMDDimension_const_sptr dimension) const
{
  bool bAdded = false;
  if(dimension.get() != NULL)
  {
    addOrdinaryDimension(dimension);
    m_spXDimension = dimension;
    m_changed = true;
    bAdded = true;
  }
  return bAdded;
}

/**
 Add an y dimension.
 @param dimensionToAdd :: The dimension to add to the geometry.
 @return true if addition was successful.
 */
bool MDGeometryBuilderXML::addYDimension(IMDDimension_const_sptr dimension) const
{
  bool bAdded = false;
  if(dimension.get() != NULL)
  {
    addOrdinaryDimension(dimension);
    m_spYDimension = dimension;
    m_changed = true;
    bAdded = true;
  }
  return bAdded;
}

/**
 Add an z dimension.
 @param dimensionToAdd :: The dimension to add to the geometry.
 @return true if addition was successful.
 */
bool MDGeometryBuilderXML::addZDimension(IMDDimension_const_sptr dimension) const
{
  bool bAdded = false;
  if(dimension.get() != NULL)
  {
    addOrdinaryDimension(dimension);
    m_spZDimension = dimension;
    m_changed = true;
    bAdded = true;
  }
  return bAdded;
}

/**
 Add an t dimension.
 @param dimensionToAdd :: The dimension to add to the geometry.
 @return true if addition was successful.
 */
bool MDGeometryBuilderXML::addTDimension(IMDDimension_const_sptr dimension) const
{
  bool bAdded = false;
  if(dimension.get() != NULL)
  {
    addOrdinaryDimension(dimension);
    m_spTDimension = dimension;
    m_changed = true;
    bAdded = true;
  }
  return bAdded;
}

/**
Builder creational method. Processes added dimensions. creates xml string.
@return xmlstring.
*/
const std::string& MDGeometryBuilderXML::create() const
{
  using namespace Poco::XML;
  std::string formattedXMLString;
  if(true == m_changed)
  {
    //Create the root element for this fragment.
    AutoPtr<Document> pDoc = new Document;
    AutoPtr<Element> dimensionSetElement = pDoc->createElement("DimensionSet");
    pDoc->appendChild(dimensionSetElement);

    //Loop through dimensions and generate xml for each.
    std::string dimensionXMLString;

    DimensionContainerType::iterator it = m_vecDimensions.begin();

    for (; it != m_vecDimensions.end(); ++it)
    {
      dimensionXMLString += (*it)->toXMLString();
    }

    //Pass dimensions to dimension set.
    dimensionSetElement->appendChild(pDoc->createTextNode("%s"));

    //x-dimension mapping.
    AutoPtr<Element> xDimensionElement = pDoc->createElement("XDimension");
    AutoPtr<Element> xDimensionIdElement = pDoc->createElement("RefDimensionId");
    if (hasXDimension())
    {
      std::string xDimensionId = this->m_spXDimension->getDimensionId();
      AutoPtr<Text> idXText = pDoc->createTextNode(xDimensionId);
      xDimensionIdElement->appendChild(idXText);
    }
    xDimensionElement->appendChild(xDimensionIdElement);
    dimensionSetElement->appendChild(xDimensionElement);

    //y-dimension mapping.
    AutoPtr<Element> yDimensionElement = pDoc->createElement("YDimension");
    AutoPtr<Element> yDimensionIdElement = pDoc->createElement("RefDimensionId");
    if (hasYDimension())
    {
      std::string yDimensionId = this->m_spYDimension->getDimensionId();
      AutoPtr<Text> idYText = pDoc->createTextNode(yDimensionId);
      yDimensionIdElement->appendChild(idYText);
    }
    yDimensionElement->appendChild(yDimensionIdElement);
    dimensionSetElement->appendChild(yDimensionElement);

    //z-dimension mapping.
    AutoPtr<Element> zDimensionElement = pDoc->createElement("ZDimension");
    AutoPtr<Element> zDimensionIdElement = pDoc->createElement("RefDimensionId");
    if (hasZDimension())
    {
      std::string zDimensionId = this->m_spZDimension->getDimensionId();
      AutoPtr<Text> idZText = pDoc->createTextNode(zDimensionId);
      zDimensionIdElement->appendChild(idZText);
    }
    zDimensionElement->appendChild(zDimensionIdElement);
    dimensionSetElement->appendChild(zDimensionElement);

    //t-dimension mapping.
    AutoPtr<Element> tDimensionElement = pDoc->createElement("TDimension");
    AutoPtr<Element> tDimensionIdElement = pDoc->createElement("RefDimensionId");
    if (hasTDimension())
    {
      std::string tDimensionId = this->m_spTDimension->getDimensionId();
      AutoPtr<Text> idTText = pDoc->createTextNode(tDimensionId);
      tDimensionIdElement->appendChild(idTText);
    }
    tDimensionElement->appendChild(tDimensionIdElement);
    dimensionSetElement->appendChild(tDimensionElement);

    std::stringstream xmlstream;
    DOMWriter writer;
    writer.writeNode(xmlstream, pDoc);

    m_lastResult = boost::str(boost::format(xmlstream.str().c_str())
      % dimensionXMLString.c_str());
    m_changed = false;
  }
  return m_lastResult;
}

bool MDGeometryBuilderXML::hasXDimension() const
{
  return NULL != this->m_spXDimension.get();
}

bool MDGeometryBuilderXML::hasYDimension() const
{
  return NULL != this->m_spYDimension.get();
}

bool MDGeometryBuilderXML::hasZDimension() const
{
  return NULL != this->m_spZDimension.get();
}

bool MDGeometryBuilderXML::hasTDimension() const
{
  return NULL != this->m_spTDimension.get();
}

/**
 Constructor
 */
MDGeometryBuilderXML::MDGeometryBuilderXML() : m_changed(true)
{
}

/**
 Destructor
 */
MDGeometryBuilderXML::~MDGeometryBuilderXML()
{
}

}
}

