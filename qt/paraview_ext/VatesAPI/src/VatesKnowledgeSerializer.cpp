// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/VatesKnowledgeSerializer.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidVatesAPI/VatesXMLDefinitions.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <utility>

using Mantid::Geometry::MDGeometryXMLDefinitions;
namespace Mantid {
namespace VATES {

VatesKnowledgeSerializer::VatesKnowledgeSerializer()
    : m_wsLocationXML(""), m_wsNameXML(""), m_wsName(""), m_geomXML("") {}

void VatesKnowledgeSerializer::setImplicitFunction(
    boost::shared_ptr<const Mantid::Geometry::MDImplicitFunction> spFunction) {
  this->m_spFunction = std::move(spFunction);
}

/// Set the workspace name to apply.
void VatesKnowledgeSerializer::setWorkspace(
    const Mantid::API::IMDWorkspace &workspace) {

  this->m_wsNameXML = MDGeometryXMLDefinitions::workspaceNameXMLTagStart() +
                      workspace.getName() +
                      MDGeometryXMLDefinitions::workspaceNameXMLTagEnd();
  this->m_wsLocationXML =
      MDGeometryXMLDefinitions::workspaceLocationXMLTagStart() + "" +
      MDGeometryXMLDefinitions::workspaceLocationXMLTagEnd();
  this->m_geomXML = workspace.getGeometryXML();
}

void VatesKnowledgeSerializer::setWorkspaceName(const std::string &wsName) {
  this->m_wsName = wsName;
  this->m_wsNameXML =
      std::string(MDGeometryXMLDefinitions::workspaceNameXMLTagStart() +
                  wsName + MDGeometryXMLDefinitions::workspaceNameXMLTagEnd());
}

void VatesKnowledgeSerializer::setGeometryXML(const std::string &geomXML) {
  this->m_geomXML = geomXML;
}

/// Create the xml string correponding to the set values.
std::string VatesKnowledgeSerializer::createXMLString() const {

  if (true == this->m_geomXML.empty()) {
    throw std::runtime_error("No geometry provided on workspace.");
  }

  if (this->m_wsNameXML ==
      (MDGeometryXMLDefinitions::workspaceNameXMLTagStart() +
       MDGeometryXMLDefinitions::workspaceNameXMLTagEnd())) {
    throw std::runtime_error("No workspace name provided on workspace.");
  }
  // Check to see if a function has been provided.
  if (m_spFunction) {
    return std::string(
        MDGeometryXMLDefinitions::workspaceInstructionXMLTagStart() +
        m_wsNameXML + m_wsLocationXML + m_geomXML +
        m_spFunction->toXMLString() +
        MDGeometryXMLDefinitions::workspaceInstructionXMLTagEnd());
  } else {
    // Functions are optional, so don't provide them as part of the completed
    // xml if not present.
    return std::string(
        MDGeometryXMLDefinitions::workspaceInstructionXMLTagStart() +
        m_wsNameXML + m_wsLocationXML + m_geomXML +
        MDGeometryXMLDefinitions::workspaceInstructionXMLTagEnd());
  }
}

const std::string &VatesKnowledgeSerializer::getWorkspaceName() const {
  return this->m_wsName;
}

const std::string &VatesKnowledgeSerializer::getWorkspaceGeometry() const {
  return this->m_geomXML;
}

bool VatesKnowledgeSerializer::hasFunctionInfo() const {
  return static_cast<bool>(m_spFunction);
}

bool VatesKnowledgeSerializer::hasGeometryInfo() const {
  return !m_geomXML.empty() && !m_wsName.empty();
}
} // namespace VATES
} // namespace Mantid
