// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SmoothNeighbours.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceNearestNeighbourInfo.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

using VecProperties = std::vector<Mantid::Kernel::Property *>;
using ConstVecProperties = const VecProperties;

namespace Mantid::Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SmoothNeighbours)

namespace {
// Used in custom GUI. Make sure you change them in SmoothNeighboursDialog.cpp
// as well.
const std::string NON_UNIFORM_GROUP = "NonUniform Detectors";
const std::string RECTANGULAR_GROUP = "Rectangular Detectors";
const std::string INPUT_WORKSPACE = "InputWorkspace";

/// Helper struct to run a lambda on exit out of the scope the
/// object is defined in
struct CallOnExit {
  using Callable = std::function<void()>;
  CallOnExit(Callable &&callable) noexcept : m_callable(std::move(callable)) {}
  ~CallOnExit() {
    try {
      m_callable();
    } catch (...) {
    };
  }

private:
  Callable m_callable;
};
} // namespace

SmoothNeighbours::SmoothNeighbours() : API::Algorithm(), m_weightedSum(std::make_unique<NullWeighting>()) {}

void SmoothNeighbours::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(INPUT_WORKSPACE, "", Direction::Input,
                                                                       std::make_shared<InstrumentValidator>()),
                  "The workspace containing the spectra to be averaged.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of "
                  "the algorithm.");

  // Unsigned double
  auto mustBePositiveDouble = std::make_shared<BoundedValidator<double>>();
  mustBePositiveDouble->setLower(0.0);

  // Unsigned int.
  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);

  std::vector<std::string> propOptions{"Flat", "Linear", "Parabolic", "Gaussian"};
  declareProperty("WeightedSum", "Flat", std::make_shared<StringListValidator>(propOptions),
                  "What sort of Weighting scheme to use?\n"
                  "  Flat: Effectively no-weighting, all weights are 1.\n"
                  "  Linear: Linear weighting 1 - r/R from origin.\n"
                  "  Parabolic : Weighting as cutoff - x + cutoff - y + 1."
                  "  Gaussian : Uses the absolute distance x^2 + y^2 ... "
                  "normalised by the cutoff^2");

  declareProperty("Sigma", 0.5, mustBePositiveDouble, "Sigma value for gaussian weighting schemes. Defaults to 0.5. ");
  setPropertySettings("Sigma", std::make_unique<EnabledWhenProperty>("WeightedSum", IS_EQUAL_TO, "Gaussian"));

  declareProperty("IgnoreMaskedDetectors", true, "If true, do not consider masked detectors in the NN search.");

  declareProperty("PreserveEvents", true,
                  "If the InputWorkspace is an "
                  "EventWorkspace, this will preserve "
                  "the full event list (warning: this "
                  "will use much more memory!).");

  // -- Rectangular properties --

  declareProperty("AdjX", 1, mustBePositive,
                  "The number of X (horizontal) adjacent pixels to average together. "
                  "Only for instruments with RectangularDetectors. ");

  declareProperty("AdjY", 1, mustBePositive,
                  "The number of Y (vertical) adjacent pixels to average together. "
                  "Only for instruments with RectangularDetectors. ");

  declareProperty("SumPixelsX", 1, mustBePositive,
                  "The total number of X (horizontal) adjacent pixels to sum together. "
                  "Only for instruments with RectangularDetectors.  AdjX will be ignored "
                  "if SumPixelsX > 1.");

  declareProperty("SumPixelsY", 1, mustBePositive,
                  "The total number of Y (vertical) adjacent pixels to sum together. "
                  "Only for instruments with RectangularDetectors. AdjY will be ignored if "
                  "SumPixelsY > 1");

  declareProperty("ZeroEdgePixels", 0, mustBePositive,
                  "The number of pixels to zero at edges. "
                  "Only for instruments with RectangularDetectors. ");

  setPropertyGroup("AdjX", RECTANGULAR_GROUP);
  setPropertyGroup("AdjY", RECTANGULAR_GROUP);
  setPropertyGroup("SumPixelsX", RECTANGULAR_GROUP);
  setPropertyGroup("SumPixelsY", RECTANGULAR_GROUP);
  setPropertyGroup("ZeroEdgePixels", RECTANGULAR_GROUP);

  // -- Non-uniform properties --

  std::vector<std::string> radiusPropOptions{"Meters", "NumberOfPixels"};
  declareProperty("RadiusUnits", "Meters", std::make_shared<StringListValidator>(radiusPropOptions),
                  "Units used to specify the radius.\n"
                  "  Meters : Radius is in meters.\n"
                  "  NumberOfPixels : Radius is in terms of the number of pixels.");

  declareProperty("Radius", 0.0, mustBePositiveDouble,
                  "The radius cut-off around a pixel to look for nearest neighbours to "
                  "average. \n"
                  "This radius cut-off is applied to a set of nearest neighbours whose "
                  "number is "
                  "defined in the NumberOfNeighbours property. See below for more details. "
                  "\n"
                  "If 0, will use the AdjX and AdjY parameters for rectangular detectors "
                  "instead.");

  declareProperty("NumberOfNeighbours", 8, mustBePositive,
                  "Number of nearest neighbouring pixels.\n"
                  "The default is 8.");

  declareProperty("SumNumberOfNeighbours", 1,
                  "Sum nearest neighbouring pixels with same parent.\n"
                  "Number of pixels will be reduced. The default is false.");

  declareProperty("ExpandSumAllPixels", false,
                  "OuputWorkspace will have same number of pixels as "
                  "InputWorkspace using SumPixelsX and SumPixelsY.  Individual "
                  "pixels will have averages.");

  setPropertyGroup("RadiusUnits", NON_UNIFORM_GROUP);
  setPropertyGroup("Radius", NON_UNIFORM_GROUP);
  setPropertyGroup("NumberOfNeighbours", NON_UNIFORM_GROUP);
  setPropertyGroup("SumNumberOfNeighbours", NON_UNIFORM_GROUP);
}

