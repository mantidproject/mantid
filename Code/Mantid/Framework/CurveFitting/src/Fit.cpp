//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Fit.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/FuncMinimizerFactory.h"
#include "MantidCurveFitting/IFuncMinimizer.h"
#include "MantidCurveFitting/CostFuncFitting.h"
#include "MantidCurveFitting/FitMW.h"
#include "MantidCurveFitting/MultiDomainCreator.h"

#include "MantidAPI/DomainCreatorFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/CompositeDomain.h"
#include "MantidAPI/IFunctionValues.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/ITableWorkspace.h"

#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

#include <boost/lexical_cast.hpp>
#include <gsl/gsl_errno.h>
#include <algorithm>

namespace Mantid
{
namespace CurveFitting
{

  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(Fit)

    using API::IDomainCreator;

  namespace
  {
    bool isStringEmpty(const std::string& str){return str.empty();}
  }
  
  /// Sets documentation strings for this algorithm
  void Fit::initDocs()
  {
    this->setWikiSummary("Fits a function to data in a Workspace ");
    this->setOptionalMessage("Fits a function to data in a Workspace");
  }
  
  /**
   * Examine "Function" and "InputWorkspace" properties to decide which domain creator to use.
   * @param propName :: A property name.
   */
  void Fit::afterPropertySet(const std::string& propName)
  {
    if (propName == "Function")
    {
      setFunction();
    }
    else if (propName.size() >= 14 && propName.substr(0,14) == "InputWorkspace")
    {
      if (getPointerToProperty("Function")->isDefault())
      {
        throw std::invalid_argument("Function must be set before InputWorkspace");
      }
      addWorkspace(propName);
    }
    else if (propName == "DomainType")
    {
      setDomainType();
    }
  }

  /**
   * Read domain type property and cache the value
   */
  void Fit::setDomainType()
  {
    std::string domainType = getPropertyValue( "DomainType" );
    if ( domainType == "Simple" )
    {
      m_domainType = IDomainCreator::Simple;
    }
    else if ( domainType == "Sequential" )
    {
      m_domainType = IDomainCreator::Sequential;
    }
    else if ( domainType == "Parallel" )
    {
      m_domainType = IDomainCreator::Parallel;
    }
    else
    {
      m_domainType = IDomainCreator::Simple;
    }
    Kernel::Property* prop = getPointerToProperty("Minimizer");
    auto minimizerProperty = dynamic_cast<Kernel::PropertyWithValue<std::string>*>( prop );
    std::vector<std::string> minimizerOptions = FuncMinimizerFactory::Instance().getKeys();
    if ( m_domainType != IDomainCreator::Simple )
    {
      auto it = std::find(minimizerOptions.begin(), minimizerOptions.end(), "Levenberg-Marquardt");
      minimizerOptions.erase( it );
    }
    minimizerProperty->replaceValidator( Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(minimizerOptions)) );
  }

  void Fit::setFunction()
  {
    // get the function
    m_function = getProperty("Function");
    auto mdf = boost::dynamic_pointer_cast<API::MultiDomainFunction>(m_function);
    if (mdf)
    {
      m_workspacePropertyNames.resize(mdf->nFunctions());
      m_workspacePropertyNames[0] = "InputWorkspace";
      for(size_t i = 1; i < mdf->nFunctions(); ++i)
      {
        std::string workspacePropertyName = "InputWorkspace_"+boost::lexical_cast<std::string>(i);
        m_workspacePropertyNames[i] = workspacePropertyName;
        if (!existsProperty(workspacePropertyName))
        {
          declareProperty(
            new API::WorkspaceProperty<API::Workspace>(workspacePropertyName,"",Kernel::Direction::Input), 
            "Name of the input Workspace");
        }
      }
    }
    else
    {
      m_workspacePropertyNames.resize(1,"InputWorkspace");
    }
  }

