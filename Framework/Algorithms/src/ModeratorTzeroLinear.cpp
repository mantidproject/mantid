// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ModeratorTzeroLinear.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ModeratorTzeroLinear)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

ModeratorTzeroLinear::ModeratorTzeroLinear()
    : API::Algorithm(), m_gradient(0.), m_intercept(0.), m_instrument() {}

const std::string ModeratorTzeroLinear::name() const {
  return "ModeratorTzeroLinear";
}

const std::string ModeratorTzeroLinear::summary() const {
  return "Corrects the time of flight of an indirect geometry instrument by a "
         "time offset that is linearly dependent on the wavelength of the "
         "neutron after passing through the moderator.";
}

int ModeratorTzeroLinear::version() const { return 1; }

const std::string ModeratorTzeroLinear::category() const {
  return "CorrectionFunctions\\InstrumentCorrections";
}

void ModeratorTzeroLinear::init() {
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<WorkspaceUnitValidator>("TOF")),
                  "The name of the input workspace, containing events and/or "
                  "histogram data, in units of time-of-flight");
  declareProperty("Gradient", EMPTY_DBL(),
                  "Wavelength dependent TOF shift, units in microsec/Angstrom. "
                  "Overrides the value stored in the instrument object");
  declareProperty("Intercept", EMPTY_DBL(),
                  "TOF shift, units in microseconds. Overrides the value"
                  "stored in the instrument object");
  // declare the output workspace
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace");

} // end of void ModeratorTzeroLinear::init()

void ModeratorTzeroLinear::exec() {
  // retrieve the input workspace.
  const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Get a pointer to the instrument contained in the workspace
  m_instrument = inputWS->getInstrument();

  // deltaE-mode (should be "indirect")
  try {
    const std::vector<std::string> Emode =
        m_instrument->getStringParameter("deltaE-mode");
    if (Emode.empty())
      throw Exception::InstrumentDefinitionError("Unable to retrieve "
                                                 "instrument geometry (direct "
                                                 "or indirect) parameter",
                                                 inputWS->getTitle());
    if (Emode[0] != "indirect")
      throw Exception::InstrumentDefinitionError(
          "Instrument geometry must be of type indirect.");
  } catch (Exception::NotFoundError &) {
    g_log.error("Unable to retrieve instrument geometry (direct or indirect) "
                "parameter");
    throw Exception::InstrumentDefinitionError(
        "Unable to retrieve instrument geometry (direct or indirect) parameter",
        inputWS->getTitle());
  }

  // gradient, intercept constants
  try {

    // determine which Gradient to use
    const std::vector<double> gradientParam =
        m_instrument->getNumberParameter("Moderator.TimeZero.gradient");
    const double gradientParamManual = getProperty("Gradient");
    if (gradientParam.empty() && gradientParamManual == EMPTY_DBL())
      throw Exception::InstrumentDefinitionError(
          "Unable to retrieve Moderator Time Zero parameters (gradient)",
          inputWS->getTitle());
    if (gradientParamManual != EMPTY_DBL()) {
      m_gradient = gradientParamManual;
    } else {
      m_gradient = gradientParam[0]; //[gradient]=microsecond/Angstrom
    }
    // conversion factor for gradient from microsecond/Angstrom to meters
    double convfactor =
        1.0e4 * PhysicalConstants::h / PhysicalConstants::NeutronMass;
    m_gradient *= convfactor; //[gradient] = meter

    // determine which Intercept to use
    const std::vector<double> interceptParam =
        m_instrument->getNumberParameter("Moderator.TimeZero.intercept");
    const double interceptParamManual = getProperty("Intercept");
    if (interceptParam.empty() && interceptParamManual == EMPTY_DBL())
      throw Exception::InstrumentDefinitionError(
          "Unable to retrieve Moderator Time Zero parameters (intercept)",
          inputWS->getTitle());
    if (interceptParamManual != EMPTY_DBL()) {
      m_intercept = interceptParamManual;
    } else {
      m_intercept = interceptParam[0]; //[intercept]=microsecond
    }

    g_log.debug() << "Moderator Time Zero: gradient=" << m_gradient
                  << "intercept=" << m_intercept << '\n';
  } catch (Exception::NotFoundError &) {
    g_log.error("Unable to retrieve Moderator Time Zero parameters (gradient "
                "and intercept)");
    throw Exception::InstrumentDefinitionError("Unable to retrieve Moderator "
                                               "Time Zero parameters (gradient "
                                               "and intercept)",
                                               inputWS->getTitle());
  }

  // Run execEvent if eventWorkSpace
  EventWorkspace_const_sptr eventWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventWS != nullptr) {
    execEvent();
    return;
  }

  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // Check whether input = output to see whether a new workspace is required.
  if (outputWS != inputWS) {
    // Create new workspace for output from old
    outputWS = create<MatrixWorkspace>(*inputWS);
  }

  // do the shift in X
  const auto &spectrumInfo = inputWS->spectrumInfo();
  const size_t numHists = inputWS->getNumberHistograms();
  Progress prog(this, 0.0, 1.0, numHists); // report progress of algorithm
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int i = 0; i < static_cast<int>(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION
    double t_f, L_i;
    size_t wsIndex = static_cast<size_t>(i);
    calculateTfLi(spectrumInfo, wsIndex, t_f, L_i);

    outputWS->setHistogram(i, inputWS->histogram(i));
    // shift the time of flights
    if (t_f >= 0) // t_f < 0 when no detector info is available
    {
      const double scaling = L_i / (L_i + m_gradient);
      const double offset = (1 - scaling) * t_f - scaling * m_intercept;
      auto &outbins = outputWS->mutableX(i);
      outbins *= scaling;
      outbins += offset;
    }
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Copy units
  if (inputWS->getAxis(0)->unit()) {
    outputWS->getAxis(0)->unit() = inputWS->getAxis(0)->unit();
  }
  try {
    if (inputWS->getAxis(1)->unit()) {
      outputWS->getAxis(1)->unit() = inputWS->getAxis(1)->unit();
    }
  } catch (Exception::IndexError &) {
    // OK, so this isn't a Workspace2D
  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputWS);
}