//--------------------------------------------------------------------------------------------
/** Fill the neighbours list given the AdjX AdjY parameters and an
 * instrument with rectangular detectors.
 */
void SmoothNeighbours::findNeighboursRectangular() {
  g_log.debug("SmoothNeighbours processing assuming rectangular detectors.");

  m_progress->resetNumSteps(m_inWS->getNumberHistograms(), 0.2, 0.5);

  Instrument_const_sptr inst = m_inWS->getInstrument();

  // To get the workspace index from the detector ID
  const detid2index_map pixel_to_wi = m_inWS->getDetectorIDToWorkspaceIndexMap(true, true);

  Progress prog(this, 0.0, 1.0, inst->nelements());

  // Build a list of Rectangular Detectors
  std::vector<std::shared_ptr<RectangularDetector>> detList;
  for (int i = 0; i < inst->nelements(); i++) {
    std::shared_ptr<RectangularDetector> det;
    std::shared_ptr<ICompAssembly> assem;
    std::shared_ptr<ICompAssembly> assem2;

    det = std::dynamic_pointer_cast<RectangularDetector>((*inst)[i]);
    if (det) {
      detList.emplace_back(det);
    } else {
      // Also, look in the first sub-level for RectangularDetectors (e.g. PG3).
      // We are not doing a full recursive search since that will be very long
      // for lots of pixels.
      assem = std::dynamic_pointer_cast<ICompAssembly>((*inst)[i]);
      if (assem) {
        for (int j = 0; j < assem->nelements(); j++) {
          det = std::dynamic_pointer_cast<RectangularDetector>((*assem)[j]);
          if (det) {
            detList.emplace_back(det);

          } else {
            // Also, look in the second sub-level for RectangularDetectors (e.g.
            // PG3).
            // We are not doing a full recursive search since that will be very
            // long for lots of pixels.
            assem2 = std::dynamic_pointer_cast<ICompAssembly>((*assem)[j]);
            if (assem2) {
              for (int k = 0; k < assem2->nelements(); k++) {
                det = std::dynamic_pointer_cast<RectangularDetector>((*assem2)[k]);
                if (det) {
                  detList.emplace_back(det);
                }
              }
            }
          }
        }
      }
    }
  }

  if (detList.empty()) {
    // Not rectangular so use Nearest Neighbours
    m_radius = translateToMeters("NumberOfPixels", std::max(m_adjX, m_adjY));
    setWeightingStrategy("Flat", m_radius);
    m_nNeighbours = m_adjX * m_adjY - 1;
    findNeighboursUbiquitous();
    return;
  }

  // Resize the vector we are setting
  m_neighbours.resize(m_inWS->getNumberHistograms());
  int startX = -m_adjX;
  int startY = -m_adjY;
  int endX = m_adjX;
  int endY = m_adjY;
  const int sumX = getProperty("SumPixelsX");
  const int sumY = getProperty("SumPixelsY");
  bool sum = sumX * sumY > 1;
  if (sum) {
    startX = 0;
    startY = 0;
    endX = sumX - 1;
    endY = sumY - 1;
  }

  m_outWI = 0;
  // Build a map to sort by the detectorID
  std::vector<std::pair<int, int>> idToIndexMap;
  idToIndexMap.reserve(detList.size());
  for (int i = 0; i < static_cast<int>(detList.size()); i++)
    idToIndexMap.emplace_back(detList[i]->getAtXY(0, 0)->getID(), i);

  // To sort in descending order
  if (sum)
    stable_sort(idToIndexMap.begin(), idToIndexMap.end());

  // Loop through the RectangularDetector's we listed before.
  for (const auto &idIndex : idToIndexMap) {
    std::shared_ptr<RectangularDetector> det = detList[idIndex.second];
    const std::string det_name = det->getName();
    for (int j = 0; j < det->xpixels(); j += sumX) {
      for (int k = 0; k < det->ypixels(); k += sumY) {
        double totalWeight = 0;
        // Neighbours and weights
        std::vector<weightedNeighbour> neighbours;

        for (int ix = startX; ix <= endX; ix++)
          for (int iy = startY; iy <= endY; iy++) {
            // Weights for corners=1; higher for center and adjacent pixels
            double smweight = m_weightedSum->weightAt(m_adjX, ix, m_adjY, iy);

            // Find the pixel ID at that XY position on the rectangular
            // detector
            if (j + ix >= det->xpixels() - m_edge || j + ix < m_edge)
              continue;
            if (k + iy >= det->ypixels() - m_edge || k + iy < m_edge)
              continue;
            int pixelID = det->getAtXY(j + ix, k + iy)->getID();

            // Find the corresponding workspace index, if any
            auto mapEntry = pixel_to_wi.find(pixelID);
            if (mapEntry != pixel_to_wi.end()) {
              size_t wi = mapEntry->second;
              neighbours.emplace_back(wi, smweight);
              // Count the total weight
              totalWeight += smweight;
            }
          }

        // Adjust the weights of each neighbour to normalize to unity
        if (!sum || m_expandSumAllPixels)
          for (auto &neighbour : neighbours)
            neighbour.second /= totalWeight;

        // Save the list of neighbours for this output workspace index.
        m_neighbours[m_outWI] = neighbours;
        m_outWI++;

        m_progress->report("Finding Neighbours");
      }
    }
    prog.report(det_name);
  }
}

