//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"

#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/StartsWithValidator.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Fit)

/** Initialisation method
*/
void Fit::initConcrete() {

  declareProperty("Ties", "", Kernel::Direction::Input);
  getPointerToProperty("Ties")
      ->setDocumentation("Math expressions defining ties between parameters of "
                         "the fitting function.");
  declareProperty("Constraints", "", Kernel::Direction::Input);
  getPointerToProperty("Constraints")->setDocumentation("List of constraints");
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(
      "MaxIterations", 500, mustBePositive->clone(),
      "Stop after this number of iterations if a good fit is not found");
  declareProperty("OutputStatus", "", Kernel::Direction::Output);
  getPointerToProperty("OutputStatus")
      ->setDocumentation("Whether the fit was successful");
  declareProperty("OutputChi2overDoF", 0.0, "Returns the goodness of the fit",
                  Kernel::Direction::Output);

  // Disable default gsl error handler (which is to call abort!)
  gsl_set_error_handler_off();

  std::vector<std::string> minimizerOptions =
      API::FuncMinimizerFactory::Instance().getKeys();
  Kernel::IValidator_sptr minimizerValidator =
      boost::make_shared<Kernel::StartsWithValidator>(minimizerOptions);

  declareProperty("Minimizer", "Levenberg-Marquardt", minimizerValidator,
                  "Minimizer to use for fitting.");

  std::vector<std::string> costFuncOptions =
      API::CostFunctionFactory::Instance().getKeys();
  // select only CostFuncFitting variety
  for (auto &costFuncOption : costFuncOptions) {
    auto costFunc = boost::dynamic_pointer_cast<CostFunctions::CostFuncFitting>(
        API::CostFunctionFactory::Instance().create(costFuncOption));
    if (!costFunc) {
      costFuncOption = "";
    }
  }
  Kernel::IValidator_sptr costFuncValidator =
      boost::make_shared<Kernel::ListValidator<std::string>>(costFuncOptions);
  declareProperty(
      "CostFunction", "Least squares", costFuncValidator,
      "The cost function to be used for the fit, default is Least squares",
      Kernel::Direction::InOut);
  declareProperty(
      "CreateOutput", false,
      "Set to true to create output workspaces with the results of the fit"
      "(default is false).");
  declareProperty(
      "Output", "",
      "A base name for the output workspaces (if not "
      "given default names will be created). The "
      "default is to use the name of the original data workspace as prefix "
      "followed by suffixes _Workspace, _Parameters, etc.");
  declareProperty("CalcErrors", false,
                  "Set to true to calcuate errors when output isn't created "
                  "(default is false).");
  declareProperty("OutputCompositeMembers", false,
                  "If true and CreateOutput is true then the value of each "
                  "member of a Composite Function is also output.");
  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<bool>>(
                      "ConvolveMembers", false),
                  "If true and OutputCompositeMembers is true members of any "
                  "Convolution are output convolved\n"
                  "with corresponding resolution");
  declareProperty("OutputParametersOnly", false,
                  "Set to true to output only the parameters and not "
                  "workspace(s) with the calculated values\n"
                  "(default is false, ignored if CreateOutput is false and "
                  "Output is an empty string).");
}

/**
  * Copy all output workspace properties from the minimizer to Fit algorithm.
  * @param minimizer :: The minimizer to copy from.
  */
void Fit::copyMinimizerOutput(const API::IFuncMinimizer &minimizer) {
  auto &properties = minimizer.getProperties();
  for (auto property : properties) {
    if ((*property).direction() == Kernel::Direction::Output &&
        (*property).isValid() == "") {
      auto clonedProperty =
          std::unique_ptr<Kernel::Property>((*property).clone());
      declareProperty(std::move(clonedProperty));
    }
  }
}

