//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ModeratorTzeroLinear.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
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

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

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
  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<WorkspaceUnitValidator>("TOF")),
                  "The name of the input workspace, containing events and/or "
                  "histogram data, in units of time-of-flight");
  // declare the output workspace
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
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
    const std::vector<double> gradientParam =
        m_instrument->getNumberParameter("Moderator.TimeZero.gradient");
    if (gradientParam.empty())
      throw Exception::InstrumentDefinitionError(
          "Unable to retrieve Moderator Time Zero parameters (gradient)",
          inputWS->getTitle());
    m_gradient = gradientParam[0]; //[gradient]=microsecond/Angstrom
    // conversion factor for gradient from microsecond/Angstrom to meters
    double convfactor =
        1.0e4 * PhysicalConstants::h / PhysicalConstants::NeutronMass;
    m_gradient *= convfactor; //[gradient] = meter
    const std::vector<double> interceptParam =
        m_instrument->getNumberParameter("Moderator.TimeZero.intercept");
    if (interceptParam.empty())
      throw Exception::InstrumentDefinitionError(
          "Unable to retrieve Moderator Time Zero parameters (intercept)",
          inputWS->getTitle());
    m_intercept = interceptParam[0]; //[intercept]=microsecond
    g_log.debug() << "Moderator Time Zero: gradient=" << m_gradient
                  << "intercept=" << m_intercept << std::endl;
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
  if (eventWS != NULL) {
    execEvent();
    return;
  }

  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // Check whether input = output to see whether a new workspace is required.
  if (outputWS != inputWS) {
    // Create new workspace for output from old
    outputWS = WorkspaceFactory::Instance().create(inputWS);
  }

  // do the shift in X
  const size_t numHists = inputWS->getNumberHistograms();
  Progress prog(this, 0.0, 1.0, numHists); // report progress of algorithm
  PARALLEL_FOR2(inputWS, outputWS)
  for (int i = 0; i < static_cast<int>(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION
    double t_f, L_i;
    size_t wsIndex = static_cast<size_t>(i);
    calculateTfLi(inputWS, wsIndex, t_f, L_i);
    // shift the time of flights
    if (t_f >= 0) // t_f < 0 when no detector info is available
    {
      const double scaling = L_i / (L_i + m_gradient);
      const double offset = (1 - scaling) * t_f - scaling * m_intercept;
      const MantidVec &inbins = inputWS->readX(i);
      MantidVec &outbins = outputWS->dataX(i);
      for (unsigned int j = 0; j < inbins.size(); j++) {
        outbins[j] = scaling * inbins[j] + offset;
      }
    } else {
      outputWS->dataX(i) = inputWS->readX(i);
    }
    // Copy y and e data
    outputWS->dataY(i) = inputWS->readY(i);
    outputWS->dataE(i) = inputWS->readE(i);
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
  EventWorkspace_const_sptr inputWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(matrixInputWS);

  // generate the output workspace pointer
  const size_t numHists = inputWS->getNumberHistograms();
  MatrixWorkspace_sptr matrixOutputWS = getProperty("OutputWorkspace");
  EventWorkspace_sptr outputWS;
  if (matrixOutputWS == matrixInputWS) {
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);
  } else {
    // Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        WorkspaceFactory::Instance().create("EventWorkspace", numHists, 2, 1));
    // Copy geometry over.
    WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
    // You need to copy over the data as well.
    outputWS->copyDataFrom((*inputWS));
    // Cast to the matrixOutputWS and save it
    matrixOutputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
    setProperty("OutputWorkspace", matrixOutputWS);
  }

  // Loop over the spectra
  Progress prog(this, 0.0, 1.0, numHists); // report progress of algorithm
  PARALLEL_FOR1(outputWS)
  for (int i = 0; i < static_cast<int>(numHists); ++i) {
    size_t wsIndex = static_cast<size_t>(i);
    PARALLEL_START_INTERUPT_REGION
    EventList &evlist = outputWS->getEventList(wsIndex);
    if (evlist.getNumberEvents() > 0) // don't bother with empty lists
    {
      // Calculate the time from sample to detector 'i'
      double t_f, L_i;
      calculateTfLi(matrixOutputWS, wsIndex, t_f, L_i);
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
void ModeratorTzeroLinear::calculateTfLi(MatrixWorkspace_const_sptr inputWS,
                                         size_t i, double &t_f, double &L_i) {
  static const double convFact = 1.0e-6 * sqrt(2 * PhysicalConstants::meV /
                                               PhysicalConstants::NeutronMass);
  static const double TfError = -1.0; // signal error when calculating final
                                      // time
  // Get detector position
  IDetector_const_sptr det;
  try {
    det = inputWS->getDetector(i);
  } catch (Exception::NotFoundError &) {
    t_f = TfError;
    return;
  }

  if (det->isMonitor()) {
    L_i = m_instrument->getSource()->getDistance(*det);
    t_f = 0.0; // t_f=0.0 since there is no sample to detector path
  } else {
    IComponent_const_sptr sample = m_instrument->getSample();
    try {
      L_i = m_instrument->getSource()->getDistance(*sample);
    } catch (Exception::NotFoundError &) {
      g_log.error("Unable to calculate source-sample distance");
      throw Exception::InstrumentDefinitionError(
          "Unable to calculate source-sample distance", inputWS->getTitle());
    }
    // Get final energy E_f, final velocity v_f
    std::vector<double> wsProp = det->getNumberParameter("Efixed");
    if (!wsProp.empty()) {
      double E_f = wsProp.at(0);         //[E_f]=meV
      double v_f = convFact * sqrt(E_f); //[v_f]=meter/microsec

      try {
        // obtain L_f, calculate t_f
        double L_f = det->getDistance(*sample);
        t_f = L_f / v_f;
        // g_log.debug() << "detector: " << i << " L_f=" << L_f << " t_f=" <<
        // t_f << std::endl;
      } catch (Exception::NotFoundError &) {
        g_log.error("Unable to calculate detector-sample distance");
        throw Exception::InstrumentDefinitionError(
            "Unable to calculate detector-sample distance",
            inputWS->getTitle());
      }
    } else {
      g_log.debug() << "Efixed not found for detector " << i << std::endl;
      t_f = TfError;
    }
  }
} // end of CalculateTf(const MatrixWorkspace_sptr inputWS, size_t i)

} // namespace Algorithms
} // namespace Mantid