  /**
   * Add a new workspace to the fit. The workspace is in the property named workspacePropertyName
   * @param workspacePropertyName :: A workspace property name (eg InputWorkspace or InputWorkspace_2).
   *  The property must already exist in the algorithm.
   */
  void Fit::addWorkspace(const std::string& workspacePropertyName, bool addProperties)
  {
    // get the workspace 
    API::Workspace_const_sptr ws = getProperty(workspacePropertyName);
    //m_function->setWorkspace(ws);
    const size_t n = std::string("InputWorkspace").size();
    const std::string suffix = (workspacePropertyName.size() > n)? workspacePropertyName.substr(n) : "";
    const size_t index = suffix.empty() ? 0 : boost::lexical_cast<size_t>(suffix.substr(1));

    API::IFunction_sptr fun = getProperty("Function");
    IDomainCreator* creator = NULL;
    setDomainType();

    if ( boost::dynamic_pointer_cast<const API::MatrixWorkspace>(ws) &&
        !boost::dynamic_pointer_cast<API::IFunctionMD>(fun) )
    {
      creator = new FitMW(this, workspacePropertyName, m_domainType);
    }
    else
    {
      try
      {
        creator = API::DomainCreatorFactory::Instance().createDomainCreator("FitMD", this, workspacePropertyName, m_domainType);
      }
      catch(Kernel::Exception::NotFoundError&)
      {
        throw std::invalid_argument("Unsupported workspace type" + ws->id());
      }
    }

    if (!m_domainCreator)
    {
      if (m_workspacePropertyNames.empty())
      {
        // this defines the function and fills in m_workspacePropertyNames with names of the sort InputWorkspace_#
        setFunction();
      }
      auto multiFun = boost::dynamic_pointer_cast<API::MultiDomainFunction>(fun);
      if (multiFun)
      {
        auto multiCreator = new MultiDomainCreator(this,m_workspacePropertyNames);
        multiCreator->setCreator(index,creator);
        m_domainCreator.reset(multiCreator);
        creator->declareDatasetProperties(suffix,addProperties);
      }
      else
      {
        m_domainCreator.reset(creator);
        creator->declareDatasetProperties(suffix,addProperties);
      }
    }
    else
    {
      boost::shared_ptr<MultiDomainCreator> multiCreator = boost::dynamic_pointer_cast<MultiDomainCreator>(m_domainCreator);
      if (!multiCreator)
      {
        throw std::runtime_error(std::string("MultiDomainCreator expected, found ") + typeid(*m_domainCreator.get()).name());
      }
      if (!multiCreator->hasCreator(index))
      {
        creator->declareDatasetProperties(suffix,addProperties);
      }
      multiCreator->setCreator(index,creator);
    }

  }

  /**
   * Collect all input workspace property names in the m_workspacePropertyNames vector
   */
  void Fit::addWorkspaces()
  {
    setDomainType();
    auto multiFun = boost::dynamic_pointer_cast<API::MultiDomainFunction>(m_function);
    if (multiFun)
    {
      m_domainCreator.reset(new MultiDomainCreator(this,m_workspacePropertyNames));
    }
    auto props = getProperties();
    for(auto prop = props.begin(); prop != props.end(); ++prop)
    {
      if ((**prop).direction() == Kernel::Direction::Input && dynamic_cast<API::IWorkspaceProperty*>(*prop))
      {
        const std::string workspacePropertyName = (**prop).name();
        API::Workspace_const_sptr ws = getProperty(workspacePropertyName);
        IDomainCreator* creator = NULL;
        if ( boost::dynamic_pointer_cast<const API::MatrixWorkspace>(ws) &&
            !boost::dynamic_pointer_cast<API::IFunctionMD>(m_function) )
        {
          creator = new FitMW(this, workspacePropertyName, m_domainType);
        }
        else
        {// don't know what to do with this workspace
          try
          {
            creator = API::DomainCreatorFactory::Instance().createDomainCreator("FitMD",this, workspacePropertyName, m_domainType);
          }
          catch(Kernel::Exception::NotFoundError&)
          {
            throw std::invalid_argument("Unsupported workspace type" + ws->id());
          }
        }
        const size_t n = std::string("InputWorkspace").size();
        const std::string suffix = (workspacePropertyName.size() > n)? workspacePropertyName.substr(n) : "";
        const size_t index = suffix.empty() ? 0 : boost::lexical_cast<size_t>(suffix.substr(1));
        creator->declareDatasetProperties(suffix,false);
        m_workspacePropertyNames.push_back(workspacePropertyName);
        if (!m_domainCreator)
        {
          m_domainCreator.reset(creator);
        }
        auto multiCreator = boost::dynamic_pointer_cast<MultiDomainCreator>(m_domainCreator);
        if (multiCreator)
        {
          multiCreator->setCreator(index,creator);
        }
      }
    }
  }


