#include <MantidVisitPresenters/RebinningXMLGenerator.h>
#include <MantidAPI/ImplicitFunction.h>
#include <boost/shared_ptr.hpp>
#include <MantidAPI/IMDWorkspace.h>
#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"

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
  this->m_wsName = workspace->getName();
  this->m_wsLocation = workspace->getWSLocation();
  this->m_geomXML = workspace->getGeometryXML();
}

void RebinningXMLGenerator::setWorkspaceName(std::string wsName)
{
  this->m_wsName = wsName;
}

void RebinningXMLGenerator::setWorkspaceLocation(std::string wsLocation)
{
  this->m_wsLocation = wsLocation;
}

void RebinningXMLGenerator::setGeometryXML(std::string geomXML)
{
  this->m_geomXML = geomXML;
}

/// Create the xml string correponding to the set values.
std::string RebinningXMLGenerator::createXMLString() const
{

  if(true == m_geomXML.empty())
  {
    throw std::runtime_error("No geometry provided on workspace.");
  }
  if(true == m_wsLocation.empty())
  {
    throw std::runtime_error("No workspace location provided on workspace.");
  }
  if(true == m_wsName.empty())
  {
    throw std::runtime_error("No workspace name provided on workspace.");
  }

  return std::string("<MDInstruction>" + m_wsName + m_wsLocation + m_geomXML + m_spFunction->toXMLString() + "</MDInstruction>");
}

}
}