//--------------------------------------------------------------------------------------------
/** Use NearestNeighbours to find the neighbours for any instrument
 */
void SmoothNeighbours::findNeighboursUbiquitous() {
  g_log.debug("SmoothNeighbours processing NOT assuming rectangular detectors.");

  m_progress->resetNumSteps(m_inWS->getNumberHistograms(), 0.2, 0.5);
  this->progress(0.2, "Building Neighbour Map");

  const spec2index_map spec2index = m_inWS->getSpectrumToWorkspaceIndexMap();

  // Resize the vector we are setting
  m_neighbours.resize(m_inWS->getNumberHistograms());

  bool ignoreMaskedDetectors = getProperty("IgnoreMaskedDetectors");
  WorkspaceNearestNeighbourInfo neighbourInfo(*m_inWS, ignoreMaskedDetectors, m_nNeighbours);

  // Cull by radius
  RadiusFilter radiusFilter(m_radius);

  // Go through every input workspace pixel
  m_outWI = 0;
  int sum = getProperty("SumNumberOfNeighbours");
  std::shared_ptr<const Geometry::IComponent> parent, neighbParent, grandparent, neighbGParent;
  std::vector<bool> used(m_inWS->getNumberHistograms(), false);
  const auto &detectorInfo = m_inWS->detectorInfo();
  for (size_t wi = 0; wi < m_inWS->getNumberHistograms(); wi++) {
    if (sum > 1)
      if (used[wi])
        continue;
    // We want to skip monitors
    try {
      // Get the list of detectors in this pixel
      const auto &dets = m_inWS->getSpectrum(wi).getDetectorIDs();
      const auto index = detectorInfo.indexOf(*dets.begin());
      if (detectorInfo.isMonitor(index))
        continue; // skip monitor
      if (detectorInfo.isMasked(index)) {
        // Calibration masks many detectors, but there should be 0s after
        // smoothing
        if (sum == 1)
          m_outWI++;
        continue; // skip masked detectors
      }
      if (sum > 1) {
        const auto &det = detectorInfo.detector(index);
        parent = det.getParent();
        if (parent)
          grandparent = parent->getParent();
      }
    } catch (Kernel::Exception::NotFoundError &) {
      continue; // skip missing detector
    }

    specnum_t inSpec = m_inWS->getSpectrum(wi).getSpectrumNo();

    // Step one - Get the number of specified neighbours
    SpectraDistanceMap insideGrid = neighbourInfo.getNeighboursExact(inSpec);

    // Step two - Filter the results by the radius cut off.
    SpectraDistanceMap neighbSpectra = radiusFilter.apply(insideGrid);

    // Force the central pixel to always be there
    // There seems to be a bug in nearestNeighbours, returns distance != 0.0 for
    // the central pixel. So we force distance = 0
    neighbSpectra[inSpec] = V3D(0.0, 0.0, 0.0);

    // Neighbours and weights list
    double totalWeight = 0;
    int noNeigh = 0;
    std::vector<weightedNeighbour> neighbours;

    // Convert from spectrum numbers to workspace indices
    for (auto &specDistance : neighbSpectra) {
      specnum_t spec = specDistance.first;

      // Use the weighting strategy to calculate the weight.
      double weight = m_weightedSum->weightAt(specDistance.second);

      if (weight > 0) {
        // Find the corresponding workspace index
        auto mapIt = spec2index.find(spec);
        if (mapIt != spec2index.end()) {
          size_t neighWI = mapIt->second;
          if (sum > 1) {
            // Get the list of detectors in this pixel
            const std::set<detid_t> &dets = m_inWS->getSpectrum(neighWI).getDetectorIDs();
            const auto &det = detectorInfo.detector(*dets.begin());
            neighbParent = det.getParent();
            neighbGParent = neighbParent->getParent();
            if (noNeigh >= sum || neighbParent->getName() != parent->getName() ||
                neighbGParent->getName() != grandparent->getName() || used[neighWI])
              continue;
            noNeigh++;
            used[neighWI] = true;
          }
          neighbours.emplace_back(neighWI, weight);
          totalWeight += weight;
        }
      }
    }

    // Adjust the weights of each neighbour to normalize to unity
    if (sum == 1)
      for (auto &neighbour : neighbours)
        neighbour.second /= totalWeight;

    // Save the list of neighbours for this output workspace index.
    m_neighbours[m_outWI] = neighbours;
    m_outWI++;

    m_progress->report("Finding Neighbours");
  } // each workspace index
}

