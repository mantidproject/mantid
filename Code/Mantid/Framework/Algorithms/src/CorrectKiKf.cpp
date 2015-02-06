//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CorrectKiKf.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {

// Register with the algorithm factory
DECLARE_ALGORITHM(CorrectKiKf)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace Geometry;
using std::size_t;

/// Default constructor
CorrectKiKf::CorrectKiKf() : Algorithm() {}

/// Destructor
CorrectKiKf::~CorrectKiKf() {}

/// Initialisation method
void CorrectKiKf::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("DeltaE");

  this->declareProperty(
      new WorkspaceProperty<API::MatrixWorkspace>(
          "InputWorkspace", "", Direction::Input, wsValidator),
      "Name of the input workspace");
  this->declareProperty(
      new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace", "",
                                                  Direction::Output),
      "Name of the output workspace, can be the same as the input");

  std::vector<std::string> propOptions;
  propOptions.push_back("Direct");
  propOptions.push_back("Indirect");
  this->declareProperty("EMode", "Direct",
                        boost::make_shared<StringListValidator>(propOptions),
                        "The energy mode (default: Direct)");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  this->declareProperty("EFixed", EMPTY_DBL(), mustBePositive,
                        "Value of fixed energy in meV : EI (EMode=Direct) or "
                        "EF (EMode=Indirect) .");
}

void CorrectKiKf::exec() {
  // Get the workspaces
  this->inputWS = this->getProperty("InputWorkspace");
  this->outputWS = this->getProperty("OutputWorkspace");

  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (this->outputWS != this->inputWS) {
    this->outputWS = API::WorkspaceFactory::Instance().create(this->inputWS);
  }

  // Check if it is an event workspace
  EventWorkspace_const_sptr eventW =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventW != NULL) {
    this->execEvent();
    return;
  }

  const size_t size = this->inputWS->blocksize();
  // Calculate the number of spectra in this workspace
  const int numberOfSpectra = static_cast<int>(this->inputWS->size() / size);
  API::Progress prog(this, 0.0, 1.0, numberOfSpectra);
  const bool histogram = this->inputWS->isHistogramData();
  bool negativeEnergyWarning = false;

  const std::string emodeStr = getProperty("EMode");
  double efixedProp = getProperty("EFixed");

  if (efixedProp == EMPTY_DBL()) {
    if (emodeStr == "Direct") {
      // Check if it has been store on the run object for this workspace
      if (this->inputWS->run().hasProperty("Ei")) {
        Kernel::Property *eiprop = this->inputWS->run().getProperty("Ei");
        efixedProp = boost::lexical_cast<double>(eiprop->value());
        g_log.debug() << "Using stored Ei value " << efixedProp << "\n";
      } else {
        throw std::invalid_argument(
            "No Ei value has been set or stored within the run information.");
      }
    } else {
      // If not specified, will try to get Ef from the parameter file for
      // indirect geometry,
      // but it will be done for each spectrum separately, in case of different
      // analyzer crystals
    }
  }

  // Get the parameter map
  const ParameterMap &pmap = outputWS->constInstrumentParameters();

  PARALLEL_FOR2(inputWS, outputWS)
  for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i) {
    PARALLEL_START_INTERUPT_REGION
    double Efi = 0;
    // Now get the detector object for this histogram to check if monitor
    // or to get Ef for indirect geometry
    if (emodeStr == "Indirect") {
      if (efixedProp != EMPTY_DBL())
        Efi = efixedProp;
      else
        try {
          IDetector_const_sptr det = inputWS->getDetector(i);
          if (!det->isMonitor()) {
            try {
              Parameter_sptr par = pmap.getRecursive(det.get(), "Efixed");
              if (par) {
                Efi = par->value<double>();
                g_log.debug() << "Detector: " << det->getID()
                              << " EFixed: " << Efi << "\n";
              }
            } catch (std::runtime_error &) { /* Throws if a DetectorGroup, use
                                                single provided value */
            }
          }

        } catch (std::runtime_error &) {
          g_log.information() << "Workspace Index " << i
                              << ": cannot find detector"
                              << "\n";
        }
    }

    MantidVec &yOut = outputWS->dataY(i);
    MantidVec &eOut = outputWS->dataE(i);
    const MantidVec &xIn = inputWS->readX(i);
    const MantidVec &yIn = inputWS->readY(i);
    const MantidVec &eIn = inputWS->readE(i);
    // Copy the energy transfer axis
    outputWS->setX(i, inputWS->refX(i));
    for (unsigned int j = 0; j < size; ++j) {
      const double deltaE = histogram ? 0.5 * (xIn[j] + xIn[j + 1]) : xIn[j];
      double Ei = 0.;
      double Ef = 0.;
      double kioverkf = 1.;
      if (emodeStr == "Direct") // Ei=Efixed
      {
        Ei = efixedProp;
        Ef = Ei - deltaE;
      } else // Ef=Efixed
      {
        Ef = Efi;
        Ei = Efi + deltaE;
      }
      // if Ei or Ef is negative, it should be a warning
      // however, if the intensity is 0 (histogram goes to energy transfer
      // higher than Ei) it is still ok, so no warning.
      if ((Ei <= 0) || (Ef <= 0)) {
        kioverkf = 0.;
        if (yIn[j] != 0)
          negativeEnergyWarning = true;
      } else
        kioverkf = std::sqrt(Ei / Ef);

      yOut[j] = yIn[j] * kioverkf;
      eOut[j] = eIn[j] * kioverkf;
    }
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  } // end for i
  PARALLEL_CHECK_INTERUPT_REGION

  if (negativeEnergyWarning)
    g_log.information() << "Ef <= 0 or Ei <= 0 in at least one spectrum!!!!"
                        << std::endl;
  if ((negativeEnergyWarning) && (efixedProp == EMPTY_DBL()))
    g_log.information() << "Try to set fixed energy" << std::endl;
  this->setProperty("OutputWorkspace", this->outputWS);
  return;
}

