//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include <fstream>
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using Mantid::DataObjects::OffsetsWorkspace;

namespace Mantid {
namespace Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(AlignDetectors)

//-----------------------------------------------------------------------
/**
 * Make a map of the conversion factors between tof and D-spacing
 * for all pixel IDs in a workspace.
 * @param inputWS the workspace containing the instrument geometry of interest.
 * @param offsetsWS map between pixelID and offset (from the calibration file)
 * @return map of conversion factors between TOF and dSpacing
 */
std::map<detid_t, double> *AlignDetectors::calcTofToD_ConversionMap(
    Mantid::API::MatrixWorkspace_const_sptr inputWS,
    OffsetsWorkspace_sptr offsetsWS) {
  // Get a pointer to the instrument contained in the workspace
  Instrument_const_sptr instrument = inputWS->getInstrument();

  double l1;
  Kernel::V3D beamline, samplePos;
  double beamline_norm;

  instrument->getInstrumentParameters(l1, beamline, beamline_norm, samplePos);

  std::map<detid_t, double> *myMap = new std::map<detid_t, double>();

  // To get all the detector ID's
  detid2det_map allDetectors;
  instrument->getDetectors(allDetectors);

  // Now go through all
  detid2det_map::iterator it;
  for (it = allDetectors.begin(); it != allDetectors.end(); ++it) {
    detid_t detectorID = it->first;
    Geometry::IDetector_const_sptr det = it->second;

    // Find the offset, if any
    double offset = offsetsWS->getValue(detectorID, 0.0);
    if (offset <= -1.) { // non-physical
      std::stringstream msg;
      msg << "Encountered offset of " << offset
          << " which converts data to negative d-spacing for detectorID "
          << detectorID << "\n";
      throw std::logic_error(msg.str());
    }

    // Compute the factor
    double factor = Instrument::calcConversion(l1, beamline, beamline_norm,
                                               samplePos, det, offset);

    // Save in map
    (*myMap)[detectorID] = factor;
  }

  // Give back the map.
  return myMap;
}

//-----------------------------------------------------------------------
/** Compute a conversion factor for a LIST of detectors.
 * Averages out the conversion factors if there are several.
 *
 * @param tofToDmap :: detectord id to double conversions map
 * @param detectors :: list of detector IDS
 * @return
 */

double calcConversionFromMap(std::map<detid_t, double> *tofToDmap,
                             const std::set<detid_t> &detectors) {
  double factor = 0.;
  detid_t numDetectors = 0;
  for (std::set<detid_t>::const_iterator iter = detectors.begin();
       iter != detectors.end(); ++iter) {
    detid_t detectorID = *iter;
    std::map<detid_t, double>::iterator it;
    it = tofToDmap->find(detectorID);
    if (it != tofToDmap->end()) {
      factor += it->second; // The factor for that ID
      numDetectors++;
    }
  }
  if (numDetectors > 0)
    return factor / static_cast<double>(numDetectors);
  else
    return 0;
}

//========================================================================
//========================================================================
/// (Empty) Constructor
AlignDetectors::AlignDetectors() { this->tofToDmap = NULL; }

/// Destructor
AlignDetectors::~AlignDetectors() { delete this->tofToDmap; }

//-----------------------------------------------------------------------
void AlignDetectors::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  // Workspace unit must be TOF.
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  wsValidator->add<RawCountValidator>();
  wsValidator->add<InstrumentValidator>();

  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "A workspace with units of TOF");

  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");

  std::vector<std::string> exts;
  exts.push_back(".cal");
  declareProperty(
      new FileProperty("CalibrationFile", "", FileProperty::OptionalLoad, exts),
      "Optional: The .cal file containing the position correction factors. "
      "Either this or OffsetsWorkspace needs to be specified.");

  declareProperty(
      new WorkspaceProperty<OffsetsWorkspace>(
          "OffsetsWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional: A OffsetsWorkspace containing the calibration offsets. Either "
      "this or CalibrationFile needs to be specified.");
}

//-----------------------------------------------------------------------
/** Executes the algorithm
 *  @throw Exception::FileError If the calibration file cannot be opened and
 * read successfully
 *  @throw Exception::InstrumentDefinitionError If unable to obtain the
 * source-sample distance
 */
