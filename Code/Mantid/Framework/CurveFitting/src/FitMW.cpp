// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FitMW.h"
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidCurveFitting/EmptyValues.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/WorkspaceProperty.h"

#include "MantidAPI/TextAxis.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EmptyValues.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <algorithm>

namespace Mantid
{
namespace CurveFitting
{

namespace
{
  /**
   * A simple implementation of Jacobian.
   */
  class SimpleJacobian: public API::Jacobian
  {
  public:
    /// Constructor
    SimpleJacobian(size_t nData,size_t nParams):m_nData(nData),m_nParams(nParams),m_data(nData*nParams){}
    /// Setter
    virtual void set(size_t iY, size_t iP, double value)
    {
      m_data[iY * m_nParams + iP] = value;
    }
    /// Getter
    virtual double get(size_t iY, size_t iP)
    {
      return m_data[iY * m_nParams + iP];
    }
  private:
    size_t m_nData; ///< size of the data / first dimension
    size_t m_nParams; ///< number of parameters / second dimension
    std::vector<double> m_data; ///< data storage
  };

  bool greaterIsLess(double x1, double x2)
  {
    return x1 > x2;
  }
}

  using namespace Kernel;
  using API::Workspace;
  using API::Axis;
  using API::MatrixWorkspace;
  using API::Algorithm;
  using API::Jacobian;

  /**
   * Constructor.
   * @param fit :: Property manager with properties defining the domain to be created
   * @param workspacePropertyName :: Name of the workspace property.
   * @param domainType :: Type of the domain: Simple, Sequential, or Parallel.
   */
  FitMW::FitMW(Kernel::IPropertyManager* fit,
    const std::string& workspacePropertyName, 
    FitMW::DomainType domainType):
    API::IDomainCreator(fit,std::vector<std::string>(1,workspacePropertyName),domainType),
  m_startX(EMPTY_DBL()),
  m_endX(EMPTY_DBL())
  {
    if (m_workspacePropertyNames.empty())
    {
      throw std::runtime_error("Cannot create FitMW: no workspace given");
    }
    m_workspacePropertyName = m_workspacePropertyNames[0];
  }

  /**
   * Constructor. Methods setWorkspace, setWorkspaceIndex and setRange must be called
   * set up the creator.
   * @param domainType :: Type of the domain: Simple, Sequential, or Parallel.
   */
  FitMW::FitMW(FitMW::DomainType domainType):
    API::IDomainCreator(NULL,std::vector<std::string>(),domainType),
  m_startX(EMPTY_DBL()),
  m_endX(EMPTY_DBL()),
  m_maxSize(10)
  {
  }


  /**
   * Set all parameters
   */
  void FitMW::setParameters() const
  {
    // if property manager is set overwrite any set parameters
    if ( m_manager )
    {

      // get the workspace 
      API::Workspace_sptr ws = m_manager->getProperty(m_workspacePropertyName);
      m_matrixWorkspace = boost::dynamic_pointer_cast<API::MatrixWorkspace>(ws);
      if (!m_matrixWorkspace)
      {
        throw std::invalid_argument("InputWorkspace must be a MatrixWorkspace.");
      }

      int index = m_manager->getProperty(m_workspaceIndexPropertyName);
      m_workspaceIndex = static_cast<size_t>(index);
      m_startX = m_manager->getProperty(m_startXPropertyName);
      m_endX = m_manager->getProperty(m_endXPropertyName);
      if ( m_domainType != Simple )
      {
        const int maxSizeInt = m_manager->getProperty( m_maxSizePropertyName );
        m_maxSize = static_cast<size_t>( maxSizeInt );
      }
    }
  }

  /**
   * Declare properties that specify the dataset within the workspace to fit to.
   * @param suffix :: names the dataset
   * @param addProp :: allows for the declaration of certain properties of the dataset
   */
  void FitMW::declareDatasetProperties(const std::string& suffix,bool addProp)
  {
    m_workspaceIndexPropertyName = "WorkspaceIndex" + suffix;
    m_startXPropertyName = "StartX" + suffix;
    m_endXPropertyName = "EndX" + suffix;
    m_maxSizePropertyName = "MaxSize" + suffix;

    if (addProp && !m_manager->existsProperty(m_workspaceIndexPropertyName))
    {
      auto mustBePositive = boost::shared_ptr< BoundedValidator<int> >( new BoundedValidator<int>() );
      mustBePositive->setLower(0);
      declareProperty(new PropertyWithValue<int>(m_workspaceIndexPropertyName,0, mustBePositive),
                      "The Workspace Index to fit in the input workspace");
      declareProperty(new PropertyWithValue<double>(m_startXPropertyName, EMPTY_DBL()),
        "A value of x in, or on the low x boundary of, the first bin to include in\n"
        "the fit (default lowest value of x)" );
      declareProperty(new PropertyWithValue<double>(m_endXPropertyName, EMPTY_DBL()),
        "A value in, or on the high x boundary of, the last bin the fitting range\n"
        "(default the highest value of x)" );
      if ( m_domainType != Simple )
      {
        declareProperty(new PropertyWithValue<int>(m_maxSizePropertyName,1, mustBePositive->clone()),
                        "The maximum number of values per a simple domain.");
      }
    }
  }

