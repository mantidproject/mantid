// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/DetectorEfficiencyCorUser.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/muParser_Silent.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Strings.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DetectorEfficiencyCorUser)

/// Algorithm's name for identification. @see Algorithm::name
const std::string DetectorEfficiencyCorUser::name() const {
  return "DetectorEfficiencyCorUser";
}

/// Algorithm's version for identification. @see Algorithm::version
int DetectorEfficiencyCorUser::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string DetectorEfficiencyCorUser::category() const {
  return "CorrectionFunctions\\EfficiencyCorrections;Inelastic\\Corrections";
}

/** Initialize the algorithm's properties.
 */
void DetectorEfficiencyCorUser::init() {
  auto val = boost::make_shared<CompositeValidator>();
  // val->add<WorkspaceUnitValidator>("Energy");
  val->add<WorkspaceUnitValidator>("DeltaE");
  val->add<HistogramValidator>();
  val->add<InstrumentValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                   Direction::Input, val),
                  "The workspace to correct for detector efficiency");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the workspace in which to store the result.");
  auto checkEi = boost::make_shared<BoundedValidator<double>>();
  checkEi->setLower(0.0);
  declareProperty("IncidentEnergy", EMPTY_DBL(), checkEi,
                  "The energy of neutrons leaving the source.");
}

/** Execute the algorithm.
 */
void DetectorEfficiencyCorUser::exec() {
  // get input properties (WSs, Ei)
  retrieveProperties();

  const size_t numberOfChannels = this->m_inputWS->blocksize();
  // Calculate the number of spectra in this workspace
  const int numberOfSpectra =
      static_cast<int>(this->m_inputWS->size() / numberOfChannels);
  API::Progress prog(this, 0.0, 1.0, numberOfSpectra);
  int64_t numberOfSpectra_i =
      static_cast<int64_t>(numberOfSpectra); // cast to make openmp happy

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_outputWS, *m_inputWS))
  for (int64_t i = 0; i < numberOfSpectra_i; ++i) {
    PARALLEL_START_INTERUPT_REGION
    const auto effFormula = retrieveFormula(i);
    // Calculate Efficiency for E = Ei
    double e;
    auto parser = generateParser(effFormula, &e);
    e = m_Ei;
    const double eff0 = evaluate(parser);
    correctHistogram(i, eff0, e, parser);

    prog.report("Detector Efficiency correction...");

    PARALLEL_END_INTERUPT_REGION
  } // end for i
  PARALLEL_CHECK_INTERUPT_REGION

  this->setProperty("OutputWorkspace", this->m_outputWS);
}

/**
 * Apply efficiency corrections to a histogram in the output workspace.
 * Efficiency = f(Ei-DeltaE) / f(Ei)
 * @param eff0 :: calculated f(Ei)
 * @param e :: reference to the parser's energy parameter
 * @param parser :: muParser used to evalute f(e)
 * @param index :: the workspace index of the histogram to correct
 */
void DetectorEfficiencyCorUser::correctHistogram(const size_t index,
                                                 const double eff0, double &e,
                                                 mu::Parser &parser) {
  const auto &xIn = m_inputWS->points(index);
  const auto &yIn = m_inputWS->y(index);
  const auto &eIn = m_inputWS->e(index);
  auto &yOut = m_outputWS->mutableY(index);
  auto &eOut = m_outputWS->mutableE(index);
  for (size_t i = 0; i < xIn.size(); ++i) {
    e = m_Ei - xIn[i];
    const double eff = evaluate(parser);
    const double corr = eff / eff0;
    yOut[i] = yIn[i] / corr;
    eOut[i] = eIn[i] / corr;
  }
}

/**
 * Calculate the value of a formula parsed by muParser
 * @param parser :: muParser object
 * @return calculated value
 * @throw InstrumentDefinitionError if parser throws during evaluation
 */
double DetectorEfficiencyCorUser::evaluate(const mu::Parser &parser) const {
  try {
    return parser.Eval();
  } catch (mu::Parser::exception_type &e) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Error calculating formula from string. Muparser error message is: " +
        e.GetMsg());
  }
}

mu::Parser DetectorEfficiencyCorUser::generateParser(const std::string &formula,
                                                     double *e) const {
  mu::Parser p;
  p.DefineVar("e", e);
  p.SetExpr(formula);
  return p;
}

/**
 * Returns the efficiency correction formula associated to a detector
 * @param workspaceIndex detector's workspace index
 * @return the efficiency correction formula
 */
std::string
DetectorEfficiencyCorUser::retrieveFormula(const size_t workspaceIndex) {
  const std::string formulaParamName("formula_eff");
  const auto &paramMap = m_inputWS->constInstrumentParameters();
  auto det = m_inputWS->getDetector(workspaceIndex);
  auto param = paramMap.getRecursive(det.get(), formulaParamName, "string");
  if (!param) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "No <" + formulaParamName + "> parameter found for component '" +
        det->getFullName() + "' in the instrument definition.");
  }
  const auto ret = param->asString();
  g_log.debug() << "Found formula for workspace index " << workspaceIndex
                << ": " << ret << "\n";
  return ret;
}

/** Loads and checks the values passed to the algorithm
 *
 *  @throw invalid_argument if there is an incapatible property value so the
 *algorithm can't continue
 */
void DetectorEfficiencyCorUser::retrieveProperties() {
  // Get the workspaces
  m_inputWS = this->getProperty("InputWorkspace");

  m_outputWS = this->getProperty("OutputWorkspace");

  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (m_outputWS != this->m_inputWS) {
    m_outputWS.reset(
        Mantid::DataObjects::create<DataObjects::Workspace2D>(*m_inputWS)
            .release());
  }

  // these first three properties are fully checked by validators
  m_Ei = this->getProperty("IncidentEnergy");
  // If we're not given an Ei, see if one has been set.
  if (m_Ei == EMPTY_DBL()) {
    Mantid::Kernel::Property *prop = m_inputWS->run().getProperty("Ei");
    double val;
    if (!prop || !Strings::convert(prop->value(), val)) {
      throw std::invalid_argument(
          "No Ei value has been set or stored within the run information.");
    }
    m_Ei = val;
    g_log.debug() << "Using stored Ei value " << m_Ei << "\n";
  } else {
    g_log.debug() << "Using user input Ei value: " << m_Ei << "\n";
  }
}
} // namespace Algorithms
// namespace Algorithms
} // namespace Mantid
