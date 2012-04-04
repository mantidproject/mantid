/*WIKI* 


This algorithm fits data in a [[Workspace]] with a function. The function and the initial values for its parameters are set with the Function property. The function must be compatible with the workspace.

Using the Minimizer property, Fit can be set to use different algorithms to perform the minimization. By default if the function's derivatives can be evaluated then Fit uses the GSL Levenberg-Marquardt minimizer. If the function's derivatives cannot be evaluated the GSL Simplex minimizer is used. Also, if one minimizer fails, for example the Levenberg-Marquardt minimizer, Fit may try its luck with a different minimizer. If this happens the user is notified about this and the Minimizer property is updated accordingly.

===Output===

Setting the Output property defines the names of the output workspaces. One of them is a [[TableWorkspace]] with the fitted parameter values.  If the function's derivatives can be evaluated an additional TableWorkspace is returned containing correlation coefficients in %.


*WIKI*/
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
    const std::vector<std::string>& workspacePropetyNames,
    boost::shared_ptr<API::FunctionDomain>& domain, 
    boost::shared_ptr<API::IFunctionValues>& ivalues, size_t i0)
  {
    UNUSED_ARG(i0);
    if (workspacePropetyNames.empty())
    {
      throw std::runtime_error("Cannot create FunctionDomainMD: no workspace given");
    }
    // get the workspace 
    API::Workspace_sptr ws = m_fit->getProperty(workspacePropetyNames[0]);
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

} // namespace Algorithm
} // namespace Mantid