  /// Create a domain from the input workspace
  void FitMW::createDomain(
    boost::shared_ptr<API::FunctionDomain>& domain, 
    boost::shared_ptr<API::IFunctionValues>& ivalues, size_t i0)
  {
    setParameters();

    const Mantid::MantidVec& X = m_matrixWorkspace->readX(m_workspaceIndex);

    if (X.empty())
    {
      throw std::runtime_error("Workspace contains no data.");
    }

    // find the fitting interval: from -> to
    Mantid::MantidVec::const_iterator from;
    size_t n = 0;
    getStartIterator(X, from, n, m_matrixWorkspace->isHistogramData());
    Mantid::MantidVec::const_iterator to = from + n;

    if ( m_domainType != Simple )
    {
      if ( m_maxSize < n )
      {
        SeqDomain* seqDomain = SeqDomain::create( m_domainType );
        domain.reset( seqDomain );
        size_t m = 0;
        while( m < n )
        {
          // create a simple creator
          FitMW* creator = new FitMW;
          creator->setWorkspace( m_matrixWorkspace );
          creator->setWorkspaceIndex( m_workspaceIndex );
          size_t k = m + m_maxSize;
          if ( k > n ) k = n;
          creator->setRange( *(from + m), *(from + k - 1) );
          seqDomain->addCreator( API::IDomainCreator_sptr( creator ) );
          m = k;
        }
        ivalues.reset( new EmptyValues( n ) );
        return;
      }
      // else continue with simple domain
    }

    // set function domain
    if (m_matrixWorkspace->isHistogramData())
    {
      std::vector<double> x( static_cast<size_t>(to - from) );
      Mantid::MantidVec::const_iterator it = from;
      for(size_t i = 0; it != to; ++it,++i)
      {
        x[i] = (*it + *(it+1)) / 2;
      }
      domain.reset(new API::FunctionDomain1DVector(x));
      x.clear();
    }
    else
    {
      domain.reset(new API::FunctionDomain1DVector(from,to));
    }

    auto values = ivalues ? dynamic_cast<API::FunctionValues*>(ivalues.get()) : new API::FunctionValues(*domain);
    if (!ivalues)
    {
      ivalues.reset(values);
    }
    else
    {
      values->expand(i0 + domain->size());
    }

    // set the data to fit to
    m_startIndex = static_cast<size_t>( from - X.begin() );
    assert( n == domain->size() );
    size_t ito = m_startIndex + n;
    const Mantid::MantidVec& Y = m_matrixWorkspace->readY( m_workspaceIndex );
    const Mantid::MantidVec& E = m_matrixWorkspace->readE( m_workspaceIndex );
    if (ito > Y.size())
    {
      throw std::runtime_error("FitMW: Inconsistent MatrixWorkspace");
    }
    //bool foundZeroOrNegativeError = false;
    for(size_t i = m_startIndex; i < ito; ++i)
    {
      size_t j = i - m_startIndex + i0;
      double y = Y[i];
      double error = E[i];
      double weight = 0.0;

      if ( ! boost::math::isfinite(y) ) // nan or inf data
      {
          if ( !m_ignoreInvalidData ) throw std::runtime_error("Infinte number or NaN found in input data.");
          y = 0.0; // leaving inf or nan would break the fit
      }
      else if ( ! boost::math::isfinite( error ) ) // nan or inf error
      {
          if ( !m_ignoreInvalidData ) throw std::runtime_error("Infinte number or NaN found in input data.");
      }
      else if ( error <= 0 )
      {
          if ( !m_ignoreInvalidData ) weight = 1.0;
      }
      else
      {
          weight = 1.0 / error;
      }

      values->setFitData( j, y );
      values->setFitWeight( j, weight );
    }

  }

