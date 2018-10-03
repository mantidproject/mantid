// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * XmlHandler.cpp
 *
 *  Created on: Nov 5, 2015
 *      Author: rhf
 */

#include "MantidDataHandling/XmlHandler.h"

namespace Mantid {
namespace DataHandling {

XmlHandler::XmlHandler() {
  // TODO Auto-generated constructor stub
}

XmlHandler::XmlHandler(std::string filename) {

  std::ifstream in(filename);
  Poco::XML::InputSource src(in);
  Poco::XML::DOMParser parser;
  pDoc = parser.parse(&src);
}

XmlHandler::~XmlHandler() {
  // TODO Auto-generated destructor stub
  // pDoc->release();
}

/**
 * Build dictionary {string : string } of all tags in the dictionary
 * Composed tags: / replaced by _
 *
 */
std::map<std::string, std::string>
XmlHandler::get_metadata(const std::string &tag_to_ignore) {
  std::map<std::string, std::string> metadata;

  Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
  Poco::XML::Node *pNode = it.nextNode();

  while (pNode) {
    if (pNode->childNodes()->length() == 1 &&
        pNode->nodeName() != tag_to_ignore) {
      std::string key =
          pNode->parentNode()->nodeName() + "/" + pNode->nodeName();
      std::string value = pNode->innerText();
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

std::map<std::string, std::string>
XmlHandler::get_attributes_from_tag(const std::string &xpath) {
  std::map<std::string, std::string> attributes_map;
  Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
  Poco::XML::Node *pNode = it.nextNode();
  Poco::XML::Node *detectorNode = pNode->getNodeByPath(xpath);
  if (detectorNode) {
    Poco::XML::NamedNodeMap *attributes = detectorNode->attributes();
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
    if (pNode->childNodes()->length() == 1) {
      subnodes.push_back(pNode->nodeName());
    }
    pNode = it.nextNode();
  }
  return subnodes;
}

} /* namespace DataHandling */
} /* namespace Mantid */