/**
Attempts to reset the Weight based on the strategyName provided. Note that if
these conditional statements fail to override the existing m_weightedSum member,
it should stay as a NullWeighting, which will throw during usage.
@param strategyName : The name of the weighting strategy to use
@param cutOff : The cutoff distance
*/
void SmoothNeighbours::setWeightingStrategy(const std::string &strategyName, double &cutOff) {
  if (strategyName == "Flat") {
    m_weightedSum = std::make_unique<FlatWeighting>();
  } else if (strategyName == "Linear") {
    m_weightedSum = std::make_unique<LinearWeighting>(cutOff);
  } else if (strategyName == "Parabolic") {
    m_weightedSum = std::make_unique<ParabolicWeighting>(cutOff);
  } else if (strategyName == "Gaussian") {
    m_weightedSum = std::make_unique<GaussianWeightingnD>(cutOff, getProperty("Sigma"));
  }
  g_log.information() << "Smoothing with " << strategyName << " Weighting\n";
}

/**
Translate the radius into meters.
@param radiusUnits : The name of the radius units
@param enteredRadius : The numerical value of the radius in whatever units have
been specified
*/
double SmoothNeighbours::translateToMeters(const std::string &radiusUnits, const double &enteredRadius) const {
  double translatedRadius = 0;
  if (radiusUnits == "Meters") {
    // Nothing more to do.
    translatedRadius = enteredRadius;
  } else if (radiusUnits == "NumberOfPixels") {
    // Get the first idetector from the workspace index 0.
    const auto &firstDet = m_inWS->spectrumInfo().detector(0);
    // Find the bounding box of that detector
    BoundingBox bbox;
    firstDet.getBoundingBox(bbox);
    // Multiply (meters/pixels) by number of pixels, note that enteredRadius
    // takes on meaning of the number of pixels.
    translatedRadius = bbox.width().norm() * enteredRadius;
  } else {
    const std::string message = "SmoothNeighbours::translateToMeters, Unknown Unit: " + radiusUnits;
    throw std::invalid_argument(message);
  }
  return translatedRadius;
}