/** Executes the algorithm
*
*  @throw runtime_error Thrown if algorithm cannot execute
*/
void Fit::execConcrete() {

  std::string ties = getPropertyValue("Ties");
  if (!ties.empty()) {
    m_function->addTies(ties);
  }
  std::string contstraints = getPropertyValue("Constraints");
  if (!contstraints.empty()) {
    m_function->addConstraints(contstraints);
  }

  auto costFunc = getCostFunctionInitialized();

  // Try to retrieve optional properties
  int intMaxIterations = getProperty("MaxIterations");
  const size_t maxIterations = static_cast<size_t>(intMaxIterations);

  // get the minimizer
  std::string minimizerName = getPropertyValue("Minimizer");
  API::IFuncMinimizer_sptr minimizer =
      API::FuncMinimizerFactory::Instance().createMinimizer(minimizerName);
  minimizer->initialize(costFunc, maxIterations);

  const int64_t nsteps = maxIterations * m_function->estimateNoProgressCalls();
  API::Progress prog(this, 0.0, 1.0, nsteps);
  m_function->setProgressReporter(&prog);

  // do the fitting until success or iteration limit is reached
  size_t iter = 0;
  bool success = false;
  std::string errorString;
  g_log.debug("Starting minimizer iteration\n");
  while (iter < maxIterations) {
    g_log.debug() << "Starting iteration " << iter << "\n";
    m_function->iterationStarting();
    if (!minimizer->iterate(iter)) {
      errorString = minimizer->getError();
      g_log.debug() << "Iteration stopped. Minimizer status string="
                    << errorString << "\n";

      success = errorString.empty() || errorString == "success";
      if (success) {
        errorString = "success";
      }
      break;
    }
    prog.report();
    m_function->iterationFinished();
    ++iter;
  }
  g_log.debug() << "Number of minimizer iterations=" << iter << "\n";

  minimizer->finalize();

  if (iter >= maxIterations) {
    if (!errorString.empty()) {
      errorString += '\n';
    }
    errorString += "Failed to converge after " + std::to_string(maxIterations) +
                   " iterations.";
  }

  // return the status flag
  setPropertyValue("OutputStatus", errorString);

  // degrees of freedom
  size_t dof = costFunc->getDomain()->size() - costFunc->nParams();
  if (dof == 0)
    dof = 1;
  double rawcostfuncval = minimizer->costFunctionVal();
  double finalCostFuncVal = rawcostfuncval / double(dof);

  setProperty("OutputChi2overDoF", finalCostFuncVal);

  // fit ended, creating output

  // get the workspace
  API::Workspace_const_sptr ws = getProperty("InputWorkspace");

  bool doCreateOutput = getProperty("CreateOutput");
  std::string baseName = getPropertyValue("Output");
  if (!baseName.empty()) {
    doCreateOutput = true;
  }
  bool doCalcErrors = getProperty("CalcErrors");
  if (doCreateOutput) {
    doCalcErrors = true;
  }
  if (costFunc->nParams() == 0) {
    doCalcErrors = false;
  }

  GSLMatrix covar;
  if (doCalcErrors) {
    // Calculate the covariance matrix and the errors.
    costFunc->calCovarianceMatrix(covar);
    costFunc->calFittingErrors(covar, rawcostfuncval);
  }

  if (doCreateOutput) {
    copyMinimizerOutput(*minimizer);

    if (baseName.empty()) {
      baseName = ws->name();
      if (baseName.empty()) {
        baseName = "Output";
      }
    }
    baseName += "_";

    declareProperty(
        Kernel::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
            "OutputNormalisedCovarianceMatrix", "", Kernel::Direction::Output),
        "The name of the TableWorkspace in which to store the final covariance "
        "matrix");
    setPropertyValue("OutputNormalisedCovarianceMatrix",
                     baseName + "NormalisedCovarianceMatrix");

    Mantid::API::ITableWorkspace_sptr covariance =
        Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
    covariance->addColumn("str", "Name");
    // set plot type to Label = 6
    covariance->getColumn(covariance->columnCount() - 1)->setPlotType(6);
    // std::vector<std::string> paramThatAreFitted; // used for populating 1st
    // "name" column
    for (size_t i = 0; i < m_function->nParams(); i++) {
      if (m_function->isActive(i)) {
        covariance->addColumn("double", m_function->parameterName(i));
        // paramThatAreFitted.push_back(m_function->parameterName(i));
      }
    }

    size_t np = m_function->nParams();
    size_t ia = 0;
    for (size_t i = 0; i < np; i++) {
      if (m_function->isFixed(i))
        continue;
      Mantid::API::TableRow row = covariance->appendRow();
      row << m_function->parameterName(i);
      size_t ja = 0;
      for (size_t j = 0; j < np; j++) {
        if (m_function->isFixed(j))
          continue;
        if (j == i)
          row << 100.0;
        else {
          if (!covar.gsl()) {
            throw std::runtime_error(
                "There was an error while allocating the (GSL) covariance "
                "matrix "
                "which is needed to produce fitting error results.");
          }
          row << 100.0 * covar.get(ia, ja) /
                     sqrt(covar.get(ia, ia) * covar.get(ja, ja));
        }
        ++ja;
      }
      ++ia;
    }

    setProperty("OutputNormalisedCovarianceMatrix", covariance);

    // create output parameter table workspace to store final fit parameters
    // including error estimates if derivative of fitting function defined

    declareProperty(
        Kernel::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
            "OutputParameters", "", Kernel::Direction::Output),
        "The name of the TableWorkspace in which to store the "
        "final fit parameters");

    setPropertyValue("OutputParameters", baseName + "Parameters");

    Mantid::API::ITableWorkspace_sptr result =
        Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
    result->addColumn("str", "Name");
    // set plot type to Label = 6
    result->getColumn(result->columnCount() - 1)->setPlotType(6);
    result->addColumn("double", "Value");
    result->addColumn("double", "Error");
    // yErr = 5
    result->getColumn(result->columnCount() - 1)->setPlotType(5);

    for (size_t i = 0; i < m_function->nParams(); i++) {
      Mantid::API::TableRow row = result->appendRow();
      row << m_function->parameterName(i) << m_function->getParameter(i)
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
    setProperty("OutputParameters", result);
#else
    row << "Cost function value" << finalCostFuncVal;
    Mantid::API::TableRow row2 = result->appendRow();
    std::string name(getPropertyValue("CostFunction"));
    name += " value";
    row2 << name << rawcostfuncval;
#endif

    setProperty("OutputParameters", result);

    bool outputParametersOnly = getProperty("OutputParametersOnly");

    if (!outputParametersOnly) {
      const bool unrollComposites = getProperty("OutputCompositeMembers");
      bool convolveMembers = existsProperty("ConvolveMembers");
      if (convolveMembers) {
        convolveMembers = getProperty("ConvolveMembers");
      }
      m_domainCreator->separateCompositeMembersInOutput(unrollComposites,
                                                        convolveMembers);
      m_domainCreator->createOutputWorkspace(
          baseName, m_function, costFunc->getDomain(), costFunc->getValues());
    }
  }

  progress(1.0);
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
