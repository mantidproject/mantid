//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ModeratorTzero.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/muParser_Silent.h"
#include <boost/lexical_cast.hpp>
#include "MantidDataObjects/EventList.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ModeratorTzero)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

/// set attribute m_formula
void ModeratorTzero::setFormula(const std::string &formula) {
  m_formula = formula;
}

void ModeratorTzero::init() {

  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input workspace, containing events and/or "
                  "histogram data, in units of time-of-flight");
  // declare the output workspace
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "The name of the output workspace");
  declareProperty(new Kernel::PropertyWithValue<double>(
                      "tolTOF", 0.1, Kernel::Direction::Input),
                  "Tolerance in the calculation of the emission time, in "
                  "microseconds (default:1)");
  declareProperty(new Kernel::PropertyWithValue<size_t>(
                      "Niter", 1, Kernel::Direction::Input),
                  "Number of iterations (default:1)");

} // end of void ModeratorTzero::init()

void ModeratorTzero::exec() {
  m_tolTOF = getProperty("tolTOF"); // Tolerance in the calculation of the
                                    // emission time, in microseconds
  m_niter = getProperty("Niter");   // number of iterations
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  m_instrument = inputWS->getInstrument(); // pointer to the instrument

  // deltaE-mode (should be "indirect")
  std::vector<std::string> Emode =
      m_instrument->getStringParameter("deltaE-mode");
  if (Emode.empty())
    throw Exception::InstrumentDefinitionError(
        "Unable to retrieve instrument geometry (direct or indirect) parameter",
        inputWS->getTitle());
  if (Emode[0] != "indirect")
    throw Exception::InstrumentDefinitionError(
        "Instrument geometry must be of type indirect.");

  // extract formula from instrument parameters
  std::vector<std::string> t0_formula =
      m_instrument->getStringParameter("t0_formula");
  if (t0_formula.empty())
    throw Exception::InstrumentDefinitionError(
        "Unable to retrieve t0_formula among instrument parameters");
  m_formula = t0_formula[0];

  // Run execEvent if eventWorkSpace
  EventWorkspace_const_sptr eventWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventWS != NULL) {
    execEvent();
    return;
  }

  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // Check whether input == output to see whether a new workspace is required.
  if (outputWS != inputWS) {
    // Create new workspace for output from old
    outputWS = WorkspaceFactory::Instance().create(inputWS);
  }

  const size_t numHists = static_cast<size_t>(inputWS->getNumberHistograms());
  Progress prog(this, 0.0, 1.0, numHists); // report progress of algorithm
  PARALLEL_FOR2(inputWS, outputWS)
  // iterate over the spectra
  for (int i = 0; i < static_cast<int>(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION
    size_t wsIndex = static_cast<size_t>(i);
    double L1 = CalculateL1(
        inputWS, wsIndex); // distance from source to sample or monitor
    double t2 = CalculateT2(inputWS, wsIndex); // time from sample to detector
    // shift the time of flights by the emission time from the moderator
    if (t2 >= 0) // t2 < 0 when no detector info is available
    {
      double E1;
      mu::Parser parser;
      parser.DefineVar("incidentEnergy", &E1); // associate E1 to this parser
      parser.SetExpr(m_formula);
      E1 = m_convfactor * (L1 / m_t1min) * (L1 / m_t1min);
      double min_t0_next = parser.Eval(); // fast neutrons are shifted by
                                          // min_t0_next, irrespective of tof
      MantidVec &inbins = inputWS->dataX(i);
      MantidVec &outbins = outputWS->dataX(i);

      // iterate over the time-of-flight values
      for (unsigned int ibin = 0; ibin < inbins.size(); ibin++) {
        double tof = inbins[ibin]; // current time-of-flight
        if (tof < m_t1min + t2)
          tof -= min_t0_next;
        else
          tof -= CalculateT0(tof, L1, t2, E1, parser);
        outbins[ibin] = tof;
      }
    } else {
      outputWS->dataX(i) = inputWS->dataX(i);
    }
    // Copy y and e data
    outputWS->dataY(i) = inputWS->dataY(i);
    outputWS->dataE(i) = inputWS->dataE(i);
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Copy units
  if (inputWS->getAxis(0)->unit().get()) {
    outputWS->getAxis(0)->unit() = inputWS->getAxis(0)->unit();
  }
  try {
    if (inputWS->getAxis(1)->unit().get()) {
      outputWS->getAxis(1)->unit() = inputWS->getAxis(1)->unit();
    }
  } catch (Exception::IndexError &) {
    // OK, so this isn't a Workspace2D
  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputWS);
}

void ModeratorTzero::execEvent() {
  g_log.information("Processing event workspace");

  const MatrixWorkspace_const_sptr matrixInputWS =
      getProperty("InputWorkspace");
  EventWorkspace_const_sptr inputWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(matrixInputWS);

  // generate the output workspace pointer
  const size_t numHists = static_cast<size_t>(inputWS->getNumberHistograms());
  Mantid::API::MatrixWorkspace_sptr matrixOutputWS =
      getProperty("OutputWorkspace");
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

  // Get a pointer to the sample
  IComponent_const_sptr sample = outputWS->getInstrument()->getSample();

  // Loop over the spectra
  Progress prog(this, 0.0, 1.0, numHists); // report progress of algorithm
  PARALLEL_FOR1(outputWS)
  for (int i = 0; i < static_cast<int>(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION
    size_t wsIndex = static_cast<size_t>(i);
    EventList &evlist = outputWS->getEventList(wsIndex);
    if (evlist.getNumberEvents() > 0) // don't bother with empty lists
    {
      double L1 = CalculateL1(
          matrixOutputWS, wsIndex); // distance from source to sample or monitor
      double t2 =
          CalculateT2(matrixOutputWS, wsIndex); // time from sample to detector
      if (t2 >= 0) // t2 < 0 when no detector info is available
      {
        double tof, E1;
        mu::Parser parser;
        parser.DefineVar("incidentEnergy",
                         &E1); // associate variable E1 to this parser
        parser.SetExpr(m_formula);
        E1 = m_convfactor * (L1 / m_t1min) * (L1 / m_t1min);
        double min_t0_next = parser.Eval(); // fast neutrons are shifted by
                                            // min_t0_next, irrespective of tof

        // fix the histogram bins
        MantidVec &x = evlist.dataX();
        for (MantidVec::iterator iter = x.begin(); iter != x.end(); ++iter) {
          tof = *iter;
          if (tof < m_t1min + t2)
            tof -= min_t0_next;
          else
            tof -= CalculateT0(tof, L1, t2, E1, parser);
          *iter = tof;
        }

        MantidVec tofs = evlist.getTofs();
        for (unsigned int itof = 0; itof < tofs.size(); itof++) {
          tof = tofs[itof] + 0.002 * (rand() % 100 - 50); // add a [-0.1,0.1]
                                                          // microsecond noise
                                                          // to avoid artifacts
                                                          // resulting from
                                                          // original tof data
          if (tof < m_t1min + t2)
            tof -= min_t0_next;
          else
            tof -= CalculateT0(tof, L1, t2, E1, parser);
          tofs[itof] = tof;
        }
        evlist.setTofs(tofs);
        evlist.setSortOrder(Mantid::DataObjects::EventSortType::UNSORTED);
      }
    }
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  outputWS->clearMRU(); // Clears the Most Recent Used lists */
} // end of void ModeratorTzero::execEvent()

/// calculate distance from source to sample or detector
double ModeratorTzero::CalculateL1(Mantid::API::MatrixWorkspace_sptr inputWS,
                                   size_t i) {
  double L1(0);
  // Get detector position
  IDetector_const_sptr det;
  try {
    det = inputWS->getDetector(i);
  } catch (Exception::NotFoundError &) {
    return 0;
  }

  if (det->isMonitor()) {
    L1 = m_instrument->getSource()->getDistance(*det);
  } else {
    IComponent_const_sptr sample = m_instrument->getSample();
    try {
      L1 = m_instrument->getSource()->getDistance(*sample);
    } catch (Exception::NotFoundError &) {
      g_log.error("Unable to calculate source-sample distance");
      throw Exception::InstrumentDefinitionError(
          "Unable to calculate source-sample distance", inputWS->getTitle());
    }
  }
  return L1;
}

// calculate time from sample to detector
double ModeratorTzero::CalculateT2(MatrixWorkspace_sptr inputWS, size_t i) {
  static const double convFact = 1.0e-6 * sqrt(2 * PhysicalConstants::meV /
                                               PhysicalConstants::NeutronMass);
  double t2(-1.0); // negative initialization signals error
  // Get detector position
  IDetector_const_sptr det;
  try {
    det = inputWS->getDetector(i);
  } catch (Exception::NotFoundError &) {
    return t2;
  }

  if (det->isMonitor()) {
    t2 = 0.0; // t2=0.0 since there is no sample to detector path
  } else {
    IComponent_const_sptr sample = m_instrument->getSample();
    // Get final energy E_f, final velocity v_f
    std::vector<double> wsProp = det->getNumberParameter("Efixed");
    if (!wsProp.empty()) {
      double E2 = wsProp.at(0);        //[E2]=meV
      double v2 = convFact * sqrt(E2); //[v2]=meter/microsec
      try {
        double L2 = det->getDistance(*sample);
        t2 = L2 / v2;
      } catch (Exception::NotFoundError &) {
        g_log.error("Unable to calculate detector-sample distance");
        throw Exception::InstrumentDefinitionError(
            "Unable to calculate detector-sample distance",
            inputWS->getTitle());
      }
    } else {
      g_log.debug() << "Efixed not found for detector " << i << std::endl;
    }
  }
  return t2;
} // end of CalculateT2(const MatrixWorkspace_sptr inputWS, size_t i)

/// Calculate emission time for a given detector (L1, t2) and TOF
double ModeratorTzero::CalculateT0(const double &tof, const double &L1,
                                   const double &t2, double &E1,
                                   mu::Parser &parser) {
  double t0_curr, t0_next;
  t0_curr = m_tolTOF; // current iteration emission time
  t0_next = 0.0;      // next iteration emission time, initialized to zero
  size_t iiter(0);    // current iteration number
  // iterate until convergence in t0 reached
  while (std::fabs(t0_curr - t0_next) >= m_tolTOF && iiter < m_niter) {
    t0_curr = t0_next;
    double t1 = tof - t0_curr - t2;
    double v1 = L1 / t1;
    E1 = m_convfactor * v1 * v1; // Energy in meV if v1 in meter/microsecond
    t0_next = parser.Eval();
    iiter++;
  }
  return t0_next;
}

double ModeratorTzero::gett1min() { return m_t1min; }

} // namespace Algorithms
} // namespace Mantid
