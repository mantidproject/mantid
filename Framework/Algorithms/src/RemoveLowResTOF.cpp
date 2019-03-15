// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RemoveLowResTOF.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/RawCountValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include <cmath>
#include <limits>
#include <map>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;
using DataObjects::EventWorkspace;
using std::size_t;
using std::string;

DECLARE_ALGORITHM(RemoveLowResTOF)

/// Default constructor
RemoveLowResTOF::RemoveLowResTOF()
    : m_inputWS(), m_inputEvWS(), m_DIFCref(0.), m_K(0.), m_Tmin(0.),
      m_wavelengthMin(0.), m_numberOfSpectra(0), m_outputLowResTOF(false) {}

/// Algorithm's name for identification overriding a virtual method
const string RemoveLowResTOF::name() const { return "RemoveLowResTOF"; }

/// Algorithm's version for identification overriding a virtual method
int RemoveLowResTOF::version() const { return 1; }

/// Algorithm's category for identification overriding a virtual method
const string RemoveLowResTOF::category() const {
  return "Diffraction\\Corrections";
}

void RemoveLowResTOF::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  wsValidator->add<HistogramValidator>();
  wsValidator->add<RawCountValidator>();
  wsValidator->add<InstrumentValidator>();
  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace", "", Direction::Input, wsValidator),
      "A workspace with x values in units of TOF and y values in counts");
  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "",
                                                      Direction::Output),
      "The name of the workspace to be created as the output of the algorithm");
  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "LowResTOFWorkspace", "", Direction::Output, PropertyMode::Optional),
      "The name of the optional output workspace that contains low resolution "
      "TOF which are removed "
      "from input workspace.");

  auto validator = boost::make_shared<BoundedValidator<double>>();
  validator->setLower(0.01);
  declareProperty("ReferenceDIFC", Mantid::EMPTY_DBL(), validator,
                  "The DIFC value for the reference");

  declareProperty("K", 3.22, validator,
                  "Some arbitrary number whose default "
                  "is 3.22 for reasons that I don't "
                  "understand");

  declareProperty("Tmin", Mantid::EMPTY_DBL(), validator,
                  "The minimum time-of-flight of the frame (in microseconds). "
                  "If not set the data range will be used.");
  declareProperty("MinWavelength", Mantid::EMPTY_DBL(), validator,
                  "The minimum wavelength for measurement. This overides all "
                  "other parameters if specified.");

  // hide things when people cjoose the minimum wavelength
  setPropertySettings("ReferenceDIFC", make_unique<EnabledWhenProperty>(
                                           "MinWavelength", IS_DEFAULT));
  setPropertySettings(
      "K", make_unique<EnabledWhenProperty>("MinWavelength", IS_DEFAULT));
  setPropertySettings(
      "Tmin", make_unique<EnabledWhenProperty>("MinWavelength", IS_DEFAULT));
}

void RemoveLowResTOF::exec() {
  // Get the input workspace
  m_inputWS = this->getProperty("InputWorkspace");
  const auto &spectrumInfo = m_inputWS->spectrumInfo();

  m_DIFCref = this->getProperty("ReferenceDIFC");
  m_K = this->getProperty("K");
  m_wavelengthMin = this->getProperty("MinWavelength");

  m_numberOfSpectra = m_inputWS->getNumberHistograms();

  std::string lowreswsname = getPropertyValue("LowResTOFWorkspace");
  if (!lowreswsname.empty())
    m_outputLowResTOF = true;
  else
    m_outputLowResTOF = false;

  // Only create the output workspace if it's different to the input one
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != m_inputWS) {
    outputWS = m_inputWS->clone();
    setProperty("OutputWorkspace", outputWS);
  }

  // go off and do the event version if appropriate
  m_inputEvWS = boost::dynamic_pointer_cast<const EventWorkspace>(m_inputWS);
  if (m_inputEvWS != nullptr) {
    this->execEvent(spectrumInfo);
    return;
  }

  // set up the progress bar
  m_progress = make_unique<Progress>(this, 0.0, 1.0, m_numberOfSpectra);

  this->getTminData(false);

  for (size_t workspaceIndex = 0; workspaceIndex < m_numberOfSpectra;
       workspaceIndex++) {
    // calculate where to zero out to
    const double tofMin = this->calcTofMin(workspaceIndex, spectrumInfo);
    const auto &X = m_inputWS->x(0);
    auto last = std::lower_bound(X.cbegin(), X.cend(), tofMin);
    if (last == X.end())
      --last;
    const size_t endBin = last - X.begin();

    // flatten out the data
    for (size_t i = 0; i < endBin; i++) {
      outputWS->maskBin(workspaceIndex, i);
    }
    m_progress->report();
  }
}

/** Remove low resolution TOF from an EventWorkspace
 */
