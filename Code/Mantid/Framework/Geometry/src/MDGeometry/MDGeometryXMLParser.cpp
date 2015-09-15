#include <algorithm>

#include "MantidGeometry/MDGeometry/MDGeometryXMLParser.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"

#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>

namespace Mantid {
namespace Geometry {
/// Helper unary comparison type for finding IMDDimensions by a specified id.
struct findID
    : public std::unary_function<Mantid::Geometry::IMDDimension_sptr, bool> {
  const std::string m_id;
  findID(const std::string &id) : m_id(id) {}

  bool operator()(const Mantid::Geometry::IMDDimension_sptr obj) const {
    return m_id == obj->getDimensionId();
  }
  findID &operator=(const findID &);
};

/// Helper unary comparison type for finding non-integrated dimensions.
struct findIntegrated
    : public std::unary_function<Mantid::Geometry::IMDDimension_sptr, bool> {
  bool operator()(const Mantid::Geometry::IMDDimension_sptr obj) const {
    return obj->getIsIntegrated();
  }
  findIntegrated &operator=(const findIntegrated &);
};

/**
Validate the current object. Take action if not set up properly.
*/
void MDGeometryXMLParser::validate() const {
  if (!m_executed) {
    throw std::runtime_error("Attempting to get dimension information from "
                             "MDGeometryXMLParser, before calling ::execute()");
  }
}

/**
Peforms the processing associated with these transformations.
*/
void MDGeometryXMLParser::execute() {
  typedef std::vector<Mantid::Geometry::IMDDimension_sptr>::iterator Iterator;

  Poco::XML::DOMParser pParser;
  Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(m_xmlToProcess);
  Poco::XML::Element *pRootElem = pDoc->documentElement();
  // Apply root node checking if supplied.
  Poco::XML::Element *geometryXMLElement = NULL;
  if (!m_rootNodeName.empty()) {
    Poco::XML::Element *temp = pRootElem->getChildElement(m_rootNodeName);
    geometryXMLElement = temp;
    if (geometryXMLElement == NULL) {
      std::string message =
          "Root node was not found to be the expected value of " +
          m_rootNodeName;
      throw std::runtime_error(message);
    }
  } else {
    // The default is to take the root node to be the geometry xml element.
    geometryXMLElement = pRootElem;
  }

  Poco::AutoPtr<Poco::XML::NodeList> dimensionsXML =
      geometryXMLElement->getElementsByTagName(
          MDGeometryXMLDefinitions::workspaceDimensionElementName());
  size_t nDimensions = dimensionsXML->length();
  VecIMDDimension_sptr vecAllDims(nDimensions);

  ////Extract dimensions
  for (size_t i = 0; i < nDimensions; i++) {
    Poco::XML::Element *dimensionXML = static_cast<Poco::XML::Element *>(
        dimensionsXML->item(static_cast<unsigned long>(i)));
    vecAllDims[i] = createDimension(*dimensionXML);
  }
  VecIMDDimension_sptr vecNonMappedDims = vecAllDims;
  Poco::XML::Element *xDimensionElement = geometryXMLElement->getChildElement(
      MDGeometryXMLDefinitions::workspaceXDimensionElementName());
  std::string xDimId =
      xDimensionElement
          ->getChildElement(
                MDGeometryXMLDefinitions::workspaceRefDimensionElementName())
          ->innerText();
  if (!xDimId.empty()) {
    Iterator xDimensionIt =
        find_if(vecAllDims.begin(), vecAllDims.end(), findID(xDimId));
    if (xDimensionIt == vecAllDims.end()) {
      throw std::invalid_argument("Cannot determine x-dimension mapping.");
    }
    m_xDimension = *xDimensionIt;
    vecNonMappedDims.erase(std::remove_if(
        vecNonMappedDims.begin(), vecNonMappedDims.end(), findID(xDimId)));
  }

  Poco::XML::Element *yDimensionElement = geometryXMLElement->getChildElement(
      MDGeometryXMLDefinitions::workspaceYDimensionElementName());
  std::string yDimId =
      yDimensionElement
          ->getChildElement(
                MDGeometryXMLDefinitions::workspaceRefDimensionElementName())
          ->innerText();

  if (!yDimId.empty()) {
    Iterator yDimensionIt =
        find_if(vecAllDims.begin(), vecAllDims.end(), findID(yDimId));
    if (yDimensionIt == vecAllDims.end()) {
      throw std::invalid_argument("Cannot determine y-dimension mapping.");
    }
    m_yDimension = *yDimensionIt;
    vecNonMappedDims.erase(std::remove_if(
        vecNonMappedDims.begin(), vecNonMappedDims.end(), findID(yDimId)));
  }

  Poco::XML::Element *zDimensionElement = geometryXMLElement->getChildElement(
      MDGeometryXMLDefinitions::workspaceZDimensionElementName());
  std::string zDimId =
      zDimensionElement
          ->getChildElement(
                MDGeometryXMLDefinitions::workspaceRefDimensionElementName())
          ->innerText();

  if (!zDimId.empty()) {
    Iterator zDimensionIt =
        find_if(vecAllDims.begin(), vecAllDims.end(), findID(zDimId));
    if (zDimensionIt == vecAllDims.end()) {
      throw std::invalid_argument("Cannot determine z-dimension mapping.");
    }
    m_zDimension = *zDimensionIt;
    vecNonMappedDims.erase(std::remove_if(
        vecNonMappedDims.begin(), vecNonMappedDims.end(), findID(zDimId)));
  }

  Poco::XML::Element *tDimensionElement = geometryXMLElement->getChildElement(
      MDGeometryXMLDefinitions::workspaceTDimensionElementName());
  std::string tDimId =
      tDimensionElement
          ->getChildElement(
                MDGeometryXMLDefinitions::workspaceRefDimensionElementName())
          ->innerText();
  if (!tDimId.empty()) {
    Iterator tDimensionIt =
        find_if(vecAllDims.begin(), vecAllDims.end(), findID(tDimId));
    if (tDimensionIt == vecAllDims.end()) {
      throw std::invalid_argument("Cannot determine t-dimension mapping.");
    }
    m_tDimension = *tDimensionIt;
    if (!vecNonMappedDims.empty()) {
      vecNonMappedDims.erase(std::remove_if(
          vecNonMappedDims.begin(), vecNonMappedDims.end(), findID(tDimId)));
    }
  }
  m_vecNonMappedDims = vecNonMappedDims; // Copy with strong guarantee.
  m_vecAllDims = vecAllDims;
  m_executed = true;
}

/**
Constructor
@param xmlToProcess : vtkDataSet to process
*/
MDGeometryXMLParser::MDGeometryXMLParser(const std::string &xmlToProcess)
    : m_executed(false), m_xmlToProcess(xmlToProcess) {}

/**
Constructor
*/
MDGeometryXMLParser::MDGeometryXMLParser()
    : m_executed(false), m_xmlToProcess("") {}

/**
Destructor
*/
MDGeometryXMLParser::~MDGeometryXMLParser() {}

/**
Getter for x dimension
@return x dimension.
*/
Mantid::Geometry::IMDDimension_sptr MDGeometryXMLParser::getXDimension() const {
  validate();
  return m_xDimension;
}

/**
Getter for y dimension
@return y dimension.
*/
Mantid::Geometry::IMDDimension_sptr MDGeometryXMLParser::getYDimension() const {
  validate();
  return m_yDimension;
}

/**
Getter for z dimension
@return z dimension.
*/
Mantid::Geometry::IMDDimension_sptr MDGeometryXMLParser::getZDimension() const {
  validate();
  return m_zDimension;
}

/**
Getter for t dimension
@return t dimension.
*/
Mantid::Geometry::IMDDimension_sptr MDGeometryXMLParser::getTDimension() const {
  validate();
  return m_tDimension;
}

/**
 Getter for all those dimensions which are not mapped.
 @return collection of non-mapped dimensions parsed.
*/
Mantid::Geometry::VecIMDDimension_sptr
MDGeometryXMLParser::getNonMappedDimensions() const {
  validate();
  return m_vecNonMappedDims;
}

/**
 Getter for all those dimensions which are not integrated.
 @return collection of non integrated dimensions parsed.
*/
Mantid::Geometry::VecIMDDimension_sptr
MDGeometryXMLParser::getNonIntegratedDimensions() const {
  validate();
  Mantid::Geometry::VecIMDDimension_sptr temp = m_vecAllDims;
  temp.erase(std::remove_if(temp.begin(), temp.end(), findIntegrated()),
             temp.end());
  return temp;
}

/**
Getter for all those dimensions which are integrated.
@return collection of non integrated dimensions parsed.
*/
Mantid::Geometry::VecIMDDimension_sptr
MDGeometryXMLParser::getIntegratedDimensions() const {
  validate();
  Mantid::Geometry::VecIMDDimension_sptr temp = m_vecAllDims;
  temp.erase(
      std::remove_if(temp.begin(), temp.end(), std::not1(findIntegrated())),
      temp.end());
  return temp;
}

/**
Getter for all dimensions parsed.
@return collection of all dimensions parsed.
*/
Mantid::Geometry::VecIMDDimension_sptr
MDGeometryXMLParser::getAllDimensions() const {
  validate();
  return m_vecAllDims;
}

/**
 Determine wheter x dimension is present.
 @return true if available.
*/
bool MDGeometryXMLParser::hasXDimension() const {
  validate();
  return NULL != m_xDimension.get();
}

/**
 Determine wheter y dimension is present.
 @return true if available.
*/
bool MDGeometryXMLParser::hasYDimension() const {
  validate();
  return NULL != m_yDimension.get();
}

/**
 Determine wheter z dimension is present.
 @return true if available.
*/
bool MDGeometryXMLParser::hasZDimension() const {
  validate();
  return NULL != m_zDimension.get();
}

/**
 Determine wheter t dimension is present.
 @return true if available.
*/
bool MDGeometryXMLParser::hasTDimension() const {
  validate();
  return NULL != m_tDimension.get();
}

/**
Setter for the root element.
@param elementName : name of the element containing xml dimensions. Usually
"Dimensions" unless xml snippet passed in directly, in which case do not set.
*/
void MDGeometryXMLParser::SetRootNodeCheck(std::string elementName) {
  m_rootNodeName = elementName;
}

/**
Assignement operator
@param other : existing MDGeometryXMLParser to assign from.
*/
MDGeometryXMLParser &MDGeometryXMLParser::
operator=(const MDGeometryXMLParser &other) {
  if (this != &other) {
    m_executed = other.m_executed;
    m_rootNodeName = other.m_rootNodeName;
    m_vecNonMappedDims = other.m_vecNonMappedDims;
    m_vecAllDims = other.m_vecAllDims;
    m_xDimension = other.m_xDimension;
    m_yDimension = other.m_yDimension;
    m_zDimension = other.m_zDimension;
    m_tDimension = other.m_tDimension;
  }
  return *this;
}

/**
Copy constructor
@param other : existing MDGeometryXMLParser to assign from.
*/
MDGeometryXMLParser::MDGeometryXMLParser(const MDGeometryXMLParser &other)
    : m_executed(other.m_executed), m_rootNodeName(other.m_rootNodeName),
      m_vecNonMappedDims(other.m_vecNonMappedDims),
      m_vecAllDims(other.m_vecAllDims), m_xDimension(other.m_xDimension),
      m_yDimension(other.m_yDimension), m_zDimension(other.m_zDimension),
      m_tDimension(other.m_tDimension) {}

/**
Determines whether query dimension is the x dimension.
@param candidate : query dimension.
@return true if matches.
*/
bool MDGeometryXMLParser::isXDimension(
    Mantid::Geometry::IMDDimension_sptr candidate) const {
  validate();
  bool bResult = false;
  if (hasXDimension()) {
    if (candidate->getDimensionId() == m_xDimension->getDimensionId()) {
      bResult = true;
    }
  }
  return bResult;
}

/**
Determines whether query dimension is the y dimension.
@param candidate : query dimension.
@return true if matches.
*/
bool MDGeometryXMLParser::isYDimension(
    Mantid::Geometry::IMDDimension_sptr candidate) const {
  validate();
  bool bResult = false;
  if (hasYDimension()) {
    if (candidate->getDimensionId() == m_yDimension->getDimensionId()) {
      bResult = true;
    }
  }
  return bResult;
}

/**
Determines whether query dimension is the z dimension.
@param candidate : query dimension.
@return true if matches.
*/
bool MDGeometryXMLParser::isZDimension(
    Mantid::Geometry::IMDDimension_sptr candidate) const {
  validate();
  bool bResult = false;
  if (hasZDimension()) {
    if (candidate->getDimensionId() == m_zDimension->getDimensionId()) {
      bResult = true;
    }
  }
  return bResult;
}

/**
Determines whether query dimension is the t dimension.
@param candidate : query dimension.
@return true if matches.
*/
bool MDGeometryXMLParser::isTDimension(
    Mantid::Geometry::IMDDimension_sptr candidate) const {
  validate();
  bool bResult = false;
  if (hasTDimension()) {
    if (candidate->getDimensionId() == m_tDimension->getDimensionId()) {
      bResult = true;
    }
  }
  return bResult;
}
}
}