void ModeratorTzeroLinear::execEvent() {
  g_log.information("Processing event workspace");

  const MatrixWorkspace_const_sptr matrixInputWS =
      getProperty("InputWorkspace");

  // generate the output workspace pointer
  MatrixWorkspace_sptr matrixOutputWS = getProperty("OutputWorkspace");
  if (matrixOutputWS != matrixInputWS) {
    matrixOutputWS = matrixInputWS->clone();
    setProperty("OutputWorkspace", matrixOutputWS);
  }
  auto outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);

  // Loop over the spectra
  const auto &spectrumInfo = matrixOutputWS->spectrumInfo();
  const size_t numHists = outputWS->getNumberHistograms();
  Progress prog(this, 0.0, 1.0, numHists); // report progress of algorithm
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
  for (int i = 0; i < static_cast<int>(numHists); ++i) {
    size_t wsIndex = static_cast<size_t>(i);
    PARALLEL_START_INTERUPT_REGION
    EventList &evlist = outputWS->getSpectrum(wsIndex);
    if (evlist.getNumberEvents() > 0) // don't bother with empty lists
    {
      // Calculate the time from sample to detector 'i'
      double t_f, L_i;
      calculateTfLi(spectrumInfo, wsIndex, t_f, L_i);
      if (t_f >= 0) {
        const double scaling = L_i / (L_i + m_gradient);
        // Calculate new time of flight, TOF'=scaling*(TOF-t_f-intercept)+t_f =
        // scaling*TOF + (1-scaling)*t_f - scaling*intercept
        evlist.convertTof(scaling, (1 - scaling) * t_f - scaling * m_intercept);
      }
    }
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  outputWS->clearMRU(); // Clears the Most Recent Used lists */
} // end of void ModeratorTzeroLinear::execEvent()

// calculate time from sample to detector
void ModeratorTzeroLinear::calculateTfLi(const SpectrumInfo &spectrumInfo,
                                         size_t i, double &t_f, double &L_i) {
  static const double convFact = 1.0e-6 * sqrt(2 * PhysicalConstants::meV /
                                               PhysicalConstants::NeutronMass);
  static const double TfError = -1.0; // signal error when calculating final
                                      // time

  if (!spectrumInfo.hasDetectors(i)) {
    t_f = TfError;
    return;
  }

  if (spectrumInfo.isMonitor(i)) {
    L_i = spectrumInfo.sourcePosition().distance(spectrumInfo.position(i));
    t_f = 0.0; // t_f=0.0 since there is no sample to detector path
  } else {
    L_i = spectrumInfo.l1();
    // Get final energy E_f, final velocity v_f
    auto wsProp = spectrumInfo.detector(i).getNumberParameter("Efixed");
    if (!wsProp.empty()) {
      double E_f = wsProp.at(0);         //[E_f]=meV
      double v_f = convFact * sqrt(E_f); //[v_f]=meter/microsec
      t_f = spectrumInfo.l2(i) / v_f;
    } else {
      g_log.debug() << "Efixed not found for detector " << i << '\n';
      t_f = TfError;
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