void RemoveLowResTOF::execEvent(const SpectrumInfo &spectrumInfo) {
  // set up the output workspace
  MatrixWorkspace_sptr matrixOutW = getProperty("OutputWorkspace");
  auto outW = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutW);

  MatrixWorkspace_sptr matrixLowResW = getProperty("LowResTOFWorkspace");
  if (m_outputLowResTOF) {
    matrixLowResW = m_inputWS->clone();
    setProperty("LowResTOFWorkspace", matrixLowResW);
  }
  auto lowW = boost::dynamic_pointer_cast<EventWorkspace>(matrixLowResW);

  g_log.debug() << "TOF range was " << m_inputEvWS->getTofMin() << " to "
                << m_inputEvWS->getTofMax() << " microseconds\n";

  std::size_t numEventsOrig = outW->getNumberEvents();
  // set up the progress bar
  m_progress = make_unique<Progress>(this, 0.0, 1.0, m_numberOfSpectra * 2);

  // algorithm assumes the data is sorted so it can jump out early
  outW->sortAll(Mantid::DataObjects::TOF_SORT, m_progress.get());

  this->getTminData(true);
  size_t numClearedEventLists = 0;
  size_t numClearedEvents = 0;

  // do the actual work
  for (size_t workspaceIndex = 0; workspaceIndex < m_numberOfSpectra;
       workspaceIndex++) {
    if (outW->getSpectrum(workspaceIndex).getNumberEvents() > 0) {
      double tmin = this->calcTofMin(workspaceIndex, spectrumInfo);
      if (tmin != tmin) {
        // Problematic
        g_log.warning() << "tmin for workspaceIndex " << workspaceIndex
                        << " is nan. Clearing out data. "
                        << "There are "
                        << outW->getSpectrum(workspaceIndex).getNumberEvents()
                        << " of it. \n";
        numClearedEventLists += 1;
        numClearedEvents += outW->getSpectrum(workspaceIndex).getNumberEvents();
        outW->getSpectrum(workspaceIndex).clear(false);

        if (m_outputLowResTOF)
          lowW->getSpectrum(workspaceIndex).clear(false);
      } else if (tmin > 0.) {
        // there might be events between 0 and tmin (i.e., low resolution)
        outW->getSpectrum(workspaceIndex).maskTof(0., tmin);
        if (outW->getSpectrum(workspaceIndex).getNumberEvents() == 0)
          numClearedEventLists += 1;

        if (m_outputLowResTOF) {
          double tmax = lowW->getSpectrum(workspaceIndex).getTofMax();
          if (tmax != tmax) {
            g_log.warning() << "tmax for workspaceIndex " << workspaceIndex
                            << " is nan. Clearing out data. \n";
            lowW->getSpectrum(workspaceIndex).clear(false);
          } else {
            // There is possibility that tmin calculated is larger than TOF-MAX
            // of the spectrum
            if (tmax + DBL_MIN > tmin)
              lowW->getSpectrum(workspaceIndex).maskTof(tmin, tmax + DBL_MIN);
          }
        }
      } else {
        // do nothing if tmin <= 0. for outW
        if (m_outputLowResTOF) {
          // tmin = 0.  no event will be in low resolution
          lowW->getSpectrum(workspaceIndex).clear(false);
        }
      } //
    }
  }
  g_log.information() << "Went from " << numEventsOrig << " events to "
                      << outW->getNumberEvents() << " events ("
                      << (static_cast<double>(numEventsOrig -
                                              outW->getNumberEvents()) *
                          100. / static_cast<double>(numEventsOrig))
                      << "% removed)\n";
  if (numClearedEventLists > 0)
    g_log.warning()
        << numClearedEventLists << " spectra of " << m_numberOfSpectra
        << " had all data removed.  The number of removed events is "
        << numClearedEvents << ".\n";
  g_log.debug() << "TOF range is now " << outW->getTofMin() << " to "
                << outW->getTofMax() << " microseconds\n";
  outW->clearMRU();
}

double RemoveLowResTOF::calcTofMin(const std::size_t workspaceIndex,
                                   const SpectrumInfo &spectrumInfo) {

  const double l1 = spectrumInfo.l1();

  // Get a vector of detector IDs
  std::vector<detid_t> detNumbers;
  const auto &detSet = m_inputWS->getSpectrum(workspaceIndex).getDetectorIDs();
  detNumbers.assign(detSet.begin(), detSet.end());

  double tmin = 0.;
  if (isEmpty(m_wavelengthMin)) {
    std::map<detid_t, double> offsets; // just an empty offsets map
    double dspmap = Conversion::tofToDSpacingFactor(
        l1, spectrumInfo.l2(workspaceIndex),
        spectrumInfo.twoTheta(workspaceIndex), detNumbers, offsets);

    // this is related to the reference tof
    double sqrtdmin =
        sqrt(m_Tmin / m_DIFCref) + m_K * log10(dspmap * m_DIFCref);
    if (sqrtdmin <= 0.)
      return 0.;
    tmin = sqrtdmin * sqrtdmin / dspmap;
    if (tmin != tmin) {
      g_log.warning() << "tmin is nan because dspmap = " << dspmap << ".\n";
    }
  } else {
    const double l2 = spectrumInfo.l2(workspaceIndex);

    Kernel::Unit_sptr wavelength = UnitFactory::Instance().create("Wavelength");
    // unfortunately there isn't a good way to convert a single value
    std::vector<double> X(1), temp(1);
    X[0] = m_wavelengthMin;
    wavelength->toTOF(X, temp, l1, l2, 0., 0, 0., 0.);
    tmin = X[0];
  }

  g_log.debug() << "tmin[" << workspaceIndex << "] " << tmin << "\n";

  return tmin;
}

void RemoveLowResTOF::getTminData(const bool isEvent) {
  // get it from the properties
  double empty = Mantid::EMPTY_DBL();
  double temp = this->getProperty("Tmin");
  if (temp != empty) {
    m_Tmin = temp;
    return;
  }

  if (isEvent) {
    m_Tmin = m_inputEvWS->getEventXMin();
  } else {
    m_Tmin = m_inputWS->getXMin();
  }
  g_log.information() << "Tmin = " << m_Tmin << " microseconds\n";
  if (m_Tmin < 0.)
    throw std::runtime_error("Cannot have minimum time less than zero");
}

} // namespace Algorithms
} // namespace Mantid
