// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/MonitorEfficiencyCorUser.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/muParser_Silent.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramMath.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Strings.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using namespace HistogramData;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MonitorEfficiencyCorUser)

/** Initialize the algorithm's properties.
 */

void MonitorEfficiencyCorUser::init() {
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<InstrumentValidator>()),
                  "The workspace to correct for monitor efficiency");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the workspace in which to store the result.");
}

void MonitorEfficiencyCorUser::exec() {
  m_inputWS = this->getProperty("InputWorkspace");

  m_outputWS = this->getProperty("OutputWorkspace");

  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (m_outputWS != this->m_inputWS) {
    m_outputWS = create<MatrixWorkspace>(*m_inputWS);
  }
  m_Ei = m_inputWS->run().getPropertyValueAsType<double>("Ei");

  std::string mon_counts_log;

  // get name of the monitor counts sample log from the instrument parameter
  // file
  try {
    mon_counts_log = getValFromInstrumentDef("monitor_counts_log");
  } catch (Kernel::Exception::InstrumentDefinitionError &) {
    // the default value is monitor_counts
    mon_counts_log = "monitor_counts";
  }

  m_monitorCounts =
      m_inputWS->run().getPropertyValueAsType<double>(mon_counts_log);

  // get Efficiency formula from the IDF - Parameters file
  const std::string effFormula = getValFromInstrumentDef("formula_mon_eff");

  // Calculate Efficiency for E = Ei
  const double eff0 = m_monitorCounts * calculateFormulaValue(effFormula, m_Ei);

  // Calculate the number of spectra in this workspace
  const int numberOfSpectra =
      static_cast<int>(this->m_inputWS->getNumberHistograms());
  API::Progress prog(this, 0.0, 1.0, numberOfSpectra);
  int64_t numberOfSpectra_i =
      static_cast<int64_t>(numberOfSpectra); // cast to make openmp happy

  // Loop over the histograms (detector spectra)
  double factor = 1 / eff0;
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_outputWS, *m_inputWS))
  for (int64_t i = 0; i < numberOfSpectra_i; ++i) {
    PARALLEL_START_INTERUPT_REGION
    m_outputWS->setHistogram(i, m_inputWS->histogram(i) * factor);

    prog.report("Detector Efficiency correction...");
    PARALLEL_END_INTERUPT_REGION
  } // end for i
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", m_outputWS);
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
                  << "evaluated to: " << eff << '\n';
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
