//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FitMD.h"

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/FunctionDomainMD.h"
#include "MantidAPI/IFunctionValues.h"
#include "MantidAPI/IFunctionMD.h"

#include <algorithm>

namespace Mantid
{
namespace CurveFitting
{

  /// Create a domain from the input workspace
  void FitMD::createDomain(
    boost::shared_ptr<API::FunctionDomain>& domain, 
    boost::shared_ptr<API::IFunctionValues>& ivalues, size_t i0)
  {
    UNUSED_ARG(i0);
    if (m_workspacePropertyNames.empty())
    {
      throw std::runtime_error("Cannot create FunctionDomainMD: no workspace given");
    }
    // get the workspace 
    API::Workspace_sptr ws = m_manager->getProperty(m_workspacePropertyNames[0]);
    m_IMDWorkspace = boost::dynamic_pointer_cast<API::IMDWorkspace>(ws);
    if (!m_IMDWorkspace)
    {
      throw std::invalid_argument("InputWorkspace must be a IMDWorkspace.");
    }

    API::FunctionDomainMD* dmd = new API::FunctionDomainMD(m_IMDWorkspace);
    domain.reset(dmd);
    auto values = new API::FunctionValues(*domain);
    ivalues.reset(values);

    auto iter = dmd->getNextIterator();
    size_t i = 0;
    while(iter)
    {
      values->setFitData(i,iter->getNormalizedSignal());
      double err = iter->getNormalizedError();
      if (err <= 0.0) err = 1.0;
      values->setFitWeight(i,1/err);
      iter = dmd->getNextIterator();
      ++i;
    };

    dmd->reset();
  }

  /// Return the size of the domain to be created.
  size_t FitMD::getDomainSize() const
  {
    return 0;
  }


} // namespace Algorithm
} // namespace Mantid
