#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidAlgorithms/RemoveLowResTOF.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/UnitFactory.h"
#include <limits>
#include <map>
#include <math.h>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;
using DataObjects::EventWorkspace;
using Kernel::Exception::InstrumentDefinitionError;
using Kernel::Exception::NotFoundError;
using std::size_t;
using std::string;

DECLARE_ALGORITHM(RemoveLowResTOF)

/// Default constructor
RemoveLowResTOF::RemoveLowResTOF() : m_progress(NULL) {}

/// Destructor
RemoveLowResTOF::~RemoveLowResTOF() { delete m_progress; }

/// Algorithm's name for identification overriding a virtual method
const string RemoveLowResTOF::name() const { return "RemoveLowResTOF"; }

/// Algorithm's version for identification overriding a virtual method
int RemoveLowResTOF::version() const { return 1; }

/// Algorithm's category for identification overriding a virtual method
const string RemoveLowResTOF::category() const { return "Diffraction"; }

void RemoveLowResTOF::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  wsValidator->add<HistogramValidator>();
  wsValidator->add<RawCountValidator>();
  wsValidator->add<InstrumentValidator>();
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                             Direction::Input, wsValidator),
      "A workspace with x values in units of TOF and y values in counts");
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                             Direction::Output),
      "The name of the workspace to be created as the output of the algorithm");
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>(
          "LowResTOFWorkspace", "", Direction::Output, PropertyMode::Optional),
      "The name of the optional output workspace that contains low resolution "
      "TOF which are removed "
      "from input workspace.");

  auto validator = boost::make_shared<BoundedValidator<double>>();
  validator->setLower(0.01);
  declareProperty("ReferenceDIFC", Mantid::EMPTY_DBL(), validator,
                  "The DIFC value for the reference");

  declareProperty("K", 3.22, validator, "Some arbitrary number whose default "
                                        "is 3.22 for reasons that I don't "
                                        "understand");

  declareProperty("Tmin", Mantid::EMPTY_DBL(), validator,
                  "The minimum time-of-flight of the frame (in microseconds). "
                  "If not set the data range will be used.");
  declareProperty("MinWavelength", Mantid::EMPTY_DBL(), validator,
                  "The minimum wavelength for measurement. This overides all "
                  "other parameters if specified.");

  // hide things when people cjoose the minimum wavelength
  setPropertySettings("ReferenceDIFC",
                      new EnabledWhenProperty("MinWavelength", IS_DEFAULT));
  setPropertySettings("K",
                      new EnabledWhenProperty("MinWavelength", IS_DEFAULT));
  setPropertySettings("Tmin",
                      new EnabledWhenProperty("MinWavelength", IS_DEFAULT));
}

void RemoveLowResTOF::exec() {
  // Get the input workspace
  m_inputWS = this->getProperty("InputWorkspace");

  // without the primary flight path the algorithm cannot work
  try {
    m_instrument = m_inputWS->getInstrument();
    m_sample = m_instrument->getSample();
    m_L1 = m_instrument->getSource()->getDistance(*m_sample);
  } catch (NotFoundError &) {
    throw InstrumentDefinitionError(
        "Unable to calculate source-sample distance", m_inputWS->getTitle());
  }

  m_DIFCref = this->getProperty("ReferenceDIFC");
  m_K = this->getProperty("K");
  m_wavelengthMin = this->getProperty("MinWavelength");

  m_numberOfSpectra = m_inputWS->getNumberHistograms();

  std::string lowreswsname = getPropertyValue("LowResTOFWorkspace");
  if (lowreswsname.size() > 0)
    m_outputLowResTOF = true;
  else
    m_outputLowResTOF = false;

  // go off and do the event version if appropriate
  m_inputEvWS = boost::dynamic_pointer_cast<const EventWorkspace>(m_inputWS);
  if (m_inputEvWS != NULL) {
    this->execEvent();
    return;
  }

  // set up the progress bar
  m_progress = new Progress(this, 0.0, 1.0, m_numberOfSpectra);
  size_t xSize = m_inputWS->dataX(0).size();

  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != m_inputWS) {
    outputWS = WorkspaceFactory::Instance().create(m_inputWS, m_numberOfSpectra,
                                                   xSize, xSize - 1);
    setProperty("OutputWorkspace", outputWS);
  }

  this->getTminData(false);

  for (size_t workspaceIndex = 0; workspaceIndex < m_numberOfSpectra;
       workspaceIndex++) {
    // copy the data from the input workspace
    outputWS->dataX(workspaceIndex) = m_inputWS->readX(workspaceIndex);
    outputWS->dataY(workspaceIndex) = m_inputWS->readY(workspaceIndex);
    outputWS->dataE(workspaceIndex) = m_inputWS->readE(workspaceIndex);

    // calculate where to zero out to
    double tofMin = this->calcTofMin(workspaceIndex);
    const MantidVec &X = m_inputWS->readX(0);
    MantidVec::const_iterator last =
        std::lower_bound(X.begin(), X.end(), tofMin);
    if (last == X.end())
      --last;
    size_t endBin = last - X.begin();

    // flatten out the data
    for (size_t i = 0; i < endBin; i++) {
      outputWS->maskBin(workspaceIndex, i);
    }
    m_progress->report();
  }

  this->runMaskDetectors();
}

/** Remove low resolution TOF from an EventWorkspace
  */
