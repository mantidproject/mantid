// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/NormaliseByDetector.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/muParser_Silent.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(NormaliseByDetector)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
NormaliseByDetector::NormaliseByDetector(bool parallelExecution) : m_parallelExecution(parallelExecution) {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string NormaliseByDetector::name() const { return "NormaliseByDetector"; }

/// Algorithm's version for identification. @see Algorithm::version
int NormaliseByDetector::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string NormaliseByDetector::category() const { return "CorrectionFunctions\\NormalisationCorrections"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void NormaliseByDetector::init() {
  auto compositeValidator = std::make_shared<CompositeValidator>();
  compositeValidator->add(std::make_shared<API::WorkspaceUnitValidator>("Wavelength"));
  compositeValidator->add(std::make_shared<API::HistogramValidator>());

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input, compositeValidator),
      "An input workspace in wavelength");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

const Geometry::FitParameter NormaliseByDetector::tryParseFunctionParameter(const Geometry::Parameter_sptr &parameter,
                                                                            const Geometry::IDetector &det) {
  if (parameter == nullptr) {
    std::stringstream stream;
    stream << det.getName()
           << " and all of it's parent components, have no "
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
void NormaliseByDetector::processHistogram(size_t wsIndex, const MatrixWorkspace_const_sptr &inWS,
                                           const MatrixWorkspace_sptr &denominatorWS, Progress &prog) {
  const auto &paramMap = inWS->constInstrumentParameters();
  const auto &spectrumInfo = inWS->spectrumInfo();
  const auto &det = spectrumInfo.detector(wsIndex);
  const std::string type = "fitting";
  Geometry::Parameter_sptr foundParam = paramMap.getRecursiveByType(&det, type);

  const Geometry::FitParameter &foundFittingParam = tryParseFunctionParameter(foundParam, det);

  const std::string &fitFunctionName = foundFittingParam.getFunction();
  IFunction_sptr function = FunctionFactory::Instance().createFunction(fitFunctionName);
  using ParamNames = std::vector<std::string>;
  ParamNames allParamNames = function->getParameterNames();

  // Lookup each parameter name.
  for (auto &paramName : allParamNames) {
    Geometry::Parameter_sptr param = paramMap.getRecursive(&det, paramName, type);

    const Geometry::FitParameter &fitParam = tryParseFunctionParameter(param, det);

    if (fitParam.getFormula().empty()) {
      throw std::runtime_error("A Forumla has not been provided for a fit function");
    } else {
      const std::string &resultUnitStr = fitParam.getResultUnit();
      if (!resultUnitStr.empty() && resultUnitStr != "Wavelength") {
        throw std::runtime_error("Units for function parameters must be in Wavelength");
      }
    }
    mu::Parser p;
    p.SetExpr(fitParam.getFormula());
    double paramValue = p.Eval();
    // Set the function coeffiecents.
    function->setParameter(fitParam.getName(), paramValue);
  }

  auto wavelengths = inWS->points(wsIndex);
  FunctionDomain1DVector domain(wavelengths.rawData());
  FunctionValues values(domain);
  function->function(domain, values);

  auto &Y = denominatorWS->mutableY(wsIndex);
  for (size_t i = 0; i < domain.size(); ++i) {
    Y[i] = values[i];
  }

  denominatorWS->mutableE(wsIndex) = 0.0;

  prog.report();
}

/**
Controlling function. Processes the histograms either in parallel or
sequentially.
@param inWS: Workspace input. Contains instrument to use as well as X data to
use.
*/
MatrixWorkspace_sptr NormaliseByDetector::processHistograms(const MatrixWorkspace_sptr &inWS) {
  const size_t nHistograms = inWS->getNumberHistograms();
  const auto progress_items = static_cast<size_t>(double(nHistograms) * 1.2);
  Progress prog(this, 0.0, 1.0, progress_items);
  // Clone the input workspace to create a template for the denominator
  // workspace.
  auto cloneAlg = createChildAlgorithm("CloneWorkspace", 0.0, 0.1, true);
  cloneAlg->setProperty("InputWorkspace", inWS);
  cloneAlg->setPropertyValue("OutputWorkspace", "temp");
  cloneAlg->executeAsChildAlg();
  Workspace_sptr temp = cloneAlg->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr denominatorWS = std::dynamic_pointer_cast<MatrixWorkspace>(temp);

  // Choose between parallel execution and sequential execution then, process
  // histograms accordingly.
  if (m_parallelExecution) {
    PARALLEL_FOR_IF(Kernel::threadSafe(*inWS, *denominatorWS))
    for (int wsIndex = 0; wsIndex < static_cast<int>(nHistograms); ++wsIndex) {
      PARALLEL_START_INTERRUPT_REGION
      this->processHistogram(wsIndex, inWS, denominatorWS, prog);
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  } else {
    for (size_t wsIndex = 0; wsIndex < nHistograms; ++wsIndex) {
      this->processHistogram(wsIndex, inWS, denominatorWS, prog);
    }
  }

  return denominatorWS;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void NormaliseByDetector::exec() {
  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");

  // Do the work of extracting functions and applying them to each bin on each
  // histogram. The denominator workspace is mutable.
  MatrixWorkspace_sptr denominatorWS = processHistograms(inWS);

  // Perform the normalisation.
  auto divideAlg = createChildAlgorithm("Divide", 0.9, 1.0, true);
  divideAlg->setRethrows(true);
  divideAlg->setProperty("LHSWorkspace", inWS);
  divideAlg->setProperty("RHSWorkspace", denominatorWS);
  divideAlg->executeAsChildAlg();
  MatrixWorkspace_sptr outputWS = divideAlg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::Algorithms
