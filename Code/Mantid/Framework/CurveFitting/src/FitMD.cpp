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
    :IDomainCreator(fit,std::vector<std::string>(1,workspacePropertyName),domainType),
    m_startIndex(0),m_count(0)
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
      if (addProp && !m_manager->existsProperty(m_maxSizePropertyName))
      {
        auto mustBePositive = boost::shared_ptr< BoundedValidator<int> >( new BoundedValidator<int>() );
        mustBePositive->setLower(1);
        declareProperty(new PropertyWithValue<int>(m_maxSizePropertyName,1, mustBePositive),
          "The maximum number of values per a simple domain.");
      }
    }
  }

  /// Create a domain from the input workspace
  void FitMD::createDomain(
    boost::shared_ptr<API::FunctionDomain>& domain, 
    boost::shared_ptr<API::IFunctionValues>& ivalues, size_t i0)
  {
    UNUSED_ARG(i0);

    setParameters();

    auto iterator = m_IMDWorkspace->createIterator();

    const size_t n = iterator->getDataSize();
    if ( m_domainType != Simple )
    {
      SeqDomain* seqDomain = SeqDomain::create( m_domainType );
      domain.reset( seqDomain );
      if ( n > m_maxSize )
      {
        for(size_t i = 0; i < n; i += m_maxSize)
        {
          if ( i > 0 ) iterator->jumpTo(i);
          FitMD* creator = new FitMD;
          creator->setWorkspace(m_IMDWorkspace);
          size_t count = m_maxSize;
          if ( n - i < count ) count = n - i;
          creator->setRange(i, count);
          seqDomain->addCreator( IDomainCreator_sptr( creator ) );
        }
        ivalues.reset( new EmptyValues( n ) );
        delete iterator;
        return;
      }
    }
    delete iterator;

    if ( m_count == 0 )
    {
      m_count = n;
    }

    API::FunctionDomainMD* dmd = new API::FunctionDomainMD(m_IMDWorkspace, m_startIndex, m_count);
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
      if (m_workspacePropertyNames.empty())
      {
        throw std::runtime_error("Cannot create FunctionDomainMD: no workspace given");
      }
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

/**
 * Set the range
 * @param startIndex :: Starting index in the worspace
 * @param count :: Size of the domain
 */
void FitMD::setRange(size_t startIndex, size_t count)
{
  m_startIndex = startIndex;
  m_count = count;
}


  /// Return the size of the domain to be created.
  size_t FitMD::getDomainSize() const
  {
    setParameters();
    if ( !m_IMDWorkspace ) throw std::runtime_error("FitMD: workspace wasn't defined");
    auto iterator = m_IMDWorkspace->createIterator();
    size_t n = iterator->getDataSize();
    delete iterator;
    if ( m_count != 0 )
    {
      if ( m_startIndex + m_count > n ) throw std::range_error("FitMD: index is out of range");
      n = m_count;
    }
    return n;
  }


} // namespace Algorithm
} // namespace Mantid