/** Executes the algorithm
 *
 */
void SmoothNeighbours::exec() {
  m_inWS = getProperty("InputWorkspace");

  m_preserveEvents = getProperty("PreserveEvents");
  m_expandSumAllPixels = getProperty("ExpandSumAllPixels");

  // Use the unit type to translate the entered radius into meters.
  m_radius = translateToMeters(getProperty("RadiusUnits"), getProperty("Radius"));

  setWeightingStrategy(getProperty("WeightedSum"), m_radius);

  m_adjX = getProperty("AdjX");
  m_adjY = getProperty("AdjY");
  m_edge = getProperty("ZeroEdgePixels");

  m_nNeighbours = getProperty("NumberOfNeighbours");

  // Progress reporting, first for the sorting
  m_progress = std::make_unique<Progress>(this, 0.0, 0.2, m_inWS->getNumberHistograms());

  // Clean up when we are done
  CallOnExit resetInWSOnExit([this]() { m_inWS.reset(); });
  CallOnExit resetNeighboursOnExit([this]() {
    m_neighbours.clear();
    m_neighbours.shrink_to_fit();
  });

  // Run the appropriate method depending on the type of the instrument
  if (m_inWS->getInstrument()->containsRectDetectors() == Instrument::ContainsState::Full)
    findNeighboursRectangular();
  else
    findNeighboursUbiquitous();

  auto wsEvent = std::dynamic_pointer_cast<EventWorkspace>(m_inWS);
  if (wsEvent)
    wsEvent->sortAll(TOF_SORT, m_progress.get());
  if (wsEvent && m_preserveEvents)
    this->execEvent(wsEvent);
  else
    this->execWorkspace2D();
}