  /**
   * Create an output workspace with the calculated values.
   * @param baseName :: Specifies the name of the output workspace
   * @param function :: A Pointer to the fitting function
   * @param domain :: The domain containing x-values for the function
   * @param ivalues :: A API::FunctionValues instance containing the fitting data
   */
  void FitMW::createOutputWorkspace(
        const std::string& baseName,
        API::IFunction_sptr function,
        boost::shared_ptr<API::FunctionDomain> domain,
        boost::shared_ptr<API::IFunctionValues> ivalues
    )
  {
    auto values = boost::dynamic_pointer_cast<API::FunctionValues>(ivalues);
    if (!values)
    {
      return;
    }

    // Compile list of functions to output. The top-level one is first
    std::list<API::IFunction_sptr> functionsToDisplay(1, function);
    if(m_outputCompositeMembers)
    {
      appendCompositeFunctionMembers(functionsToDisplay,function);
    }

    // Nhist = Data histogram, Difference Histogram + nfunctions
    const size_t nhistograms = functionsToDisplay.size() + 2;
    const size_t nyvalues = ivalues->size();
    auto ws = createEmptyResultWS(nhistograms, nyvalues);
    API::TextAxis *textAxis = dynamic_cast<API::TextAxis*>(ws->getAxis(1));
    textAxis->setLabel(0,"Data");
    textAxis->setLabel(1,"Calc");
    textAxis->setLabel(2,"Diff");

    // Add each calculated function
    auto iend = functionsToDisplay.end();
    size_t wsIndex(1); // Zero reserved for data
    for(auto it = functionsToDisplay.begin(); it != iend; ++it)
    {
      if(textAxis && wsIndex > 2) textAxis->setLabel(wsIndex, (*it)->name());
      addFunctionValuesToWS(*it, ws, wsIndex, domain, values);
      if(it == functionsToDisplay.begin()) wsIndex += 2; //Skip difference histogram for now
      else ++wsIndex;
    }

    // Set the difference spectrum
    const MantidVec& Ycal = ws->readY(1);
    MantidVec& Diff = ws->dataY(2);
    const size_t nData = ivalues->size();
    for(size_t i = 0; i < nData; ++i)
    {
      Diff[i] = values->getFitData(i) - Ycal[i];
    }

    declareProperty(new API::WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output Workspace holding resulting simulated spectrum");
    m_manager->setPropertyValue("OutputWorkspace",baseName+"Workspace");
    m_manager->setProperty("OutputWorkspace",ws);
  }

  /**
   * Calculate size and starting iterator in the X array
   * @param X :: The x array.
   * @param from :: iterator to the beginning of the fitting data
   * @param n :: Size of the fitting data
   * @param isHisto :: True if it's histogram data.
   */
  void FitMW::getStartIterator(const Mantid::MantidVec& X, Mantid::MantidVec::const_iterator& from, size_t& n, bool isHisto) const
  {
    if (X.empty())
    {
      throw std::runtime_error("Workspace contains no data.");
    }

    setParameters();

    // find the fitting interval: from -> to
    Mantid::MantidVec::const_iterator to;

    bool isXAscending = X.front() < X.back();

    if (isXAscending)
    {
      if (m_startX == EMPTY_DBL() && m_endX == EMPTY_DBL())
      {
        m_startX = X.front();
        from = X.begin();
        m_endX = X.back();
        to = X.end();
      }
      else if (m_startX == EMPTY_DBL() || m_endX == EMPTY_DBL())
      {
        throw std::invalid_argument("Both StartX and EndX must be given to set fitting interval.");
      }
      else
      {
        if (m_startX > m_endX)
        {
          std::swap(m_startX,m_endX);
        }
        from = std::lower_bound(X.begin(),X.end(),m_startX);
        to = std::upper_bound(from,X.end(),m_endX);
      }
    }
    else // x is descending
    {
      if (m_startX == EMPTY_DBL() && m_endX == EMPTY_DBL())
      {
        m_startX = X.front();
        from = X.begin();
        m_endX = X.back();
        to = X.end();
      }
      else if (m_startX == EMPTY_DBL() || m_endX == EMPTY_DBL())
      {
        throw std::invalid_argument("Both StartX and EndX must be given to set fitting interval.");
      }
      else
      {
        if (m_startX < m_endX)
        {
          std::swap(m_startX,m_endX);
        }
        //from = std::lower_bound(X.begin(),X.end(),startX,([](double x1,double x2)->bool{return x1 > x2;}));
        //to = std::upper_bound(from,X.end(),endX,([](double x1,double x2)->bool{return x1 > x2;}));
        from = std::lower_bound(X.begin(),X.end(),m_startX,greaterIsLess);
        to = std::upper_bound(from,X.end(),m_endX,greaterIsLess);
      }
    }

    n = static_cast<size_t>(to - from);

    if (isHisto)
    {
      if ( X.end() == to )
      {
        to = X.end() - 1;
        --n;
      }
    }
  }

  /**
   * Return the size of the domain to be created.
   */
  size_t FitMW::getDomainSize() const 
  {
    setParameters();
    const Mantid::MantidVec& X = m_matrixWorkspace->readX(m_workspaceIndex);
    size_t n = 0;
    Mantid::MantidVec::const_iterator from;
    getStartIterator(X, from, n, m_matrixWorkspace->isHistogramData());
    return n;
  }

  /**
   * Initialize the function with the workspace.
   * @param function :: Function to initialize.
   */
  void FitMW::initFunction(API::IFunction_sptr function)
  {
    setParameters();
    if (!function)
    {
      throw std::runtime_error("Cannot initialize empty function.");
    }
    function->setWorkspace(m_matrixWorkspace);
    function->setMatrixWorkspace( m_matrixWorkspace, m_workspaceIndex, m_startX, m_endX);
  }

  //--------------------------------------------------------------------------------------------------------------
  /**
   * @param functionList The current list of functions to append to
   * @param function A function that may or not be composite
   */
  void FitMW::appendCompositeFunctionMembers(std::list<API::IFunction_sptr> & functionList,
                                             const API::IFunction_sptr & function) const
  {
    const auto compositeFn = boost::dynamic_pointer_cast<API::CompositeFunction>(function);
    if(!compositeFn) return;

    const size_t nlocals = compositeFn->nFunctions();
    for(size_t i = 0; i < nlocals; ++i)
    {
      auto localFunction = compositeFn->getFunction(i);
      auto localComposite = boost::dynamic_pointer_cast<API::CompositeFunction>(localFunction);
      if(localComposite) appendCompositeFunctionMembers(functionList, localComposite);
      else functionList.insert(functionList.end(), localFunction);
    }
  }

  /**
   * Creates a workspace to hold the results. If the input workspace contains histogram data then so will this
   * It assigns the X and input data values but no Y,E data for any functions
   * @param nhistograms The number of histograms
   * @param nyvalues The number of y values to hold
   */
  API::MatrixWorkspace_sptr FitMW::createEmptyResultWS(const size_t nhistograms, const size_t nyvalues)
  {
    size_t nxvalues(nyvalues);
    if(m_matrixWorkspace->isHistogramData()) nxvalues += 1;
    API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", nhistograms, nxvalues, nyvalues);
    ws->setTitle("");
    ws->setYUnitLabel(m_matrixWorkspace->YUnitLabel());
    ws->setYUnit(m_matrixWorkspace->YUnit());
    ws->getAxis(0)->unit() = m_matrixWorkspace->getAxis(0)->unit();
    API::TextAxis* tAxis = new API::TextAxis(nhistograms);
    ws->replaceAxis(1,tAxis);

    const MantidVec& inputX = m_matrixWorkspace->readX(m_workspaceIndex);
    const MantidVec& inputY = m_matrixWorkspace->readY(m_workspaceIndex);
    const MantidVec& inputE = m_matrixWorkspace->readE(m_workspaceIndex);
    // X values for all
    for(size_t i = 0;i < nhistograms; i++)
    {
      ws->dataX(i).assign(inputX.begin() + m_startIndex, inputX.begin() + m_startIndex + nxvalues);
    }
    // Data values for the first histogram
    ws->dataY(0).assign( inputY.begin() + m_startIndex, inputY.begin() + m_startIndex + nyvalues);
    ws->dataE(0).assign( inputE.begin() + m_startIndex, inputE.begin() + m_startIndex + nyvalues);

    return ws;
  }

  /**
   * Add the calculated function values to the workspace
   * @param function The function to evaluate
   * @param ws A workspace to fill
   * @param wsIndex The index to store the values
   * @param domain The domain to calculate the values over
   * @param resultValues A presized values holder for the results
   */
  void FitMW::addFunctionValuesToWS(const API::IFunction_sptr & function, boost::shared_ptr<API::MatrixWorkspace> & ws,
      const size_t wsIndex, const boost::shared_ptr<API::FunctionDomain> & domain, boost::shared_ptr<API::FunctionValues> resultValues) const
  {
    const size_t nData = resultValues->size();
    resultValues->zeroCalculated();

    // Function value
    function->function(*domain,*resultValues);
    // and errors
    SimpleJacobian J(nData, function->nParams());
    try
    {
      function->functionDeriv(*domain,J);
    }
    catch(...)
    {
      function->calNumericalDeriv(*domain,J);
    }


    MantidVec& yValues = ws->dataY(wsIndex);
    MantidVec& eValues = ws->dataE(wsIndex);
    for(size_t i=0; i < nData; i++)
    {
      yValues[i] = resultValues->getCalculated(i);
      double err = 0.0;
      for(size_t j=0; j< function->nParams();++j)
      {
        double d = J.get(i,j) * function->getError(j);
        err += d*d;
      }
      eValues[i] = std::sqrt(err);
    }

  }

} // namespace Algorithm
} // namespace Mantid
