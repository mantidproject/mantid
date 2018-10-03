// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <sstream>

#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/functional/hash.hpp>

namespace Mantid {
namespace Geometry {

/**
 Add an ordinary dimension.
 @param dimensionToAdd :: The dimension to add to the geometry.
 @return true if addition does not cause an overwrite successful.
 */
template <typename CheckDimensionPolicy>
bool MDGeometryBuilderXML<CheckDimensionPolicy>::addOrdinaryDimension(
    IMDDimension_const_sptr dimensionToAdd) const {
  bool bAdded = false; // Addition fails by default.
  if (dimensionToAdd) {
    const std::string dimensionId = dimensionToAdd->getDimensionId();
    auto location =
        std::find_if(m_vecDimensions.cbegin(), m_vecDimensions.cend(),
                     [&dimensionId](const IMDDimension_const_sptr &b) {
                       return dimensionId == b->getDimensionId();
                     });
    if (location == m_vecDimensions.cend()) {
      m_vecDimensions.push_back(std::move(dimensionToAdd));
      bAdded = true;
      m_changed = true;
    }
  }
  return bAdded;
}

/**
 Add many ordinary dimensions.
 @param manyDims :: Collection of dimensions to add as ordinary dimensions.
 */
template <typename CheckDimensionPolicy>
void MDGeometryBuilderXML<CheckDimensionPolicy>::addManyOrdinaryDimensions(
    VecIMDDimension_sptr manyDims) const {
  for (auto &manyDim : manyDims) {
    addOrdinaryDimension(manyDim);
  }
}

template <typename CheckDimensionPolicy>
MDGeometryBuilderXML<CheckDimensionPolicy>::MDGeometryBuilderXML(
    const MDGeometryBuilderXML<CheckDimensionPolicy> &other) {
  m_vecDimensions = other.m_vecDimensions;
  m_spXDimension = other.m_spXDimension;
  m_spYDimension = other.m_spYDimension;
  m_spZDimension = other.m_spZDimension;
  m_spTDimension = other.m_spTDimension;
  m_changed = other.m_changed;
  m_lastResult = other.m_lastResult;
}

template <typename CheckDimensionPolicy>
MDGeometryBuilderXML<CheckDimensionPolicy> &
MDGeometryBuilderXML<CheckDimensionPolicy>::
operator=(const MDGeometryBuilderXML<CheckDimensionPolicy> &other) {
  if (this != &other) {
    m_vecDimensions = other.m_vecDimensions;
    m_spXDimension = other.m_spXDimension;
    m_spYDimension = other.m_spYDimension;
    m_spZDimension = other.m_spZDimension;
    m_spTDimension = other.m_spTDimension;
    m_changed = other.m_changed;
    m_lastResult = other.m_lastResult;
  }
  return *this;
}

/**
 Check that the addition of dimensions conforms to the defined policy.
 @param dimensionToAdd :: The dimension to add to the geometry.
 @return nothing. Throws on failure.
 */
template <typename CheckDimensionPolicy>
void MDGeometryBuilderXML<CheckDimensionPolicy>::applyPolicyChecking(
    const IMDDimension &dimensionToAdd) const {
  CheckDimensionPolicy policy;
  policy(dimensionToAdd);
}

/**
 Add an x dimension.
 @param dimension :: The dimension to add to the geometry.
 @return true if addition was successful.
 */
template <typename CheckDimensionPolicy>
bool MDGeometryBuilderXML<CheckDimensionPolicy>::addXDimension(
    IMDDimension_const_sptr dimension) const {

  bool bAdded = false;
  if (dimension) {
    applyPolicyChecking(*dimension);
    addOrdinaryDimension(dimension);
    m_spXDimension = std::move(dimension);
    m_changed = true;
    bAdded = true;
  }
  return bAdded;
}

/**
 Add an y dimension.
 @param dimension :: The dimension to add to the geometry.
 @return true if addition was successful.
 */
template <typename CheckDimensionPolicy>
bool MDGeometryBuilderXML<CheckDimensionPolicy>::addYDimension(
    IMDDimension_const_sptr dimension) const {

  bool bAdded = false;
  if (dimension) {
    applyPolicyChecking(*dimension);
    addOrdinaryDimension(dimension);
    m_spYDimension = std::move(dimension);
    m_changed = true;
    bAdded = true;
  }
  return bAdded;
}

/**
 Add an z dimension.
 @param dimension :: The dimension to add to the geometry.
 @return true if addition was successful.
 */
template <typename CheckDimensionPolicy>
bool MDGeometryBuilderXML<CheckDimensionPolicy>::addZDimension(
    IMDDimension_const_sptr dimension) const {
  bool bAdded = false;
  if (dimension) {
    applyPolicyChecking(*dimension);
    addOrdinaryDimension(dimension);
    m_spZDimension = std::move(dimension);
    m_changed = true;
    bAdded = true;
  }
  return bAdded;
}

/**
 Add an t dimension.
 @param dimension :: The dimension to add to the geometry.
 @return true if addition was successful.
 */
template <typename CheckDimensionPolicy>
bool MDGeometryBuilderXML<CheckDimensionPolicy>::addTDimension(
    IMDDimension_const_sptr dimension) const {

  bool bAdded = false;
  if (dimension) {
    applyPolicyChecking(*dimension);
    addOrdinaryDimension(dimension);
    m_spTDimension = std::move(dimension);
    m_changed = true;
    bAdded = true;
  }
  return bAdded;
}

/**
Builder creational method. Processes added dimensions. creates xml string.
@return xmlstring.
*/
template <typename CheckDimensionPolicy>
const std::string &MDGeometryBuilderXML<CheckDimensionPolicy>::create() const {
  using namespace Poco::XML;
  if (m_changed) {
    // Create the root element for this fragment.
    AutoPtr<Document> pDoc = new Document;
    AutoPtr<Element> dimensionSetElement = pDoc->createElement("DimensionSet");
    pDoc->appendChild(dimensionSetElement);

    // Loop through dimensions and generate xml for each.
    std::string dimensionXMLString;

    for (auto it = m_vecDimensions.begin(); it != m_vecDimensions.end(); ++it) {
      dimensionXMLString += (*it)->toXMLString();
    }

    // Pass dimensions to dimension set.
    AutoPtr<Text> percents = pDoc->createTextNode("%s");
    dimensionSetElement->appendChild(percents);

    // x-dimension mapping.
    AutoPtr<Element> xDimensionElement = pDoc->createElement("XDimension");
    AutoPtr<Element> xDimensionIdElement =
        pDoc->createElement("RefDimensionId");
    if (hasXDimension()) {
      std::string xDimensionId = this->m_spXDimension->getDimensionId();
      AutoPtr<Text> idXText = pDoc->createTextNode(xDimensionId);
      xDimensionIdElement->appendChild(idXText);
    }
    xDimensionElement->appendChild(xDimensionIdElement);
    dimensionSetElement->appendChild(xDimensionElement);

    // y-dimension mapping.
    AutoPtr<Element> yDimensionElement = pDoc->createElement("YDimension");
    AutoPtr<Element> yDimensionIdElement =
        pDoc->createElement("RefDimensionId");
    if (hasYDimension()) {
      std::string yDimensionId = this->m_spYDimension->getDimensionId();
      AutoPtr<Text> idYText = pDoc->createTextNode(yDimensionId);
      yDimensionIdElement->appendChild(idYText);
    }
    yDimensionElement->appendChild(yDimensionIdElement);
    dimensionSetElement->appendChild(yDimensionElement);

    // z-dimension mapping.
    AutoPtr<Element> zDimensionElement = pDoc->createElement("ZDimension");
    AutoPtr<Element> zDimensionIdElement =
        pDoc->createElement("RefDimensionId");
    if (hasZDimension()) {
      std::string zDimensionId = this->m_spZDimension->getDimensionId();
      AutoPtr<Text> idZText = pDoc->createTextNode(zDimensionId);
      zDimensionIdElement->appendChild(idZText);
    }
    zDimensionElement->appendChild(zDimensionIdElement);
    dimensionSetElement->appendChild(zDimensionElement);

    // t-dimension mapping.
    AutoPtr<Element> tDimensionElement = pDoc->createElement("TDimension");
    AutoPtr<Element> tDimensionIdElement =
        pDoc->createElement("RefDimensionId");
    if (hasTDimension()) {
      std::string tDimensionId = this->m_spTDimension->getDimensionId();
      AutoPtr<Text> idTText = pDoc->createTextNode(tDimensionId);
      tDimensionIdElement->appendChild(idTText);
    }
    tDimensionElement->appendChild(tDimensionIdElement);
    dimensionSetElement->appendChild(tDimensionElement);

    std::stringstream xmlstream;
    DOMWriter writer;
    writer.writeNode(xmlstream, pDoc);

    m_lastResult = boost::str(boost::format(xmlstream.str().c_str()) %
                              dimensionXMLString.c_str());
    m_changed = false;
  }
  return m_lastResult;
}

template <typename CheckDimensionPolicy>
bool MDGeometryBuilderXML<CheckDimensionPolicy>::hasXDimension() const {
  return nullptr != this->m_spXDimension.get();
}

template <typename CheckDimensionPolicy>
bool MDGeometryBuilderXML<CheckDimensionPolicy>::hasYDimension() const {
  return nullptr != this->m_spYDimension.get();
}

template <typename CheckDimensionPolicy>
bool MDGeometryBuilderXML<CheckDimensionPolicy>::hasZDimension() const {
  return nullptr != this->m_spZDimension.get();
}

template <typename CheckDimensionPolicy>
bool MDGeometryBuilderXML<CheckDimensionPolicy>::hasTDimension() const {
  return nullptr != this->m_spTDimension.get();
}

template <typename CheckDimensionPolicy>
bool MDGeometryBuilderXML<CheckDimensionPolicy>::hasIntegratedTDimension()
    const {
  return hasTDimension() && this->m_spTDimension->getIsIntegrated();
}

/**
 Constructor
 */
template <typename CheckDimensionPolicy>
MDGeometryBuilderXML<CheckDimensionPolicy>::MDGeometryBuilderXML()
    : m_changed(true) {}

/**
 Destructor
 */
template <typename CheckDimensionPolicy>
MDGeometryBuilderXML<CheckDimensionPolicy>::~MDGeometryBuilderXML() {}

// Create a builder that blocks the creation when a integrated dimension is used
// in x, y, z, t mappings.
template class MDGeometryBuilderXML<StrictDimensionPolicy>;
// Create a builder that applies no blocking/checking.
template class MDGeometryBuilderXML<NoDimensionPolicy>;
} // namespace Geometry
} // namespace Mantid
