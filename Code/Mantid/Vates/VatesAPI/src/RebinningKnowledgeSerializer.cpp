#include <MantidAPI/ImplicitFunction.h>
#include <boost/shared_ptr.hpp>
#include <MantidAPI/IMDWorkspace.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"

using Mantid::Geometry::MDGeometryXMLDefinitions;
namespace Mantid
{
namespace VATES
{

RebinningKnowledgeSerializer::RebinningKnowledgeSerializer(LocationPolicy locationPolicy) : 
  m_spFunction(), 
  m_wsLocationXML(""), 
  m_wsNameXML(""), 
  m_wsLocation(""), 
  m_wsName(""), 
  m_geomXML(""),
  m_locationPolicy(locationPolicy)
{
}

void RebinningKnowledgeSerializer::setImplicitFunction(boost::shared_ptr<const Mantid::API::ImplicitFunction> spFunction)
{
  this->m_spFunction = spFunction;
}

/// Set the workspace name to apply.
void RebinningKnowledgeSerializer::setWorkspace(boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace)
{
  
  this->m_wsNameXML =  MDGeometryXMLDefinitions::workspaceNameXMLTagStart() + workspace->getName() + MDGeometryXMLDefinitions::workspaceNameXMLTagEnd();
  this->m_wsLocation = workspace->getWSLocation();
  this->m_wsLocationXML =   MDGeometryXMLDefinitions::workspaceLocationXMLTagStart() + this->m_wsLocation + MDGeometryXMLDefinitions::workspaceLocationXMLTagEnd();
  this->m_geomXML = workspace->getGeometryXML();
}

void RebinningKnowledgeSerializer::setWorkspaceName(std::string wsName)
{
  this->m_wsName = wsName;
  this->m_wsNameXML =   std::string(MDGeometryXMLDefinitions::workspaceNameXMLTagStart()  + wsName + MDGeometryXMLDefinitions::workspaceNameXMLTagEnd());
}

void RebinningKnowledgeSerializer::setWorkspaceLocation(std::string wsLocation)
{

  this->m_wsLocation = wsLocation;
  this->m_wsLocationXML =   std::string(MDGeometryXMLDefinitions::workspaceLocationXMLTagStart()  + wsLocation + MDGeometryXMLDefinitions::workspaceLocationXMLTagEnd() );
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
  if(LocationMandatory == this->m_locationPolicy) //Only if it is stated that a location must be provided, do we apply the checking.
  {
    if(this->m_wsLocationXML == (MDGeometryXMLDefinitions::workspaceLocationXMLTagStart() + MDGeometryXMLDefinitions::workspaceLocationXMLTagEnd()))
    {
      throw std::runtime_error("No workspace location provided on workspace.");
    }
  }
  if(this->m_wsNameXML == (MDGeometryXMLDefinitions::workspaceNameXMLTagStart() + MDGeometryXMLDefinitions::workspaceNameXMLTagEnd()))
  {
    throw std::runtime_error("No workspace name provided on workspace.");
  }

  //Check to see if a function has been provided.
  if(NULL != m_spFunction.get())
  {
    return std::string(MDGeometryXMLDefinitions::workspaceInstructionXMLTagStart()  + m_wsNameXML + m_wsLocationXML + m_geomXML + m_spFunction->toXMLString() + MDGeometryXMLDefinitions::workspaceInstructionXMLTagEnd());
  }
  else
  {
    //Functions are optional, so don't provide them as part of the completed xml if not present.
    return std::string(MDGeometryXMLDefinitions::workspaceInstructionXMLTagStart()  + m_wsNameXML + m_wsLocationXML + m_geomXML + MDGeometryXMLDefinitions::workspaceInstructionXMLTagEnd());
  }
}

const std::string& RebinningKnowledgeSerializer::getWorkspaceLocation() const
{
  return this->m_wsLocation;
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
   return  !m_geomXML.empty() && !m_wsLocation.empty() && !m_wsName.empty();
 }


}
}
