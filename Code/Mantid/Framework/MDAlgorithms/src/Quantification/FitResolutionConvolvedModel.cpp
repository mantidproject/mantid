#include "MantidMDAlgorithms/Quantification/FitResolutionConvolvedModel.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidMDAlgorithms/Quantification/ResolutionConvolvedCrossSection.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModelFactory.h"
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolutionFactory.h"

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FitResolutionConvolvedModel);

using Kernel::Direction;
using Kernel::ListValidator;
using Kernel::MandatoryValidator;
using API::ITableWorkspace;
using API::ITableWorkspace_sptr;
using API::IMDEventWorkspace;
using API::IMDEventWorkspace_sptr;
using API::MatrixWorkspace;
using API::MatrixWorkspace_sptr;
using API::WorkspaceProperty;

namespace {
// Property names
const char *INPUT_WS_NAME = "InputWorkspace";
const char *SIMULATED_NAME = "OutputWorkspace";
const char *OUTPUT_PARS = "OutputParameters";
const char *OUTPUTCOV_MATRIX = "CovarianceMatrix";
const char *RESOLUTION_NAME = "ResolutionFunction";
const char *FOREGROUND_NAME = "ForegroundModel";
const char *PARS_NAME = "Parameters";
const char *MAX_ITER_NAME = "MaxIterations";
}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string FitResolutionConvolvedModel::name() const {
  return "FitResolutionConvolvedModel";
}

/// Algorithm's version for identification. @see Algorithm::version
int FitResolutionConvolvedModel::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string FitResolutionConvolvedModel::category() const {
  return "Quantification";
}

//----------------------------------------------------------------------------------------------

/// Returns the number of iterations that should be performed
int FitResolutionConvolvedModel::niterations() const {
  int maxIter = getProperty(MAX_ITER_NAME);
  return maxIter;
}

/// Returns the name of the max iterations property
std::string FitResolutionConvolvedModel::maxIterationsPropertyName() const {
  return MAX_ITER_NAME;
}

/// Returns the name of the output parameters property
std::string FitResolutionConvolvedModel::outputParsPropertyName() const {
  return OUTPUT_PARS;
}

/// Returns the name of the covariance matrix property
std::string FitResolutionConvolvedModel::covMatrixPropertyName() const {
  return OUTPUTCOV_MATRIX;
}

/**
 * Create the function string required by fit
 * @return Creates a string that can be passed to fit containing the necessary
 * setup for the function
 */
std::string FitResolutionConvolvedModel::createFunctionString() const {
  std::ostringstream stringBuilder;

  const char seperator(',');
  stringBuilder << "name=" << ResolutionConvolvedCrossSection().name()
                << seperator << "ResolutionFunction="
                << this->getPropertyValue(RESOLUTION_NAME) << seperator
                << "ForegroundModel=" << this->getPropertyValue(FOREGROUND_NAME)
                << seperator << this->getPropertyValue("Parameters");
  return stringBuilder.str();
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void FitResolutionConvolvedModel::init() {
  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(INPUT_WS_NAME, "",
                                                           Direction::Input),
                  "The input MDEvent workspace");

  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(SIMULATED_NAME, "",
                                                           Direction::Output),
                  "The simulated output workspace");

  declareProperty(new WorkspaceProperty<ITableWorkspace>(OUTPUT_PARS, "",
                                                         Direction::Output),
                  "The name of the TableWorkspace in which to store the final "
                  "fit parameters");

  declareProperty(new WorkspaceProperty<API::ITableWorkspace>(
                      OUTPUTCOV_MATRIX, "", Direction::Output),
                  "The name of the TableWorkspace in which to store the final "
                  "covariance matrix");

  std::vector<std::string> models =
      MDResolutionConvolutionFactory::Instance().getKeys();
  declareProperty(RESOLUTION_NAME, "",
                  boost::make_shared<ListValidator<std::string>>(models),
                  "The name of a resolution model", Direction::Input);

  models = ForegroundModelFactory::Instance().getKeys();
  declareProperty(FOREGROUND_NAME, "",
                  boost::make_shared<ListValidator<std::string>>(models),
                  "The name of a foreground function", Direction::Input);

  declareProperty(MAX_ITER_NAME, 20,
                  "The maximum number of iterations to perform for the fitting",
                  Direction::Input);

  declareProperty(PARS_NAME, "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The parameters/attributes for the function & model. See Fit "
                  "documentation for format",
                  Direction::Input);
}

//----------------------------------------------------------------------------------------------
/**
 * Execute the algorithm.
 */
void FitResolutionConvolvedModel::exec() {
  API::IAlgorithm_sptr fit = createFittingAlgorithm();
  fit->setPropertyValue("Function", createFunctionString());
  fit->setProperty("InputWorkspace", getPropertyValue(INPUT_WS_NAME));
  fit->setProperty("DomainType",
                   "Simple"); // Parallel not quite giving correct answers
  fit->setProperty("Minimizer", "Levenberg-MarquardtMD");

  const int maxIter = niterations();
  fit->setProperty("MaxIterations", maxIter);
  fit->setProperty("CreateOutput", true);
  fit->setPropertyValue("Output", getPropertyValue(SIMULATED_NAME));

  try {
    fit->execute();
  } catch (std::exception &exc) {
    throw std::runtime_error(
        std::string("FitResolutionConvolvedModel - Error running Fit: ") +
        exc.what());
  }

  // Pass on the relevant properties
  IMDEventWorkspace_sptr simulatedData = fit->getProperty("OutputWorkspace");
  this->setProperty(SIMULATED_NAME, simulatedData);

  if (this->existsProperty(OUTPUT_PARS)) {
    ITableWorkspace_sptr outputPars = fit->getProperty("OutputParameters");
    setProperty(OUTPUT_PARS, outputPars);
  }
  if (this->existsProperty(OUTPUTCOV_MATRIX)) {
    ITableWorkspace_sptr covarianceMatrix =
        fit->getProperty("OutputNormalisedCovarianceMatrix");
    setProperty(OUTPUTCOV_MATRIX, covarianceMatrix);
  }
}

/**
 * Create the fitting Child Algorithm
 * @return A shared pointer to the new algorithm
 */
API::IAlgorithm_sptr FitResolutionConvolvedModel::createFittingAlgorithm() {
  const double startProgress(0.0), endProgress(1.0);
  const bool enableLogging(true);
  return createChildAlgorithm("Fit", startProgress, endProgress, enableLogging);
}

} // namespace MDAlgorithms
} // namespace Mantid