void AlignDetectors::exec() {
  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Read in the calibration data
  const std::string calFileName = getProperty("CalibrationFile");
  OffsetsWorkspace_sptr offsetsWS = getProperty("OffsetsWorkspace");

  progress(0.0, "Reading calibration file");
  if (offsetsWS && !calFileName.empty())
    throw std::invalid_argument("You must specify either CalibrationFile or "
                                "OffsetsWorkspace but not both.");
  if (!offsetsWS && calFileName.empty())
    throw std::invalid_argument(
        "You must specify either CalibrationFile or OffsetsWorkspace.");

  if (!calFileName.empty()) {
    // Load the .cal file
    IAlgorithm_sptr alg = createChildAlgorithm("LoadCalFile");
    alg->setPropertyValue("CalFilename", calFileName);
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty<bool>("MakeGroupingWorkspace", false);
    alg->setProperty<bool>("MakeOffsetsWorkspace", true);
    alg->setProperty<bool>("MakeMaskWorkspace", false);
    alg->setPropertyValue("WorkspaceName", "temp");
    alg->executeAsChildAlg();
    offsetsWS = alg->getProperty("OutputOffsetsWorkspace");
  }

  const int64_t numberOfSpectra = inputWS->getNumberHistograms();

  // generate map of the tof->d conversion factors
  this->tofToDmap = calcTofToD_ConversionMap(inputWS, offsetsWS);

  // Check if its an event workspace
  EventWorkspace_const_sptr eventW =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventW != NULL) {
    this->execEvent();
    return;
  }

  API::MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (outputWS != inputWS) {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    setProperty("OutputWorkspace", outputWS);
  }

  // Set the final unit that our output workspace will have
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("dSpacing");

  // Initialise the progress reporting object
  Progress progress(this, 0.0, 1.0, numberOfSpectra);

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR2(inputWS, outputWS)
  for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i) {
    PARALLEL_START_INTERUPT_REGION
    try {
      // Get the input spectrum number at this workspace index
      const ISpectrum *inSpec = inputWS->getSpectrum(size_t(i));
      const double factor =
          calcConversionFromMap(this->tofToDmap, inSpec->getDetectorIDs());

      // Get references to the x data
      MantidVec &xOut = outputWS->dataX(i);

      // Make sure reference to input X vector is obtained after output one
      // because in the case
      // where the input & output workspaces are the same, it might move if the
      // vectors were shared.
      const MantidVec &xIn = inSpec->readX();

      // std::transform( xIn.begin(), xIn.end(), xOut.begin(),
      // std::bind2nd(std::multiplies<double>(), factor) );
      // the above transform creates wrong output in parallel in debug in Visual
      // Studio
      for (size_t k = 0; k < xOut.size(); ++k) {
        xOut[k] = xIn[k] * factor;
      }

      // Copy the Y&E data
      outputWS->dataY(i) = inSpec->readY();
      outputWS->dataE(i) = inSpec->readE();

    } catch (Exception::NotFoundError &) {
      // Zero the data in this case
      outputWS->dataX(i).assign(outputWS->readX(i).size(), 0.0);
      outputWS->dataY(i).assign(outputWS->readY(i).size(), 0.0);
      outputWS->dataE(i).assign(outputWS->readE(i).size(), 0.0);
    }
    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

//-----------------------------------------------------------------------
/**
 * Execute the align detectors algorithm for an event workspace.
 */
void AlignDetectors::execEvent() {
  // g_log.information("Processing event workspace");

  // the calibration information is already read in at this point

  // convert the input workspace into the event workspace we already know it is
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

  // Set the final unit that our output workspace will have
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("dSpacing");

  const int64_t numberOfSpectra =
      static_cast<int64_t>(inputWS->getNumberHistograms());

  // Initialise the progress reporting object
  Progress progress(this, 0.0, 1.0, numberOfSpectra);

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Compute the conversion factor
    double factor = calcConversionFromMap(
        this->tofToDmap, inputWS->getSpectrum(size_t(i))->getDetectorIDs());

    // Perform the multiplication on all events
    outputWS->getEventList(i).convertTof(factor);

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  if (outputWS->getTofMin() < 0.) {
    std::stringstream msg;
    msg << "Something wrong with the calibration. Negative minimum d-spacing "
           "created. d_min = " << outputWS->getTofMin() << " d_max "
        << outputWS->getTofMax();
    throw std::runtime_error(msg.str());
  }
  outputWS->clearMRU();
}

} // namespace Algorithms
} // namespace Mantid
