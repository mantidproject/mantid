// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/XmlHandler.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/SAX/InputSource.h>
#include <boost/algorithm/string/trim.hpp>

#include <fstream>

namespace Mantid::DataHandling {

XmlHandler::XmlHandler(const std::string &filename) {
  std::ifstream in(filename);
  Poco::XML::InputSource src(in);
  Poco::XML::DOMParser parser;
  pDoc = parser.parse(&src);
}

/**
 * Build dictionary {string : string } of all tags in the dictionary
 * Composed tags: / replaced by _
 *
 */
std::map<std::string, std::string> XmlHandler::get_metadata(const std::vector<std::string> &tags_to_ignore) {
  std::map<std::string, std::string> metadata;

  Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
  Poco::XML::Node *pNode = it.nextNode();

  while (pNode) {
    Poco::AutoPtr<Poco::XML::NodeList> children = pNode->childNodes();
    if (children->length() == 1 && std::find(std::begin(tags_to_ignore), std::end(tags_to_ignore), pNode->nodeName()) ==
                                       std::end(tags_to_ignore)) {
      std::string key = pNode->parentNode()->nodeName() + "/" + pNode->nodeName();
      std::string value = pNode->innerText();
      boost::algorithm::trim(value);
      metadata.emplace(key, value);
    }
    pNode = it.nextNode();
  }
  return metadata;
}

std::string XmlHandler::get_text_from_tag(const std::string &xpath) {
  Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
  Poco::XML::Node *pNode = it.nextNode();
  Poco::XML::Node *detectorNode = pNode->getNodeByPath(xpath);
  std::string value;
  if (detectorNode)
    value = detectorNode->innerText();
  return value;
}

std::map<std::string, std::string> XmlHandler::get_attributes_from_tag(const std::string &xpath) {
  std::map<std::string, std::string> attributes_map;
  Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
  Poco::XML::Node *pNode = it.nextNode();
  Poco::XML::Node *detectorNode = pNode->getNodeByPath(xpath);
  if (detectorNode) {
    Poco::AutoPtr<Poco::XML::NamedNodeMap> attributes = detectorNode->attributes();
    for (unsigned int i = 0; i < attributes->length(); i++) {
      Poco::XML::Node *attribute = attributes->item(i);
      attributes_map.emplace(attribute->nodeName(), attribute->nodeValue());
    }
  }
  return attributes_map;
}

/**
 * Returns list of sub-nodes for a xpath node
 * For: xpath = //Data/
 * Returns: Detector , DetectorWing
 */
std::vector<std::string> XmlHandler::get_subnodes(const std::string &xpath) {

  std::vector<std::string> subnodes;
  Poco::XML::Node *xpathNode = pDoc->getNodeByPath(xpath);

  Poco::XML::NodeIterator it(xpathNode, Poco::XML::NodeFilter::SHOW_ELEMENT);
  Poco::XML::Node *pNode = it.nextNode();

  while (pNode) {
    Poco::AutoPtr<Poco::XML::NodeList> children = pNode->childNodes();
    if (children->length() == 1) {
      subnodes.emplace_back(pNode->nodeName());
    }
    pNode = it.nextNode();
  }
  return subnodes;
}

} // namespace Mantid::DataHandling