//--------------------------------------------------------------------------------------------
/** Execute the algorithm for a Workspace2D/don't preserve events input */
void SmoothNeighbours::execWorkspace2D() {
  m_progress->resetNumSteps(m_inWS->getNumberHistograms(), 0.5, 1.0);

  // Get some stuff from the input workspace
  const size_t numberOfSpectra = m_outWI;

  const size_t YLength = m_inWS->blocksize();

  MatrixWorkspace_sptr outWS;
  // Make a brand new Workspace2D
  if (std::dynamic_pointer_cast<OffsetsWorkspace>(m_inWS)) {
    g_log.information() << "Creating new OffsetsWorkspace\n";
    outWS = MatrixWorkspace_sptr(new OffsetsWorkspace(m_inWS->getInstrument()));
  } else {
    outWS = std::dynamic_pointer_cast<MatrixWorkspace>(
        API::WorkspaceFactory::Instance().create("Workspace2D", numberOfSpectra, YLength + 1, YLength));
  }
  this->setProperty("OutputWorkspace", outWS);

  setupNewInstrument(*outWS);

  // Go through all the output workspace
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inWS, *outWS))
  for (int outWIi = 0; outWIi < int(numberOfSpectra); outWIi++) {
    PARALLEL_START_INTERRUPT_REGION

    auto &outSpec = outWS->getSpectrum(outWIi);
    // Reset the Y and E vectors
    outSpec.clearData();
    auto &outY = outSpec.mutableY();
    // We will temporarily carry the squared error
    auto &outE = outSpec.mutableE();
    // tmp to carry the X Data.
    auto &outX = outSpec.mutableX();

    // Which are the neighbours?
    std::vector<weightedNeighbour> &neighbours = m_neighbours[outWIi];
    std::vector<weightedNeighbour>::iterator it;
    for (it = neighbours.begin(); it != neighbours.end(); ++it) {
      size_t inWI = it->first;
      double weight = it->second;
      double weightSquared = weight * weight;

      const auto &inSpec = m_inWS->getSpectrum(inWI);
      const auto &inY = inSpec.y();
      const auto &inE = inSpec.e();
      const auto &inX = inSpec.x();

      for (size_t i = 0; i < YLength; i++) {
        // Add the weighted signal
        outY[i] += inY[i] * weight;
        // Square the error, scale by weight (which you have to square too),
        // then add in quadrature
        double errorSquared = inE[i];
        errorSquared *= errorSquared;
        errorSquared *= weightSquared;
        outE[i] += errorSquared;
        // Copy the X values as well
        outX[i] = inX[i];
      }
      if (m_inWS->isHistogramData()) {
        outX[YLength] = inX[YLength];
      }
    } //(each neighbour)

    // Now un-square the error, since we summed it in quadrature
    for (size_t i = 0; i < YLength; i++)
      outE[i] = sqrt(outE[i]);

    m_progress->report("Summing");
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  if (m_expandSumAllPixels)
    spreadPixels(outWS);
}

//--------------------------------------------------------------------------------------------
/** Build the instrument/detector setup in workspace
 */
void SmoothNeighbours::setupNewInstrument(MatrixWorkspace &outWS) const {
  // Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(*m_inWS, outWS, false);

  // Go through all the output workspace
  size_t numberOfSpectra = outWS.getNumberHistograms();

  for (int outWIi = 0; outWIi < int(numberOfSpectra); outWIi++) {
    auto &outSpec = outWS.getSpectrum(outWIi);

    // Reset detectors
    outSpec.clearDetectorIDs();

    // Which are the neighbours?
    for (const auto &neighbor : m_neighbours[outWIi]) {
      const auto &inSpec = m_inWS->getSpectrum(neighbor.first);
      outSpec.addDetectorIDs(inSpec.getDetectorIDs());
    }
  }
}
//--------------------------------------------------------------------------------------------
/** Spread the average over all the pixels
 */
