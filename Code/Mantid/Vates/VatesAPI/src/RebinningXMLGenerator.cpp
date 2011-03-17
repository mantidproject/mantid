#include <MantidAPI/ImplicitFunction.h>
#include <boost/shared_ptr.hpp>
#include <MantidAPI/IMDWorkspace.h>
#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "MantidVatesAPI/RebinningXMLGenerator.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"

namespace Mantid
{
namespace VATES
{

RebinningXMLGenerator::RebinningXMLGenerator() : m_spFunction(), m_wsLocationXML(""), m_wsNameXML(""), m_wsLocation(""), m_wsName(""), m_geomXML("")
{
}

void RebinningXMLGenerator::setImplicitFunction(boost::shared_ptr<const Mantid::API::ImplicitFunction> spFunction)
{
  this->m_spFunction = spFunction;
}

/// Set the workspace name to apply.
void RebinningXMLGenerator::setWorkspace(boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace)
{
  this->m_wsNameXML =  XMLDefinitions::workspaceNameXMLTagStart() + workspace->getName() + XMLDefinitions::workspaceNameXMLTagEnd();
  this->m_wsLocation = workspace->getWSLocation();
  this->m_wsLocationXML =   XMLDefinitions::workspaceLocationXMLTagStart() + this->m_wsLocation + XMLDefinitions::workspaceLocationXMLTagEnd();
  this->m_geomXML = workspace->getGeometryXML();
}

void RebinningXMLGenerator::setWorkspaceName(std::string wsName)
{
  this->m_wsName = wsName;
  this->m_wsNameXML =   std::string(XMLDefinitions::workspaceNameXMLTagStart()  + wsName + XMLDefinitions::workspaceNameXMLTagEnd());
}

void RebinningXMLGenerator::setWorkspaceLocation(std::string wsLocation)
{

  this->m_wsLocation = wsLocation;
  this->m_wsLocationXML =   std::string(XMLDefinitions::workspaceLocationXMLTagStart()  + wsLocation + XMLDefinitions::workspaceLocationXMLTagEnd() );
}

void RebinningXMLGenerator::setGeometryXML(std::string geomXML)
{
  this->m_geomXML = geomXML;
}

/// Create the xml string correponding to the set values.
std::string RebinningXMLGenerator::createXMLString() const
{

  if(true == this->m_geomXML.empty())
  {
    throw std::runtime_error("No geometry provided on workspace.");
  }
  if(this->m_wsLocationXML == (XMLDefinitions::workspaceLocationXMLTagStart() + XMLDefinitions::workspaceLocationXMLTagEnd()))
  {
    throw std::runtime_error("No workspace location provided on workspace.");
  }
  if(this->m_wsNameXML == (XMLDefinitions::workspaceNameXMLTagStart() + XMLDefinitions::workspaceNameXMLTagEnd()))
  {
    throw std::runtime_error("No workspace name provided on workspace.");
  }

  //Check to see if a function has been provided.
  if(NULL != m_spFunction.get())
  {
    return std::string(XMLDefinitions::workspaceInstructionXMLTagStart()  + m_wsNameXML + m_wsLocationXML + m_geomXML + m_spFunction->toXMLString() + XMLDefinitions::workspaceInstructionXMLTagEnd());
  }
  else
  {
    //Functions are optional, so don't provide them as part of the completed xml if not present.
    return std::string(XMLDefinitions::workspaceInstructionXMLTagStart()  + m_wsNameXML + m_wsLocationXML + m_geomXML + XMLDefinitions::workspaceInstructionXMLTagEnd());
  }
}

const std::string& RebinningXMLGenerator::getWorkspaceLocation() const
{
  return this->m_wsLocation;
}

const std::string& RebinningXMLGenerator::getWorkspaceName() const
{
  return this->m_wsName;
}

const std::string& RebinningXMLGenerator::getWorkspaceGeometry() const
{
  return this->m_geomXML;
}

 bool RebinningXMLGenerator::hasFunctionInfo() const
 {
   return NULL != m_spFunction.get();
 }

 bool RebinningXMLGenerator::hasGeometryInfo() const
 {
   return  !m_geomXML.empty() && !m_wsLocation.empty() && !m_wsName.empty();
 }


}
}
