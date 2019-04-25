// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CorrectKiKf.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid {
namespace Algorithms {

// Register with the algorithm factory
DECLARE_ALGORITHM(CorrectKiKf)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace Geometry;
using std::size_t;

/// Initialisation method
void CorrectKiKf::init() {
  auto wsValidator = boost::make_shared<WorkspaceUnitValidator>("DeltaE");

  this->declareProperty(
      make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
          "InputWorkspace", "", Direction::Input, wsValidator),
      "Name of the input workspace");
  this->declareProperty(
      make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "Name of the output workspace, can be the same as the input");

  std::vector<std::string> propOptions{"Direct", "Indirect"};
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
  MatrixWorkspace_const_sptr inputWS = this->getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = this->getProperty("OutputWorkspace");

  // Check if it is an event workspace
  EventWorkspace_const_sptr eventW =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventW != nullptr) {
    this->execEvent();
    return;
  }

  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (outputWS != inputWS) {
    outputWS = create<MatrixWorkspace>(*inputWS);
  }

  const size_t size = inputWS->blocksize();
  // Calculate the number of spectra in this workspace
  const int numberOfSpectra = static_cast<int>(inputWS->size() / size);
  API::Progress prog(this, 0.0, 1.0, numberOfSpectra);
  bool negativeEnergyWarning = false;

  const std::string emodeStr = getProperty("EMode");
  double efixedProp = getProperty("EFixed");

  if (efixedProp == EMPTY_DBL()) {
    if (emodeStr == "Direct") {
      // Check if it has been store on the run object for this workspace
      if (inputWS->run().hasProperty("Ei")) {
        efixedProp = inputWS->run().getPropertyValueAsType<double>("Ei");
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
  const auto &spectrumInfo = inputWS->spectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i) {
    PARALLEL_START_INTERUPT_REGION
    double Efi = 0;
    // Now get the detector object for this histogram to check if monitor
    // or to get Ef for indirect geometry
    if (emodeStr == "Indirect") {
      if (efixedProp != EMPTY_DBL())
        Efi = efixedProp;
      // If a DetectorGroup is present should provide a value as a property
      // instead
      else if (spectrumInfo.hasUniqueDetector(i)) {
        getEfixedFromParameterMap(Efi, i, spectrumInfo, pmap);
      } else {
        g_log.information()
            << "Workspace Index " << i << ": cannot find detector"
            << "\n";
      }
    }

    auto &yOut = outputWS->mutableY(i);
    auto &eOut = outputWS->mutableE(i);
    const auto &xIn = inputWS->points(i);
    auto &yIn = inputWS->y(i);
    auto &eIn = inputWS->e(i);
    // Copy the energy transfer axis
    outputWS->setSharedX(i, inputWS->sharedX(i));
    for (unsigned int j = 0; j < size; ++j) {
      const double deltaE = xIn[j];
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
    g_log.information() << "Ef <= 0 or Ei <= 0 in at least one spectrum!!!!\n";
  if ((negativeEnergyWarning) && (efixedProp == EMPTY_DBL()))
    g_log.information() << "Try to set fixed energy\n";
  this->setProperty("OutputWorkspace", outputWS);
}

/**
 * Execute CorrectKiKf for event workspaces
 *
 */

void CorrectKiKf::execEvent() {
  g_log.information("Processing event workspace");

  const MatrixWorkspace_const_sptr matrixInputWS =
      getProperty("InputWorkspace");
  auto inputWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(matrixInputWS);

  // generate the output workspace pointer
  API::MatrixWorkspace_sptr matrixOutputWS = getProperty("OutputWorkspace");
  if (matrixOutputWS != matrixInputWS) {
    matrixOutputWS = matrixInputWS->clone();
    setProperty("OutputWorkspace", matrixOutputWS);
  }
  auto outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);

  const std::string emodeStr = getProperty("EMode");
  double efixedProp = getProperty("EFixed"), efixed;

  if (efixedProp == EMPTY_DBL()) {
    if (emodeStr == "Direct") {
      // Check if it has been store on the run object for this workspace
      if (inputWS->run().hasProperty("Ei")) {
        efixedProp = inputWS->run().getPropertyValueAsType<double>("Ei");
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
  const auto &spectrumInfo = inputWS->spectrumInfo();
  API::Progress prog = API::Progress(this, 0.0, 1.0, numHistograms);
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
  for (int64_t i = 0; i < numHistograms; ++i) {
    PARALLEL_START_INTERUPT_REGION

    double Efi = 0;
    // Now get the detector object for this histogram to check if monitor
    // or to get Ef for indirect geometry
    if (emodeStr == "Indirect") {
      if (efixedProp != EMPTY_DBL()) {
        Efi = efixedProp;
        // If a DetectorGroup is present should provide a value as a property
        // instead
      } else if (spectrumInfo.hasUniqueDetector(i)) {
        getEfixedFromParameterMap(Efi, i, spectrumInfo, pmap);
      } else {
        g_log.information()
            << "Workspace Index " << i << ": cannot find detector"
            << "\n";
      }
    }

    if (emodeStr == "Indirect")
      efixed = Efi;
    else
      efixed = efixedProp;

    // Do the correction
    auto &evlist = outputWS->getSpectrum(i);
    switch (evlist.getEventType()) {
    case TOF:
      // Switch to weights if needed.
      evlist.switchTo(WEIGHTED);
      /* no break */
      // Fall through

    case WEIGHTED:
      correctKiKfEventHelper(evlist.getWeightedEvents(), efixed, emodeStr);
      break;

    case WEIGHTED_NOTIME:
      correctKiKfEventHelper(evlist.getWeightedEventsNoTime(), efixed,
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
                               outputWS->getNumberEvents()
                        << " events, out of " << inputWS->getNumberEvents()
                        << '\n';
    if (efixedProp == EMPTY_DBL())
      g_log.information() << "Try to set fixed energy\n";
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

void CorrectKiKf::getEfixedFromParameterMap(double &Efi, int64_t i,
                                            const SpectrumInfo &spectrumInfo,
                                            const ParameterMap &pmap) {
  Efi = 0;

  if (spectrumInfo.isMonitor(i))
    return;

  const auto &det = spectrumInfo.detector(i);
  Parameter_sptr par = pmap.getRecursive(&det, "Efixed");
  if (par) {
    Efi = par->value<double>();
    g_log.debug() << "Detector: " << det.getID() << " EFixed: " << Efi << "\n";
  }
}

} // namespace Algorithms
} // namespace Mantid