void SmoothNeighbours::spreadPixels(const MatrixWorkspace_sptr &outWS) {
  // Get some stuff from the input workspace
  const size_t numberOfSpectra = m_inWS->getNumberHistograms();

  const size_t YLength = m_inWS->blocksize();

  MatrixWorkspace_sptr outws2;
  // Make a brand new Workspace2D
  if (std::dynamic_pointer_cast<OffsetsWorkspace>(m_inWS)) {
    g_log.information() << "Creating new OffsetsWorkspace\n";
    outws2 = std::make_shared<OffsetsWorkspace>(m_inWS->getInstrument());
  } else {
    outws2 = std::dynamic_pointer_cast<MatrixWorkspace>(
        API::WorkspaceFactory::Instance().create("Workspace2D", numberOfSpectra, YLength + 1, YLength));
  }

  // Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(*m_inWS, *outws2, false);
  // Go through all the input workspace
  for (int outWIi = 0; outWIi < int(numberOfSpectra); outWIi++) {
    const auto &inSpec = m_inWS->getSpectrum(outWIi);
    auto &outSpec2 = outws2->getSpectrum(outWIi);
    outSpec2.mutableX() = inSpec.x();
    outSpec2.addDetectorIDs(inSpec.getDetectorIDs());
    // Zero the Y and E vectors
    outSpec2.clearData();
  }

  // Go through all the output workspace
  const size_t numberOfSpectra2 = outWS->getNumberHistograms();
  for (int outWIi = 0; outWIi < int(numberOfSpectra2); outWIi++) {

    // Which are the neighbours?
    for (const auto &neighbor : m_neighbours[outWIi]) {
      outws2->setHistogram(neighbor.first, outWS->histogram(outWIi));
    }
  }
  this->setProperty("OutputWorkspace", outws2);
}
//--------------------------------------------------------------------------------------------
/** Execute the algorithm for a EventWorkspace input
 * @param ws :: EventWorkspace
 */
void SmoothNeighbours::execEvent(Mantid::DataObjects::EventWorkspace_sptr &ws) {
  m_progress->resetNumSteps(m_inWS->getNumberHistograms(), 0.5, 1.0);

  // Get some stuff from the input workspace
  const size_t numberOfSpectra = m_outWI;
  const auto YLength = static_cast<int>(m_inWS->blocksize());

  EventWorkspace_sptr outWS;
  // Make a brand new EventWorkspace
  outWS = std::dynamic_pointer_cast<EventWorkspace>(
      API::WorkspaceFactory::Instance().create("EventWorkspace", numberOfSpectra, YLength + 1, YLength));
  // Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(*ws, *outWS, false);
  // Ensure thread-safety
  outWS->sortAll(TOF_SORT, nullptr);

  this->setProperty("OutputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(outWS));

  // Go through all the output workspace
  PARALLEL_FOR_IF(Kernel::threadSafe(*ws, *outWS))
  for (int outWIi = 0; outWIi < int(numberOfSpectra); outWIi++) {
    PARALLEL_START_INTERRUPT_REGION

    // Create the output event list (empty)
    EventList &outEL = outWS->getSpectrum(outWIi);

    // Which are the neighbours?
    std::vector<weightedNeighbour> &neighbours = m_neighbours[outWIi];
    std::vector<weightedNeighbour>::iterator it;
    for (it = neighbours.begin(); it != neighbours.end(); ++it) {
      size_t inWI = it->first;
      // if(sum)outEL.copyInfoFrom(*ws->getSpectrum(inWI));
      double weight = it->second;
      // Copy the event list
      EventList tmpEL = ws->getSpectrum(inWI);
      // Scale it
      tmpEL *= weight;
      // Add it
      outEL += tmpEL;
    }

    m_progress->report("Summing");
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // Give the 0-th X bins to all the output spectra.
  outWS->setAllX(m_inWS->binEdges(0));
  if (m_expandSumAllPixels)
    spreadPixels(outWS);
}

} // namespace Mantid::Algorithms
