//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FitMD.h"
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidCurveFitting/EmptyValues.h"

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/FunctionDomainMD.h"
#include "MantidAPI/IFunctionValues.h"
#include "MantidAPI/IFunctionMD.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/PropertyWithValue.h"

#include <algorithm>

namespace Mantid
{
namespace CurveFitting
{

  using namespace Kernel;

  /**
   * Constructor
   */
  FitMD::FitMD(IPropertyManager* fit, const std::string& workspacePropertyName, DomainType domainType)
    :IDomainCreator(fit,std::vector<std::string>(1,workspacePropertyName),domainType)
  {
    if (m_workspacePropertyNames.empty())
    {
      throw std::runtime_error("Cannot create FitMD: no workspace given");
    }
    m_workspacePropertyName = m_workspacePropertyNames[0];
  }
  /**
   * Declare properties that specify the dataset within the workspace to fit to.
   * @param domainIndex :: Index of created domain in a composite domain or 0 in single domain case
   */
  void FitMD::declareDatasetProperties(const std::string& suffix,bool addProp)
  {
    if ( m_domainType != Simple )
    {
      m_maxSizePropertyName = "MaxSize" + suffix;
      auto mustBePositive = boost::shared_ptr< BoundedValidator<int> >( new BoundedValidator<int>() );
      mustBePositive->setLower(1);
      declareProperty(new PropertyWithValue<int>(m_maxSizePropertyName,1, mustBePositive),
                      "The maximum number of values per a simple domain.");
    }
  }

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

  /**
   * Set all parameters
   */
  void FitMD::setParameters() const
  {
    // if property manager is set overwrite any set parameters
    if ( m_manager )
    {

      // get the workspace 
      API::Workspace_sptr ws = m_manager->getProperty(m_workspacePropertyName);
      m_IMDWorkspace = boost::dynamic_pointer_cast<API::IMDWorkspace>(ws);
      if (!m_IMDWorkspace)
      {
        throw std::invalid_argument("InputWorkspace must be a MatrixWorkspace.");
      }
      if ( m_domainType != Simple )
      {
        const int maxSizeInt = m_manager->getProperty( m_maxSizePropertyName );
        m_maxSize = static_cast<size_t>( maxSizeInt );
      }
    }
  }

  /// Return the size of the domain to be created.
  size_t FitMD::getDomainSize() const
  {
    setParameters();
    auto iterator = m_IMDWorkspace->createIterator();
    size_t n = iterator->getDataSize();
    delete iterator;
    return n;
  }


} // namespace Algorithm
} // namespace Mantid
