/*WIKI* 
This is a generic algorithm for fitting data in a Workspace with a function.
The workspace must have the type supported by the algorithm. Currently supported
types are: [[MatrixWorkspace]] for fitting with a [[IFunction1D]] and
[[IMDWorkspace]] for fitting with [[IFunctionMD]]. After Function and InputWorkspace
properties are set the algorithm may decide that it needs more information from
the caller to locate the fitting data. For example, if a spectrum in a MatrixWorkspace
is to be fit with a 1D function it will need to know at least the index of that spectrum.
To request this information Fit dynamically creates relevant properties which the caller
can set. Note that the dynamic properties depend both on the workspace and the function.
For example, the data in a MatrixWorkspace can be fit with a 2D function. In this case all
spectra will be used in the fit and no additional properties will be declared. The Function
property must be set before any other.

The function and the initial values for its parameters are set with the Function property.
A function can be simple or composite. A [[:Category:Fit_functions|simple function]] has a
name registered with Mantid framework. The Fit algorithm creates an instance of a function
by this name. A composite function is an arithmetic sum of two or more functions (simple or
composite). Each function has a number of named parameters, the names are case sensitive.
All function parameters will be used in the fit unless some of them are tied. Parameters
can be tied by setting the Ties property. A tie is a mathematical expression which is used
to calculate the value of a (dependent) parameter. Only the parameter names of the same
function can be used as variables in this expression.

Using the Minimizer property, Fit can be set to use different algorithms to perform the
minimization. By default if the function's derivatives can be evaluated then Fit uses the
GSL Levenberg-Marquardt minimizer.

In Mantidplot this algorithm can be run from the [[MantidPlot:_Data Analysis and Curve
Fitting#Simple Peak Fitting with the Fit Wizard|Fit Property Browser]] which allows all
the settings to be specified via its graphical user interface.

===Setting a simple function===

To use a simple function for a fit set its name and initial parameter values using the
Function property. This property is a comma separated list of name=value pairs. The name
of the first name=value pairs must be "name" and it must be set equal to the name of one
of a [[:Category:Fit_functions|simple function]]. This name=value pair is followed by
name=value pairs specifying values for the parameters of this function. If a parameter
is not set in Function it will be given its default value defined by the function. All
names are case sensitive. For example for fitting a Gaussian the Function property might
look like this:

 Function: "name=Gaussian, PeakCentre=4.6, Height=10, Sigma=0.5"

Some functions have attributes. An attribute is a non-fitting parameter and can be of
one of the following types: text string, integer, or double. Attributes are set just
like the parameters using name=value pairs. For example:

 Function: "name=UserFunction, Formula=a+b*x, a=1, b=2"

In this example Formula is the name of a string attribute which defines an expression
for the user UserFunction. The fitting parameters a and b are created when the Formula
attribute is set. It is important that Formula is defined before initializing the parameters.

A list of the available simple functions can be found [[:Category:Fit_functions|here]].

===Setting a composite function===

A composite function is a sum of simple functions. It does not have a name. To define a
composite function set a number of simple functions in the Function property. Each simple
function definition must be separated by a semicolon ';'. For example fitting two Gaussians
on a linear background might look like this:

 Function: "name=LinearBackground, A0=0.3; 
            name=Gaussian, PeakCentre=4.6, Height=10, Sigma=0.5;
            name=Gaussian, PeakCentre=7.6, Height=8, Sigma=0.5"

===Setting ties===

Parameters can be tied to other parameters or to a constant. In this case they do not take
part in the fitting but are evaluated using the tying expressions. Use Ties property to set
any ties. In case of a simple function the parameter names are used as variables in the
tying expressions. For example

 Ties: "a=2*b+1, c=2"

This ties parameter "a" to parameter "b" and fixes "c" to the constant 2.

In case of a composite function the variable name must refer to both the parameter name and
the simple function it belongs to. It is done by writing the variable name in the following format:
 f<index>.<name>
The format consists of two parts separated by a period '.'. The first part defines the
function by its index in the composite function (starting at 0). The index corresponds
to the order in which the functions are defined in the Function property. For example:

 Ties: "f1.Sigma=f0.Sigma,f2.Sigma=f0.Sigma"

This ties parameter "Sigma" of functions 1 and 2 to the "Sigma" of function 0. Of course
all three functions must have a parameter called "Sigma" for this to work. The last example
can also be written

 Ties: "f1.Sigma=f2.Sigma=f0.Sigma"

===Setting constraints===

Parameters can be constrained to be above a lower boundary and/or below an upper boundary.
If a constraint is violated a penalty to the fit is applied which should result the parameters
satisfying the constraint. The penalty applied is described in more detail [[FitConstraint|here]].
Use Constraints property to set any constraints. In case of a simple function the parameter names
are used as variables in the constraint expressions. For example

 Constraints: "4.0 < c < 4.2"

Constraint the parameter "c" to be with the range 4.0 to 4.2, whereas 

 Constraints: "c > 4.0"

means "c" is constrained to be above the lower value 4.0 and 

 Constraints: "c < 4.2"

means "c" is constrained to be below the upper value 4.2.

In case of a composite function the same notation is used for constraints and for ties. For example

 Constraints: "f1.c < 4.2"

constrain the parameter "c" of function 1.

===Fitting to data in a MatrixWorkspace===

The error values in the input workspace are used to weight the data in the fit. Zero error values
are not allowed and are replaced with ones.

===Output===

Setting the Output property defines the names of the two output workspaces. One of them is a
[[TableWorkspace]] with the fitted parameter values. The other is a [[Workspace2D]] which
compares the fit with the original data. It has three spectra. The first (index 0) contains
the original data, the second one the data simulated with the fitting function and the third
spectrum is the difference between the first two. For example, if the Output was set to "MyResults"
the parameter TableWorkspace will have name "MyResults_Parameters" and the Workspace2D will be named
"MyResults_Workspace". If the function's derivatives can be evaluated an additional TableWorkspace is
returned. When the Output is set to "MyResults" this TableWorkspace will have the name
"MyResults_NormalisedCovarianceMatrix" and it returns a calculated correlation matrix.
Denote this matrix C and its elements Cij then the diagonal elements are listed as 1.0
and the off diagnonal elements as percentages of correlation between parameter i and j equal to 100*Cij/sqrt(Cii*Cjj).

==Examples==

This example shows a simple fit to a Gaussian function. The algorithm properties are:

 InputWorkspace:  Test
 WorkspaceIndex:  0
 Function:        name=Gaussian, PeakCentre=4, Height=1.3, Sigma=0.5
 Output:          res

[[Image:GaussianFit.jpg]]

----

The next example shows a fit of the same data but with a tie.

 InputWorkspace:  Test
 WorkspaceIndex:  0
 Function:        name=Gaussian, PeakCentre=4, Height=1.3, Sigma=0.5
 Ties:            Sigma=Height/2
 Output:          res

[[Image:GaussianFit_Ties.jpg]]

----

This example shows a fit of two overlapping Gaussians on a linear background. Here we create
a composite function with a LinearBackground and two Gaussians:

 InputWorkspace:  Test
 WorkspaceIndex:  0
 Function:        name=LinearBackground,A0=1;
                  name=Gaussian,PeakCentre=4,Height=1.5, Sigma=0.5;
                  name=Gaussian,PeakCentre=6,Height=4, Sigma=0.5 
 Output:          res

[[Image:Gaussian2Fit.jpg]]

----

This example repeats the previous one but with the Sigmas of the two Gaussians tied:

 InputWorkspace:  Test
 WorkspaceIndex:  0
 Function:        name=LinearBackground,A0=1;
                  name=Gaussian,PeakCentre=4,Height=1.5, Sigma=0.5;
                  name=Gaussian,PeakCentre=6,Height=4, Sigma=0.5 
 Ties:            f2.Sigma = f1.Sigma
 Output:          res

[[Image:Gaussian2Fit_Ties.jpg]]

=== Additional properties for a 1D function and a MatrixWorkspace ===
If Function defines a one-dimensional function and InputWorkspace is a [[MatrixWorkspace]]
the algorithm will have these additional properties:

{| border="1" cellpadding="5" cellspacing="0"
!Name
!Direction
!Type
!Default
!Description
|-
|WorkspaceIndex
|Input
|integer
|0
|The spectrum to fit, using the workspace numbering of the spectra 
|-
|StartX
|Input
|double
|Start of the spectrum
|An X value in the first bin to be included in the fit
|-
|EndX
|Input
|double
|End of the spectrum
|An X value in the last bin to be included in the fit
|}

*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Fit.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/CostFuncFitting.h"
#include "MantidCurveFitting/FitMW.h"
#include "MantidCurveFitting/MultiDomainCreator.h"
#include "MantidCurveFitting/Convolution.h"

#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFuncMinimizer.h"
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
#include "MantidKernel/StartsWithValidator.h"

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
    std::vector<std::string> minimizerOptions = API::FuncMinimizerFactory::Instance().getKeys();
    if ( m_domainType != IDomainCreator::Simple )
    {
      auto it = std::find(minimizerOptions.begin(), minimizerOptions.end(), "Levenberg-Marquardt");
      minimizerOptions.erase( it );
    }
    minimizerProperty->replaceValidator( Kernel::IValidator_sptr(new Kernel::StartsWithValidator(minimizerOptions)) );
  }

  void Fit::setFunction()
  {
    // get the function
    m_function = getProperty("Function");
    auto mdf = boost::dynamic_pointer_cast<API::MultiDomainFunction>(m_function);
    if (mdf)
    {
        size_t ndom = mdf->getMaxIndex() + 1;
      m_workspacePropertyNames.resize( ndom );
      m_workspacePropertyNames[0] = "InputWorkspace";
      for(size_t i = 1; i < ndom; ++i)
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
   * @param addProperties :: allow for declaration of properties that specify the dataset
   *  within the workspace to fit to.
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
    getPointerToProperty("Ties")->setDocumentation("Math expressions defining ties between parameters of the fitting function.");
    declareProperty("Constraints","", Kernel::Direction::Input);
    getPointerToProperty("Constraints")->setDocumentation("List of constraints");
    auto mustBePositive = boost::shared_ptr< Kernel::BoundedValidator<int> >( new Kernel::BoundedValidator<int>() );
    mustBePositive->setLower(0);
    declareProperty("MaxIterations", 500, mustBePositive->clone(),
      "Stop after this number of iterations if a good fit is not found" );
    declareProperty("IgnoreInvalidData",false,"Flag to ignore infinities, NaNs and data with zero errors.");
    declareProperty("OutputStatus","", Kernel::Direction::Output);
    getPointerToProperty("OutputStatus")->setDocumentation( "Whether the fit was successful" );
    declareProperty("OutputChi2overDoF",0.0, "Returns the goodness of the fit", Kernel::Direction::Output);

    // Disable default gsl error handler (which is to call abort!)
    gsl_set_error_handler_off();

    std::vector<std::string> minimizerOptions = API::FuncMinimizerFactory::Instance().getKeys();

    declareProperty("Minimizer","Levenberg-Marquardt",
      Kernel::IValidator_sptr(new Kernel::StartsWithValidator(minimizerOptions)),
      "Minimizer to use for fitting. Minimizers available are \"Levenberg-Marquardt\", \"Simplex\", \"Conjugate gradient (Fletcher-Reeves imp.)\", \"Conjugate gradient (Polak-Ribiere imp.)\", \"BFGS\", and \"Levenberg-MarquardtMD\"");

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
    declareProperty("OutputCompositeMembers",false,
        "If true and CreateOutput is true then the value of each member of a Composite Function is also output.");
    declareProperty(new Kernel::PropertyWithValue<bool>("ConvolveMembers", false),
      "If true and OutputCompositeMembers is true members of any Convolution are output convolved\n"
      "with corresponding resolution");
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
    
    // prepare the function for a fit
    m_function->setUpForFit();

    API::FunctionDomain_sptr domain;
    API::IFunctionValues_sptr values;
    m_domainCreator->ignoreInvalidData(getProperty("IgnoreInvalidData"));
    m_domainCreator->createDomain(domain,values);

    // do something with the function which may depend on workspace
    m_domainCreator->initFunction(m_function);

    // get the minimizer
    std::string minimizerName = getPropertyValue("Minimizer");
    API::IFuncMinimizer_sptr minimizer = API::FuncMinimizerFactory::Instance().createMinimizer(minimizerName);

    // Try to retrieve optional properties
    const int maxIterations = getProperty("MaxIterations");

    // get the cost function which must be a CostFuncFitting
    boost::shared_ptr<CostFuncFitting> costFunc = boost::dynamic_pointer_cast<CostFuncFitting>(
      API::CostFunctionFactory::Instance().create(getPropertyValue("CostFunction"))
      );

    costFunc->setFittingFunction(m_function,domain,values);
    minimizer->initialize(costFunc);

    const int64_t nsteps = maxIterations*m_function->estimateNoProgressCalls();
    API::Progress prog(this,0.0,1.0,nsteps);
    m_function->setProgressReporter(&prog);

    // do the fitting until success or iteration limit is reached
    size_t iter = 0;
    bool success = false;
    std::string errorString;
    g_log.debug("Starting minimizer iteration\n");
    while (static_cast<int>(iter) < maxIterations)
    {
      iter++;
      g_log.debug() << "Starting iteration " << iter << "\n";
      m_function->iterationStarting();
      if ( !minimizer->iterate() )
      {
        errorString = minimizer->getError();
        g_log.debug() << "Iteration stopped. Minimizer status string=" << errorString << "\n";

        success = errorString.empty() || errorString == "success";
        if (success)
        {
          errorString = "success";
        }
        break;
      }
      prog.report();
      m_function->iterationFinished();
      if(g_log.is(Kernel::Logger::Priority::PRIO_INFORMATION))
      {
        g_log.debug() << "Iteration " << iter << ", cost function = " << minimizer->costFunctionVal() << "\n";
      }
    }
    g_log.debug() << "Number of minimizer iterations=" << iter << "\n";

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
    double rawcostfuncval = minimizer->costFunctionVal();
    double finalCostFuncVal = rawcostfuncval / double(dof);

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
#if 1
      std::string costfuncname = getPropertyValue("CostFunction");
      if (costfuncname == "Rwp")
        row << "Cost function value" << rawcostfuncval;
      else
        row << "Cost function value" << finalCostFuncVal;
      setProperty("OutputParameters",result);
#else
      row << "Cost function value" << finalCostFuncVal;
      Mantid::API::TableRow row2 = result->appendRow();
      std::string name(getPropertyValue("CostFunction"));
      name += " value";
      row2 << name << rawcostfuncval;
#endif

      setProperty("OutputParameters",result);

      const bool unrollComposites = getProperty("OutputCompositeMembers");
      bool convolveMembers = existsProperty("ConvolveMembers");
      if ( convolveMembers )
      {
          convolveMembers = getProperty("ConvolveMembers");
      }
      m_domainCreator->separateCompositeMembersInOutput(unrollComposites,convolveMembers);
      m_domainCreator->createOutputWorkspace(baseName,m_function,domain,values);

    }

  }


} // namespace Algorithm
} // namespace Mantid
