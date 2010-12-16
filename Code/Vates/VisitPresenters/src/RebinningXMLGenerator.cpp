#include <MantidAPI/ImplicitFunction.h>
#include <boost/shared_ptr.hpp>
#include <MantidAPI/IMDWorkspace.h>
#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include <MantidVisitPresenters/RebinningXMLGenerator.h>
#include <MantidVisitPresenters/RebinningCutterXMLDefinitions.h>

namespace Mantid
{
namespace VATES
{

void RebinningXMLGenerator::setImplicitFunction(boost::shared_ptr<const Mantid::API::ImplicitFunction> spFunction)
{
  this->m_spFunction = spFunction;
}

/// Set the workspace name to apply.
void RebinningXMLGenerator::setWorkspace(boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace)
{
  this->m_wsName =  XMLDefinitions::workspaceNameXMLTagStart + workspace->getName() + XMLDefinitions::workspaceNameXMLTagEnd;
  this->m_wsLocation =   XMLDefinitions::workspaceLocationXMLTagStart + workspace->getWSLocation() + XMLDefinitions::workspaceLocationXMLTagEnd;
  this->m_geomXML = workspace->getGeometryXML();
}

void RebinningXMLGenerator::setWorkspaceName(std::string wsName)
{
  this->m_wsName =   std::string(XMLDefinitions::workspaceNameXMLTagStart  + wsName + XMLDefinitions::workspaceNameXMLTagEnd);
}

void RebinningXMLGenerator::setWorkspaceLocation(std::string wsLocation)
{
  this->m_wsLocation =   std::string(XMLDefinitions::workspaceLocationXMLTagStart  + wsLocation + XMLDefinitions::workspaceLocationXMLTagEnd );
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
  if(this->m_wsLocation == (XMLDefinitions::workspaceLocationXMLTagStart + XMLDefinitions::workspaceLocationXMLTagEnd))
  {
    throw std::runtime_error("No workspace location provided on workspace.");
  }
  if(this->m_wsName == (XMLDefinitions::workspaceNameXMLTagStart + XMLDefinitions::workspaceNameXMLTagEnd))
  {
    throw std::runtime_error("No workspace name provided on workspace.");
  }

  return std::string(XMLDefinitions::workspaceInstructionXMLTagStart  + m_wsName + m_wsLocation + m_geomXML + m_spFunction->toXMLString() + XMLDefinitions::workspaceInstructionXMLTagEnd);
}

}
}