/**
 * Execute CorrectKiKf for event workspaces
 *
 */

void CorrectKiKf::execEvent() {
  g_log.information("Processing event workspace");

  const MatrixWorkspace_const_sptr matrixInputWS =
      this->getProperty("InputWorkspace");
  EventWorkspace_const_sptr inputWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(matrixInputWS);

  // generate the output workspace pointer
  API::MatrixWorkspace_sptr matrixOutputWS =
      this->getProperty("OutputWorkspace");
  EventWorkspace_sptr outputWS;
  if (matrixOutputWS == matrixInputWS)
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);
  else {
    // Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create(
            "EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    // Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS,
                                                           false);
    // You need to copy over the data as well.
    outputWS->copyDataFrom((*inputWS));

    // Cast to the matrixOutputWS and save it
    matrixOutputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
    this->setProperty("OutputWorkspace", matrixOutputWS);
  }

  const std::string emodeStr = getProperty("EMode");
  double efixedProp = getProperty("EFixed"), efixed;

  if (efixedProp == EMPTY_DBL()) {
    if (emodeStr == "Direct") {
      // Check if it has been store on the run object for this workspace
      if (this->inputWS->run().hasProperty("Ei")) {
        Kernel::Property *eiprop = this->inputWS->run().getProperty("Ei");
        efixedProp = boost::lexical_cast<double>(eiprop->value());
        g_log.debug() << "Using stored Ei value " << efixedProp << "\n";
      } else {
        throw std::invalid_argument(
            "No Ei value has been set or stored within the run information.");
      }
    } else {
      // If not specified, will try to get Ef from the parameter file for
      // indirect geometry,
      // but it will be done for each spectrum separately, in case of different
      // analyzer crystals
    }
  }

  // Get the parameter map
  const ParameterMap &pmap = outputWS->constInstrumentParameters();

  int64_t numHistograms = static_cast<int64_t>(inputWS->getNumberHistograms());
  API::Progress prog = API::Progress(this, 0.0, 1.0, numHistograms);
  PARALLEL_FOR1(outputWS)
  for (int64_t i = 0; i < numHistograms; ++i) {
    PARALLEL_START_INTERUPT_REGION

    double Efi = 0;
    // Now get the detector object for this histogram to check if monitor
    // or to get Ef for indirect geometry
    if (emodeStr == "Indirect") {
      if (efixedProp != EMPTY_DBL())
        Efi = efixedProp;
      else
        try {
          IDetector_const_sptr det = inputWS->getDetector(i);
          if (!det->isMonitor()) {
            try {
              Parameter_sptr par = pmap.getRecursive(det.get(), "Efixed");
              if (par) {
                Efi = par->value<double>();
                g_log.debug() << "Detector: " << det->getID()
                              << " EFixed: " << Efi << "\n";
              }
            } catch (std::runtime_error &) { /* Throws if a DetectorGroup, use
                                                single provided value */
            }
          }

        } catch (std::runtime_error &) {
          g_log.information() << "Workspace Index " << i
                              << ": cannot find detector"
                              << "\n";
        }
    }

    if (emodeStr == "Indirect")
      efixed = Efi;
    else
      efixed = efixedProp;

    // Do the correction
    EventList *evlist = outputWS->getEventListPtr(i);
    switch (evlist->getEventType()) {
    case TOF:
      // Switch to weights if needed.
      evlist->switchTo(WEIGHTED);
    /* no break */
    // Fall through

    case WEIGHTED:
      correctKiKfEventHelper(evlist->getWeightedEvents(), efixed, emodeStr);
      break;

    case WEIGHTED_NOTIME:
      correctKiKfEventHelper(evlist->getWeightedEventsNoTime(), efixed,
                             emodeStr);
      break;
    }

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  outputWS->clearMRU();
  if (inputWS->getNumberEvents() != outputWS->getNumberEvents()) {
    g_log.information() << "Ef <= 0 or Ei <= 0 for "
                        << inputWS->getNumberEvents() -
                               outputWS->getNumberEvents() << " events, out of "
                        << inputWS->getNumberEvents() << std::endl;
    if (efixedProp == EMPTY_DBL())
      g_log.information() << "Try to set fixed energy" << std::endl;
  }
}

template <class T>
void CorrectKiKf::correctKiKfEventHelper(std::vector<T> &wevector,
                                         double efixed,
                                         const std::string emodeStr) {
  double Ei, Ef;
  float kioverkf;
  typename std::vector<T>::iterator it;
  for (it = wevector.begin(); it < wevector.end();) {
    if (emodeStr == "Direct") // Ei=Efixed
    {
      Ei = efixed;
      Ef = Ei - it->tof();
    } else // Ef=Efixed
    {
      Ef = efixed;
      Ei = Ef + it->tof();
    }
    // if Ei or Ef is negative, delete the event
    if ((Ei <= 0) || (Ef <= 0)) {
      it = wevector.erase(it);
    } else {
      kioverkf = static_cast<float>(std::sqrt(Ei / Ef));
      it->m_weight *= kioverkf;
      it->m_errorSquared *= kioverkf * kioverkf;
      ++it;
    }
  }
}

} // namespace Algorithm
} // namespace Mantid
