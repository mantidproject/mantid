#include "MantidAlgorithms/NormaliseByDetector.h"
#include "MantidKernel/System.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/Progress.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/muParser_Silent.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(NormaliseByDetector)

//----------------------------------------------------------------------------------------------
/** Constructor
*/
NormaliseByDetector::NormaliseByDetector(bool parallelExecution)
    : m_parallelExecution(parallelExecution) {}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
NormaliseByDetector::~NormaliseByDetector() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string NormaliseByDetector::name() const {
  return "NormaliseByDetector";
};

/// Algorithm's version for identification. @see Algorithm::version
int NormaliseByDetector::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string NormaliseByDetector::category() const {
  return "CorrectionFunctions\\NormalisationCorrections";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
*/
void NormaliseByDetector::init() {
  auto compositeValidator = boost::make_shared<CompositeValidator>();
  compositeValidator->add(
      boost::make_shared<API::WorkspaceUnitValidator>("Wavelength"));
  compositeValidator->add(boost::make_shared<API::HistogramValidator>());

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                                         Direction::Input,
                                                         compositeValidator),
                  "An input workspace in wavelength");

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "An output workspace.");
}

const Geometry::FitParameter NormaliseByDetector::tryParseFunctionParameter(
    Geometry::Parameter_sptr parameter, Geometry::IDetector_const_sptr det) {
  if (parameter == NULL) {
    std::stringstream stream;
    stream << det->getName() << " and all of it's parent components, have no "
                                "fitting type parameters. This algorithm "
                                "cannot be run without fitting parameters. See "
                                "wiki help for details on setup.";
    this->g_log.warning(stream.str());
    throw std::runtime_error(stream.str());
  }
  return parameter->value<Geometry::FitParameter>();
}

/**
Process each histogram of the input workspace, extracting the detector/component
and looking up the efficiency function.
Efficiency functions are then executed against the X data of the input workspace
to generate new Y and E outputs for the denominatorWS.
@param wsIndex: The index of the histogram in the input workspace to process.
@param denominatorWS : Workspace that will become the denominator in the
normalisation routine.
@param inWS: Workspace input. Contains instrument to use as well as X data to
use.
@param prog: progress reporting object.
*/
void NormaliseByDetector::processHistogram(size_t wsIndex,
                                           MatrixWorkspace_sptr denominatorWS,
                                           MatrixWorkspace_const_sptr inWS,
                                           Progress &prog) {
  const Geometry::ParameterMap &paramMap = inWS->instrumentParameters();
  Geometry::IDetector_const_sptr det = inWS->getDetector(wsIndex);
  const std::string type = "fitting";
  Geometry::Parameter_sptr foundParam =
      paramMap.getRecursiveByType(&(*det), type);

  const Geometry::FitParameter &foundFittingParam =
      tryParseFunctionParameter(foundParam, det);

  std::string fitFunctionName = foundFittingParam.getFunction();
  IFunction_sptr function =
      FunctionFactory::Instance().createFunction(fitFunctionName);
  typedef std::vector<std::string> ParamNames;
  ParamNames allParamNames = function->getParameterNames();

  // Lookup each parameter name.
  for (ParamNames::iterator it = allParamNames.begin();
       it != allParamNames.end(); ++it) {
    Geometry::Parameter_sptr param =
        paramMap.getRecursive(&(*det), (*it), type);

    const Geometry::FitParameter &fitParam =
        tryParseFunctionParameter(param, det);

    if (fitParam.getFormula().compare("") == 0) {
      throw std::runtime_error(
          "A Forumla has not been provided for a fit function");
    } else {
      std::string resultUnitStr = fitParam.getResultUnit();
      if (!resultUnitStr.empty() && resultUnitStr.compare("Wavelength") != 0) {
        throw std::runtime_error(
            "Units for function parameters must be in Wavelength");
      }
    }
    mu::Parser p;
    p.SetExpr(fitParam.getFormula());
    double paramValue = p.Eval();
    // Set the function coeffiecents.
    function->setParameter(fitParam.getName(), paramValue);
  }

  auto wavelengths = inWS->readX(wsIndex);
  const size_t nInputBins = wavelengths.size() - 1;
  std::vector<double> centerPointWavelength(nInputBins);
  std::vector<double> outIntensity(nInputBins);
  for (size_t binIndex = 0; binIndex < nInputBins; ++binIndex) {
    centerPointWavelength[binIndex] =
        0.5 * (wavelengths[binIndex] + wavelengths[binIndex + 1]);
  }
  FunctionDomain1DVector domain(centerPointWavelength);
  FunctionValues values(domain);
  function->function(domain, values);
  for (size_t i = 0; i < domain.size(); ++i) {
    outIntensity[i] = values[i];
  }
  denominatorWS->dataY(wsIndex) = outIntensity;
  denominatorWS->dataE(wsIndex) = MantidVec(nInputBins, 0);
  prog.report();
}

/**
Controlling function. Processes the histograms either in parallel or
sequentially.
@param inWS: Workspace input. Contains instrument to use as well as X data to
use.
*/
MatrixWorkspace_sptr
NormaliseByDetector::processHistograms(MatrixWorkspace_sptr inWS) {
  const size_t nHistograms = inWS->getNumberHistograms();
  const size_t progress_items = static_cast<size_t>(double(nHistograms) * 1.2);
  Progress prog(this, 0.0, 1.0, progress_items);
  // Clone the input workspace to create a template for the denominator
  // workspace.
  IAlgorithm_sptr cloneAlg =
      this->createChildAlgorithm("CloneWorkspace", 0.0, 0.1, true);
  cloneAlg->setProperty("InputWorkspace", inWS);
  cloneAlg->setPropertyValue("OutputWorkspace", "temp");
  cloneAlg->executeAsChildAlg();
  Workspace_sptr temp = cloneAlg->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr denominatorWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(temp);

  // Choose between parallel execution and sequential execution then, process
  // histograms accordingly.
  if (m_parallelExecution == true) {
    PARALLEL_FOR2(inWS, denominatorWS)
    for (int wsIndex = 0; wsIndex < static_cast<int>(nHistograms); ++wsIndex) {
      PARALLEL_START_INTERUPT_REGION
      this->processHistogram(wsIndex, denominatorWS, inWS, prog);
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  } else {
    for (size_t wsIndex = 0; wsIndex < nHistograms; ++wsIndex) {
      this->processHistogram(wsIndex, denominatorWS, inWS, prog);
    }
  }

  return denominatorWS;
};

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
*/
void NormaliseByDetector::exec() {
  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");

  // Do the work of extracting functions and applying them to each bin on each
  // histogram. The denominator workspace is mutable.
  MatrixWorkspace_sptr denominatorWS = processHistograms(inWS);

  // Perform the normalisation.
  IAlgorithm_sptr divideAlg =
      this->createChildAlgorithm("Divide", 0.9, 1.0, true);
  divideAlg->setRethrows(true);
  divideAlg->setProperty("LHSWorkspace", inWS);
  divideAlg->setProperty("RHSWorkspace", denominatorWS);
  divideAlg->executeAsChildAlg();
  MatrixWorkspace_sptr outputWS = divideAlg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid
} // namespace Algorithms
