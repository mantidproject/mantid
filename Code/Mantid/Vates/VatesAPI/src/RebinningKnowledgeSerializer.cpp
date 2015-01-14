#include <MantidGeometry/MDGeometry/MDImplicitFunction.h>
#include <boost/shared_ptr.hpp>
#include <MantidAPI/IMDWorkspace.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"
#include "MantidVatesAPI/VatesXMLDefinitions.h"

using Mantid::Geometry::MDGeometryXMLDefinitions;
namespace Mantid
{
namespace VATES
{

RebinningKnowledgeSerializer::RebinningKnowledgeSerializer(LocationPolicy locationPolicy) : 
  m_wsLocationXML(""), 
  m_wsNameXML(""), 
  m_wsName(""), 
  m_geomXML(""),
  m_locationPolicy(locationPolicy)
{
}

void RebinningKnowledgeSerializer::setImplicitFunction(boost::shared_ptr<const Mantid::Geometry::MDImplicitFunction> spFunction)
{
  this->m_spFunction = spFunction;
}

/// Set the workspace name to apply.
void RebinningKnowledgeSerializer::setWorkspace(boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace)
{
  
  this->m_wsNameXML =  MDGeometryXMLDefinitions::workspaceNameXMLTagStart() + workspace->getName() + MDGeometryXMLDefinitions::workspaceNameXMLTagEnd();
  this->m_wsLocationXML =   MDGeometryXMLDefinitions::workspaceLocationXMLTagStart() + "" + MDGeometryXMLDefinitions::workspaceLocationXMLTagEnd();
  this->m_geomXML = workspace->getGeometryXML();
}

void RebinningKnowledgeSerializer::setWorkspaceName(std::string wsName)
{
  this->m_wsName = wsName;
  this->m_wsNameXML =   std::string(MDGeometryXMLDefinitions::workspaceNameXMLTagStart()  + wsName + MDGeometryXMLDefinitions::workspaceNameXMLTagEnd());
}

void RebinningKnowledgeSerializer::setGeometryXML(std::string geomXML)
{
  this->m_geomXML = geomXML;
}

/// Create the xml string correponding to the set values.
std::string RebinningKnowledgeSerializer::createXMLString() const
{

  if(true == this->m_geomXML.empty())
  {
    throw std::runtime_error("No geometry provided on workspace.");
  }

  if(this->m_wsNameXML == (MDGeometryXMLDefinitions::workspaceNameXMLTagStart() + MDGeometryXMLDefinitions::workspaceNameXMLTagEnd()))
  {
    throw std::runtime_error("No workspace name provided on workspace.");
  }
  //Check to see if a function has been provided.
  if(m_spFunction != NULL)
  {
    return std::string(MDGeometryXMLDefinitions::workspaceInstructionXMLTagStart()  + m_wsNameXML + m_wsLocationXML + m_geomXML + m_spFunction->toXMLString() + MDGeometryXMLDefinitions::workspaceInstructionXMLTagEnd());
  }
  else
  {
    //Functions are optional, so don't provide them as part of the completed xml if not present.
    return std::string(MDGeometryXMLDefinitions::workspaceInstructionXMLTagStart()  + m_wsNameXML + m_wsLocationXML + m_geomXML + MDGeometryXMLDefinitions::workspaceInstructionXMLTagEnd());
  }
}

const std::string& RebinningKnowledgeSerializer::getWorkspaceName() const
{
  return this->m_wsName;
}

const std::string& RebinningKnowledgeSerializer::getWorkspaceGeometry() const
{
  return this->m_geomXML;
}

 bool RebinningKnowledgeSerializer::hasFunctionInfo() const
 {
   return NULL != m_spFunction.get();
 }

 bool RebinningKnowledgeSerializer::hasGeometryInfo() const
 {
   return  !m_geomXML.empty() && !m_wsName.empty();
 }


}
}
