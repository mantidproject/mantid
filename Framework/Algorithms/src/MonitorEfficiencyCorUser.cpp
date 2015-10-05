#include "MantidAlgorithms/MonitorEfficiencyCorUser.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidGeometry/muParser_Silent.h"
#include "MantidKernel/MultiThreaded.h"
#include <ctime>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MonitorEfficiencyCorUser)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MonitorEfficiencyCorUser::MonitorEfficiencyCorUser() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MonitorEfficiencyCorUser::~MonitorEfficiencyCorUser() {}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */

void MonitorEfficiencyCorUser::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,
                              boost::make_shared<InstrumentValidator>()),
      "The workspace to correct for monitor efficiency");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name of the workspace in which to store the result.");
}

void MonitorEfficiencyCorUser::exec() {
  m_inputWS = this->getProperty("InputWorkspace");

  m_outputWS = this->getProperty("OutputWorkspace");

  if (m_inputWS->getInstrument()->getName() != "TOFTOF") {
    std::string message("The input workspace does not come from TOFTOF");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (m_outputWS != this->m_inputWS) {
    m_outputWS = API::WorkspaceFactory::Instance().create(m_inputWS);
  }
  double val;
  Strings::convert(m_inputWS->run().getProperty("Ei")->value(), val);
  m_Ei = val;
  Strings::convert(m_inputWS->run().getProperty("monitor_counts")->value(),
                   m_monitorCounts);

  // get Efficiency formula from the IDF - Parameters file
  const std::string effFormula = getValFromInstrumentDef("formula_mon_eff");

  // Calculate Efficiency for E = Ei
  const double eff0 = m_monitorCounts * calculateFormulaValue(effFormula, m_Ei);

  const size_t numberOfChannels = this->m_inputWS->blocksize();
  // Calculate the number of spectra in this workspace
  const int numberOfSpectra =
      static_cast<int>(this->m_inputWS->getNumberHistograms());
  API::Progress prog(this, 0.0, 1.0, numberOfSpectra);
  int64_t numberOfSpectra_i =
      static_cast<int64_t>(numberOfSpectra); // cast to make openmp happy

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR2(m_outputWS, m_inputWS)
  for (int64_t i = 0; i < numberOfSpectra_i; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // MantidVec& xOut = m_outputWS->dataX(i);
    MantidVec &yOut = m_outputWS->dataY(i);
    MantidVec &eOut = m_outputWS->dataE(i);
    const MantidVec &yIn = m_inputWS->readY(i);
    const MantidVec &eIn = m_inputWS->readE(i);
    m_outputWS->setX(i, m_inputWS->refX(i));

    applyMonEfficiency(numberOfChannels, yIn, eIn, eff0, yOut, eOut);

    prog.report("Detector Efficiency correction...");
    PARALLEL_END_INTERUPT_REGION
  } // end for i
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", m_outputWS);
}

/**
 * Apply the monitor efficiency to a single spectrum
 * @param numberOfChannels Number of channels in a spectra (nbins - 1)
 * @param yIn spectrum counts
 * @param eIn spectrum errors
 * @param effVec efficiency values (to be divided by the counts)
 * @param yOut corrected spectrum counts
 * @param eOut corrected spectrum errors
 */
void MonitorEfficiencyCorUser::applyMonEfficiency(
    const size_t numberOfChannels, const MantidVec &yIn, const MantidVec &eIn,
    const double effVec, MantidVec &yOut, MantidVec &eOut) {

  for (unsigned int j = 0; j < numberOfChannels; ++j) {
    yOut[j] = yIn[j] / effVec;
    eOut[j] = eIn[j] / effVec;
  }
}

/**
 * Calculate the value of a formula
 * @param formula :: Formula to correct
 * @param energy :: value of the energy to use in the formula
 * @return calculated corrected value of the monitor count
 */
double
MonitorEfficiencyCorUser::calculateFormulaValue(const std::string &formula,
                                                double energy) {
  try {
    mu::Parser p;
    p.DefineVar("e", &energy);
    p.SetExpr(formula);
    double eff = p.Eval();
    g_log.debug() << "Formula: " << formula << " with: " << energy
                  << "evaluated to: " << eff << std::endl;
    return eff;

  } catch (mu::Parser::exception_type &e) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Error calculating formula from string. Muparser error message is: " +
        e.GetMsg());
  }
}

/**
 * Returns the value associated to a parameter name in the IDF
 * @param parameterName :: parameter name in the IDF
 * @return the value associated to the parameter name
 */
std::string MonitorEfficiencyCorUser::getValFromInstrumentDef(
    const std::string &parameterName) {

  const ParameterMap &pmap = m_inputWS->constInstrumentParameters();
  Instrument_const_sptr instrument = m_inputWS->getInstrument();
  Parameter_sptr par =
      pmap.getRecursive(instrument->getChild(0).get(), parameterName);
  if (par) {
    std::string ret = par->asString();
    g_log.debug() << "Parsed parameter " << parameterName << ": " << ret
                  << "\n";
    return ret;
  } else {
    throw Kernel::Exception::InstrumentDefinitionError(
        "There is no <" + parameterName + "> in the instrument definition!");
  }
}

} // namespace Algorithms
} // namespace Mantid