  /** Initialisation method
  */
  void Fit::init()
  {
    declareProperty(new API::FunctionProperty("Function"),"Parameters defining the fitting function and its initial values");

    declareProperty(new API::WorkspaceProperty<API::Workspace>("InputWorkspace","",Kernel::Direction::Input), "Name of the input Workspace");

    std::vector<std::string> domainTypes;
    domainTypes.push_back( "Simple" );
    domainTypes.push_back( "Sequential" );
    domainTypes.push_back( "Parallel" );
    declareProperty("DomainType","Simple",
      Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(domainTypes)),
      "The type of function domain to use: Simple, Sequential, or Parallel.", Kernel::Direction::Input);

    declareProperty("Ties","", Kernel::Direction::Input);
    declareProperty("Constraints","", Kernel::Direction::Input);
    auto mustBePositive = boost::shared_ptr< Kernel::BoundedValidator<int> >( new Kernel::BoundedValidator<int>() );
    mustBePositive->setLower(0);
    declareProperty("MaxIterations", 500, mustBePositive->clone(),
      "Stop after this number of iterations if a good fit is not found" );
    declareProperty("OutputStatus","", Kernel::Direction::Output);
    declareProperty("OutputChi2overDoF",0.0, Kernel::Direction::Output);

    // Disable default gsl error handler (which is to call abort!)
    gsl_set_error_handler_off();

    std::vector<std::string> minimizerOptions = FuncMinimizerFactory::Instance().getKeys();