void RemoveLowResTOF::execEvent() {
  // set up the output workspace
  MatrixWorkspace_sptr matrixOutW = this->getProperty("OutputWorkspace");
  DataObjects::EventWorkspace_sptr outW;
  if (matrixOutW == m_inputWS)
    outW = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutW);
  else {
    outW = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace",
                                                 m_numberOfSpectra, 2, 1));
    // Copy required stuff from it
    API::WorkspaceFactory::Instance().initializeFromParent(m_inputWS, outW,
                                                           false);
    outW->copyDataFrom((*m_inputEvWS));

    // cast to the matrixoutput workspace and save it
    matrixOutW = boost::dynamic_pointer_cast<MatrixWorkspace>(outW);
    this->setProperty("OutputWorkspace", matrixOutW);
  }

  MatrixWorkspace_sptr matrixLowResW = getProperty("LowResTOFWorkspace");
  DataObjects::EventWorkspace_sptr lowW;
  if (m_outputLowResTOF) {
    // Duplicate input workspace to output workspace
    lowW = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace",
                                                 m_numberOfSpectra, 2, 1));
    API::WorkspaceFactory::Instance().initializeFromParent(m_inputWS, lowW,
                                                           false);
    lowW->copyDataFrom((*m_inputEvWS));

    matrixLowResW = boost::dynamic_pointer_cast<MatrixWorkspace>(lowW);
    setProperty("LowResTOFWorkspace", matrixLowResW);
  }

  g_log.debug() << "TOF range was " << m_inputEvWS->getTofMin() << " to "
                << m_inputEvWS->getTofMax() << " microseconds\n";

  std::size_t numEventsOrig = outW->getNumberEvents();
  // set up the progress bar
  m_progress = new Progress(this, 0.0, 1.0, m_numberOfSpectra * 2);

  // algorithm assumes the data is sorted so it can jump out early
  outW->sortAll(Mantid::DataObjects::TOF_SORT, m_progress);

  this->getTminData(true);
  size_t numClearedEventLists = 0;
  size_t numClearedEvents = 0;

  // do the actual work
  for (size_t workspaceIndex = 0; workspaceIndex < m_numberOfSpectra;
       workspaceIndex++) {
    if (outW->getEventList(workspaceIndex).getNumberEvents() > 0) {
      double tmin = this->calcTofMin(workspaceIndex);
      if (tmin != tmin) {
        // Problematic
        g_log.warning() << "tmin for workspaceIndex " << workspaceIndex
                        << " is nan. Clearing out data. "
                        << "There are "
                        << outW->getEventList(workspaceIndex).getNumberEvents()
                        << " of it. \n";
        numClearedEventLists += 1;
        numClearedEvents +=
            outW->getEventList(workspaceIndex).getNumberEvents();
        outW->getEventList(workspaceIndex).clear(false);

        if (m_outputLowResTOF)
          lowW->getEventList(workspaceIndex).clear(false);
      } else if (tmin > 0.) {
        // there might be events between 0 and tmin (i.e., low resolution)
        outW->getEventList(workspaceIndex).maskTof(0., tmin);
        if (outW->getEventList(workspaceIndex).getNumberEvents() == 0)
          numClearedEventLists += 1;

        if (m_outputLowResTOF) {
          double tmax = lowW->getEventList(workspaceIndex).getTofMax();
          if (tmax != tmax) {
            g_log.warning() << "tmax for workspaceIndex " << workspaceIndex
                            << " is nan. Clearing out data. \n";
            lowW->getEventList(workspaceIndex).clear(false);
          } else {
            // There is possibility that tmin calculated is larger than TOF-MAX
            // of the spectrum
            if (tmax + DBL_MIN > tmin)
              lowW->getEventList(workspaceIndex).maskTof(tmin, tmax + DBL_MIN);
          }
        }
      } else {
        // do nothing if tmin <= 0. for outW
        if (m_outputLowResTOF) {
          // tmin = 0.  no event will be in low resolution
          lowW->getEventList(workspaceIndex).clear(false);
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
  this->runMaskDetectors();
}

double RemoveLowResTOF::calcTofMin(const std::size_t workspaceIndex) {
  const Kernel::V3D &sourcePos = m_instrument->getSource()->getPos();
  const Kernel::V3D &samplePos = m_sample->getPos();
  const Kernel::V3D &beamline = samplePos - sourcePos;
  double beamline_norm = 2. * beamline.norm();

  // Get a vector of detector IDs
  std::vector<detid_t> detNumbers;
  const std::set<detid_t> &detSet =
      m_inputWS->getSpectrum(workspaceIndex)->getDetectorIDs();
  detNumbers.assign(detSet.begin(), detSet.end());

  double tmin = 0.;
  if (isEmpty(m_wavelengthMin)) {
    std::map<detid_t, double> offsets; // just an empty offsets map
    double dspmap =
        Instrument::calcConversion(m_L1, beamline, beamline_norm, samplePos,
                                   m_instrument, detNumbers, offsets);

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
    double l2 = 0;
    for (std::set<detid_t>::const_iterator it = detSet.begin();
         it != detSet.end(); ++it) {
      l2 += m_instrument->getDetector(*it)->getDistance(*m_sample);
    }
    l2 /= static_cast<double>(detSet.size());

    Kernel::Unit_sptr wavelength = UnitFactory::Instance().create("Wavelength");
    // unfortunately there isn't a good way to convert a single value
    std::vector<double> X(1), temp(1);
    X[0] = m_wavelengthMin;
    wavelength->toTOF(X, temp, m_L1, l2, 0., 0, 0., 0.);
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

void RemoveLowResTOF::runMaskDetectors() {
  IAlgorithm_sptr alg = createChildAlgorithm("MaskDetectors");
  alg->setProperty<MatrixWorkspace_sptr>("Workspace",
                                         this->getProperty("OutputWorkspace"));
  alg->setProperty<MatrixWorkspace_sptr>("MaskedWorkspace",
                                         this->getProperty("InputWorkspace"));
  if (!alg->execute())
    throw std::runtime_error(
        "MaskDetectors Child Algorithm has not executed successfully");
}

} // namespace Algorithm
} // namespace Mantid
