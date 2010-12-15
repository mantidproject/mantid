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
  this->m_spWorkspace = workspace;
}


/// Creat ethe xml string correponding to the set values.
std::string RebinningXMLGenerator::createXMLString() const
{
  //Builders must always verify the inputs they have been given.
  if(this->m_spFunction.get() == NULL)
  {
    throw std::runtime_error("No ImplicitFunction provided");
  }
  if(this->m_spWorkspace.get() == NULL)
  {
    throw std::runtime_error("No Workspace provided. Can do nothing more.");
  }
  if(true == m_spWorkspace->getGeometryXML().empty())
  {
    throw std::runtime_error("No geometry provided on workspace.");
  }
  if(true == m_spWorkspace->getWSLocation().empty())
  {
    throw std::runtime_error("No workspace location provided on workspace.");
  }
  if(true == m_spWorkspace->getName().empty())
  {
    throw std::runtime_error("No workspace name provided on workspace.");
  }

  return std::string("<MDInstruction>" + m_spWorkspace->getName() + m_spWorkspace->getWSLocation() + m_spWorkspace->getGeometryXML() + m_spFunction->toXMLString() + "</MDInstruction>");
}

}
}