    declareProperty("Minimizer","Levenberg-Marquardt",
      Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(minimizerOptions)),
      "The minimizer method applied to do the fit, default is Levenberg-Marquardt", Kernel::Direction::InOut);

    std::vector<std::string> costFuncOptions = API::CostFunctionFactory::Instance().getKeys();
    // select only CostFuncFitting variety
    for(auto it = costFuncOptions.begin(); it != costFuncOptions.end(); ++it)
    {
      auto costFunc = boost::dynamic_pointer_cast<CostFuncFitting>(
        API::CostFunctionFactory::Instance().create(*it)
        );
      if (!costFunc)
      {
        *it = "";
      }
    }
    std::remove_if(costFuncOptions.begin(),costFuncOptions.end(),isStringEmpty);
    declareProperty("CostFunction","Least squares",
      Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(costFuncOptions)),
      "The cost function to be used for the fit, default is Least squares", Kernel::Direction::InOut);
    declareProperty("CreateOutput", false,
      "Set to true to create output workspaces with the results of the fit"
      "(default is false)." );
    declareProperty("Output", "",
      "A base name for the output workspaces (if not given default names will be created)." );
    declareProperty("CalcErrors", false,
      "Set to true to calcuate errors when output isn't created "
      "(default is false)." );
  }

  /** Executes the algorithm
  *
  *  @throw runtime_error Thrown if algorithm cannot execute
  */
  void Fit::exec()
  {
    // this is to make it work with AlgorithmProxy
    if (!m_domainCreator)
    {
      setFunction();
      addWorkspaces();
    }

    std::string ties = getPropertyValue("Ties");
    if (!ties.empty())
    {
      m_function->addTies(ties);
    }
    std::string contstraints = getPropertyValue("Constraints");
    if (!contstraints.empty())
    {
      m_function->addConstraints(contstraints);
    }

    API::FunctionDomain_sptr domain;
    API::IFunctionValues_sptr values;
    m_domainCreator->createDomain(domain,values);

    // do something with the function which may depend on workspace
    m_domainCreator->initFunction(m_function);

    // get the minimizer
    std::string minimizerName = getPropertyValue("Minimizer");
    IFuncMinimizer_sptr minimizer = FuncMinimizerFactory::Instance().create(minimizerName);

    // Try to retrieve optional properties
    const int maxIterations = getProperty("MaxIterations");

    // get the cost function which must be a CostFuncFitting
    boost::shared_ptr<CostFuncFitting> costFunc = boost::dynamic_pointer_cast<CostFuncFitting>(
      API::CostFunctionFactory::Instance().create(getPropertyValue("CostFunction"))
      );

    costFunc->setFittingFunction(m_function,domain,values);
    minimizer->initialize(costFunc);

    API::Progress prog(this,0.0,1.0,maxIterations?maxIterations:1);

    // do the fitting until success or iteration limit is reached
    size_t iter = 0;
    bool success = false;
    std::string errorString;
    //double costFuncVal = 0;
    do
    {
      iter++;
      if ( !minimizer->iterate() )
      {
        errorString = minimizer->getError();
        success = errorString.empty() || errorString == "success";
        if (success)
        {
          errorString = "success";
        }
        break;
      }
      prog.report();
    }
    while (static_cast<int>(iter) < maxIterations);

    if (static_cast<int>(iter) >= maxIterations)
    {
      if ( !errorString.empty() )
      {
        errorString += '\n';
      }
      errorString += "Failed to converge after " + boost::lexical_cast<std::string>(maxIterations) + " iterations.";
    }

    // return the status flag
    setPropertyValue("OutputStatus",errorString);

    // degrees of freedom
    size_t dof = values->size() - costFunc->nParams();
    if (dof == 0) dof = 1;
    double finalCostFuncVal = minimizer->costFunctionVal() / double(dof);

    setProperty("OutputChi2overDoF",finalCostFuncVal);

    // fit ended, creating output

    // get the workspace 
    API::Workspace_const_sptr ws = getProperty("InputWorkspace");

    bool doCreateOutput = getProperty("CreateOutput");
    std::string baseName = getPropertyValue("Output");
    if ( !baseName.empty() )
    {
      doCreateOutput = true;
    }
    bool doCalcErrors = getProperty("CalcErrors");
    if ( doCreateOutput )
    {
      doCalcErrors = true;
    }
    if ( costFunc->nParams() == 0 )
    {
      doCalcErrors = false;
    }

    GSLMatrix covar;
    if ( doCalcErrors )
    {
      // Calculate the covariance matrix and the errors.
      costFunc->calCovarianceMatrix(covar);
      costFunc->calFittingErrors(covar);
    }

    if (doCreateOutput)
    {
      if (baseName.empty())
      {
        baseName = ws->name();
        if (baseName.empty())
        {
          baseName = "Output";
        }
      }
      baseName += "_";

        declareProperty(
          new API::WorkspaceProperty<API::ITableWorkspace>("OutputNormalisedCovarianceMatrix","",Kernel::Direction::Output),
          "The name of the TableWorkspace in which to store the final covariance matrix" );
        setPropertyValue("OutputNormalisedCovarianceMatrix",baseName+"NormalisedCovarianceMatrix");

        Mantid::API::ITableWorkspace_sptr covariance = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
        covariance->addColumn("str","Name");
        // set plot type to Label = 6
        covariance->getColumn(covariance->columnCount()-1)->setPlotType(6);
        //std::vector<std::string> paramThatAreFitted; // used for populating 1st "name" column
        for(size_t i=0; i < m_function->nParams(); i++)
        {
          if (m_function->isActive(i)) 
          {
            covariance->addColumn("double",m_function->parameterName(i));
            //paramThatAreFitted.push_back(m_function->parameterName(i));
          }
        }

        size_t np = m_function->nParams();
        size_t ia = 0;
        for(size_t i = 0; i < np; i++)
        {
          if (m_function->isFixed(i)) continue;
          Mantid::API::TableRow row = covariance->appendRow();
          row << m_function->parameterName(i);
          size_t ja = 0;
          for(size_t j = 0; j < np; j++)
          {
            if (m_function->isFixed(j)) continue;
            if (j == i)
              row << 100.0;
            else
            {
              row << 100.0*covar.get(ia,ja)/sqrt(covar.get(ia,ia)*covar.get(ja,ja));
            }
            ++ja;
          }
          ++ia;
        }

        setProperty("OutputNormalisedCovarianceMatrix",covariance);

      // create output parameter table workspace to store final fit parameters 
      // including error estimates if derivative of fitting function defined

      declareProperty(
        new API::WorkspaceProperty<API::ITableWorkspace>("OutputParameters","",Kernel::Direction::Output),
        "The name of the TableWorkspace in which to store the final fit parameters" );

      setPropertyValue("OutputParameters",baseName+"Parameters");

      Mantid::API::ITableWorkspace_sptr result = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
      result->addColumn("str","Name");
      // set plot type to Label = 6
      result->getColumn(result->columnCount()-1)->setPlotType(6);
      result->addColumn("double","Value");
      result->addColumn("double","Error");
      // yErr = 5
      result->getColumn(result->columnCount()-1)->setPlotType(5);

      for(size_t i=0;i<m_function->nParams();i++)
      {
        Mantid::API::TableRow row = result->appendRow();
        row << m_function->parameterName(i) 
            << m_function->getParameter(i)
            << m_function->getError(i);
      }
      // Add chi-squared value at the end of parameter table
      Mantid::API::TableRow row = result->appendRow();
      row << "Cost function value" << finalCostFuncVal;      
      setProperty("OutputParameters",result);

      m_domainCreator->createOutputWorkspace(baseName,m_function,domain,values);

    }

  }


} // namespace Algorithm
} // namespace Mantid
