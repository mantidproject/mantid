// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
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
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"

#include <boost/algorithm/string.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

using VecProperties = std::vector<Mantid::Kernel::Property *>;
using ConstVecProperties = const VecProperties;

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SmoothNeighbours)

// Used in custom GUI. Make sure you change them in SmoothNeighboursDialog.cpp
// as well.
const std::string SmoothNeighbours::NON_UNIFORM_GROUP = "NonUniform Detectors";
const std::string SmoothNeighbours::RECTANGULAR_GROUP = "Rectangular Detectors";
const std::string SmoothNeighbours::INPUT_WORKSPACE = "InputWorkspace";

SmoothNeighbours::SmoothNeighbours()
    : API::Algorithm(), AdjX(0), AdjY(0), Edge(0), Radius(0.), nNeighbours(0),
      WeightedSum(new NullWeighting), PreserveEvents(false),
      expandSumAllPixels(false), outWI(0), inWS(), m_neighbours(),
      m_progress(nullptr) {}

/** Initialisation method.
 *
 */
void SmoothNeighbours::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      INPUT_WORKSPACE, "", Direction::Input,
                      boost::make_shared<InstrumentValidator>()),
                  "The workspace containing the spectra to be averaged.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of "
                  "the algorithm.");

  // Unsigned double
  auto mustBePositiveDouble = boost::make_shared<BoundedValidator<double>>();
  mustBePositiveDouble->setLower(0.0);

  // Unsigned int.
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);

  std::vector<std::string> propOptions{"Flat", "Linear", "Parabolic",
                                       "Gaussian"};
  declareProperty("WeightedSum", "Flat",
                  boost::make_shared<StringListValidator>(propOptions),
                  "What sort of Weighting scheme to use?\n"
                  "  Flat: Effectively no-weighting, all weights are 1.\n"
                  "  Linear: Linear weighting 1 - r/R from origin.\n"
                  "  Parabolic : Weighting as cutoff - x + cutoff - y + 1."
                  "  Gaussian : Uses the absolute distance x^2 + y^2 ... "
                  "normalised by the cutoff^2");

  declareProperty(
      "Sigma", 0.5, mustBePositiveDouble,
      "Sigma value for gaussian weighting schemes. Defaults to 0.5. ");
  setPropertySettings("Sigma", std::make_unique<EnabledWhenProperty>(
                                   "WeightedSum", IS_EQUAL_TO, "Gaussian"));

  declareProperty(
      "IgnoreMaskedDetectors", true,
      "If true, do not consider masked detectors in the NN search.");

  declareProperty("PreserveEvents", true,
                  "If the InputWorkspace is an "
                  "EventWorkspace, this will preserve "
                  "the full event list (warning: this "
                  "will use much more memory!).");

  // -- Rectangular properties
  // ----------------------------------------------------------------------

  declareProperty(
      "AdjX", 1, mustBePositive,
      "The number of X (horizontal) adjacent pixels to average together. "
      "Only for instruments with RectangularDetectors. ");

  declareProperty(
      "AdjY", 1, mustBePositive,
      "The number of Y (vertical) adjacent pixels to average together. "
      "Only for instruments with RectangularDetectors. ");

  declareProperty(
      "SumPixelsX", 1, mustBePositive,
      "The total number of X (horizontal) adjacent pixels to sum together. "
      "Only for instruments with RectangularDetectors.  AdjX will be ignored "
      "if SumPixelsX > 1.");

  declareProperty(
      "SumPixelsY", 1, mustBePositive,
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

  // -- Non-uniform properties
  // ----------------------------------------------------------------------

  std::vector<std::string> radiusPropOptions{"Meters", "NumberOfPixels"};
  declareProperty(
      "RadiusUnits", "Meters",
      boost::make_shared<StringListValidator>(radiusPropOptions),
      "Units used to specify the radius.\n"
      "  Meters : Radius is in meters.\n"
      "  NumberOfPixels : Radius is in terms of the number of pixels.");

  declareProperty(
      "Radius", 0.0, mustBePositiveDouble,
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

  m_progress->resetNumSteps(inWS->getNumberHistograms(), 0.2, 0.5);

  Instrument_const_sptr inst = inWS->getInstrument();

  // To get the workspace index from the detector ID
  const detid2index_map pixel_to_wi =
      inWS->getDetectorIDToWorkspaceIndexMap(true);

  // std::cout << " inst->nelements() " << inst->nelements() << "\n";
  Progress prog(this, 0.0, 1.0, inst->nelements());

  // Build a list of Rectangular Detectors
  std::vector<boost::shared_ptr<RectangularDetector>> detList;
  for (int i = 0; i < inst->nelements(); i++) {
    boost::shared_ptr<RectangularDetector> det;
    boost::shared_ptr<ICompAssembly> assem;
    boost::shared_ptr<ICompAssembly> assem2;

    det = boost::dynamic_pointer_cast<RectangularDetector>((*inst)[i]);
    if (det) {
      detList.push_back(det);
    } else {
      // Also, look in the first sub-level for RectangularDetectors (e.g. PG3).
      // We are not doing a full recursive search since that will be very long
      // for lots of pixels.
      assem = boost::dynamic_pointer_cast<ICompAssembly>((*inst)[i]);
      if (assem) {
        for (int j = 0; j < assem->nelements(); j++) {
          det = boost::dynamic_pointer_cast<RectangularDetector>((*assem)[j]);
          if (det) {
            detList.push_back(det);

          } else {
            // Also, look in the second sub-level for RectangularDetectors (e.g.
            // PG3).
            // We are not doing a full recursive search since that will be very
            // long for lots of pixels.
            assem2 = boost::dynamic_pointer_cast<ICompAssembly>((*assem)[j]);
            if (assem2) {
              for (int k = 0; k < assem2->nelements(); k++) {
                det = boost::dynamic_pointer_cast<RectangularDetector>(
                    (*assem2)[k]);
                if (det) {
                  detList.push_back(det);
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
    Radius = translateToMeters("NumberOfPixels", std::max(AdjX, AdjY));
    setWeightingStrategy("Flat", Radius);
    nNeighbours = AdjX * AdjY - 1;
    findNeighboursUbiqutious();
  }

  // Resize the vector we are setting
  m_neighbours.resize(inWS->getNumberHistograms());
  int StartX = -AdjX;
  int StartY = -AdjY;
  int EndX = AdjX;
  int EndY = AdjY;
  int SumX = getProperty("SumPixelsX");
  int SumY = getProperty("SumPixelsY");
  bool sum = SumX * SumY > 1;
  if (sum) {
    StartX = 0;
    StartY = 0;
    EndX = SumX - 1;
    EndY = SumY - 1;
  }

  outWI = 0;
  // Build a map to sort by the detectorID
  std::vector<std::pair<int, int>> v1;
  for (int i = 0; i < static_cast<int>(detList.size()); i++)
    v1.emplace_back(detList[i]->getAtXY(0, 0)->getID(), i);

  // To sort in descending order
  if (sum)
    stable_sort(v1.begin(), v1.end());

  std::vector<std::pair<int, int>>::iterator Iter1;

  // Loop through the RectangularDetector's we listed before.
  for (Iter1 = v1.begin(); Iter1 != v1.end(); ++Iter1) {
    int i = (*Iter1).second;
    boost::shared_ptr<RectangularDetector> det = detList[i];
    std::string det_name = det->getName();
    if (det) {
      for (int j = 0; j < det->xpixels(); j += SumX) {
        for (int k = 0; k < det->ypixels(); k += SumY) {
          double totalWeight = 0;
          // Neighbours and weights
          std::vector<weightedNeighbour> neighbours;

          for (int ix = StartX; ix <= EndX; ix++)
            for (int iy = StartY; iy <= EndY; iy++) {
              // Weights for corners=1; higher for center and adjacent pixels
              double smweight = WeightedSum->weightAt(AdjX, ix, AdjY, iy);

              // Find the pixel ID at that XY position on the rectangular
              // detector
              if (j + ix >= det->xpixels() - Edge || j + ix < Edge)
                continue;
              if (k + iy >= det->ypixels() - Edge || k + iy < Edge)
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
          if (!sum || expandSumAllPixels)
            for (auto &neighbour : neighbours)
              neighbour.second /= totalWeight;

          // Save the list of neighbours for this output workspace index.
          m_neighbours[outWI] = neighbours;
          outWI++;

          m_progress->report("Finding Neighbours");
        }
      }
    }
    prog.report(det_name);
  }
}

//--------------------------------------------------------------------------------------------
/** Use NearestNeighbours to find the neighbours for any instrument
 */
void SmoothNeighbours::findNeighboursUbiqutious() {
  g_log.debug(
      "SmoothNeighbours processing NOT assuming rectangular detectors.");

  m_progress->resetNumSteps(inWS->getNumberHistograms(), 0.2, 0.5);
  this->progress(0.2, "Building Neighbour Map");

  Instrument_const_sptr inst = inWS->getInstrument();
  const spec2index_map spec2index = inWS->getSpectrumToWorkspaceIndexMap();

  // Resize the vector we are setting
  m_neighbours.resize(inWS->getNumberHistograms());

  bool ignoreMaskedDetectors = getProperty("IgnoreMaskedDetectors");
  WorkspaceNearestNeighbourInfo neighbourInfo(*inWS, ignoreMaskedDetectors,
                                              nNeighbours);

  // Cull by radius
  RadiusFilter radiusFilter(Radius);

  // Go through every input workspace pixel
  outWI = 0;
  int sum = getProperty("SumNumberOfNeighbours");
  boost::shared_ptr<const Geometry::IComponent> parent, neighbParent,
      grandparent, neighbGParent;
  auto used = new bool[inWS->getNumberHistograms()];
  if (sum > 1) {
    for (size_t wi = 0; wi < inWS->getNumberHistograms(); wi++)
      used[wi] = false;
  }
  const auto &detectorInfo = inWS->detectorInfo();
  for (size_t wi = 0; wi < inWS->getNumberHistograms(); wi++) {
    if (sum > 1)
      if (used[wi])
        continue;
    // We want to skip monitors
    try {
      // Get the list of detectors in this pixel
      const auto &dets = inWS->getSpectrum(wi).getDetectorIDs();
      const auto index = detectorInfo.indexOf(*dets.begin());
      if (detectorInfo.isMonitor(index))
        continue; // skip monitor
      if (detectorInfo.isMasked(index)) {
        // Calibration masks many detectors, but there should be 0s after
        // smoothing
        if (sum == 1)
          outWI++;
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

    specnum_t inSpec = inWS->getSpectrum(wi).getSpectrumNo();

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
      double weight = WeightedSum->weightAt(specDistance.second);

      if (weight > 0) {
        // Find the corresponding workspace index
        auto mapIt = spec2index.find(spec);
        if (mapIt != spec2index.end()) {
          size_t neighWI = mapIt->second;
          if (sum > 1) {
            // Get the list of detectors in this pixel
            const std::set<detid_t> &dets =
                inWS->getSpectrum(neighWI).getDetectorIDs();
            const auto &det = detectorInfo.detector(*dets.begin());
            neighbParent = det.getParent();
            neighbGParent = neighbParent->getParent();
            if (noNeigh >= sum ||
                neighbParent->getName() != parent->getName() ||
                neighbGParent->getName() != grandparent->getName() ||
                used[neighWI])
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
    m_neighbours[outWI] = neighbours;
    outWI++;

    m_progress->report("Finding Neighbours");
  } // each workspace index

  delete[] used;
}

/**
Attempts to reset the Weight based on the strategyName provided. Note that if
these conditional statements fail to override the existing WeightedSum member,
it should stay as a NullWeighting, which will throw during usage.
@param strategyName : The name of the weighting strategy to use
@param cutOff : The cutoff distance
*/
void SmoothNeighbours::setWeightingStrategy(const std::string &strategyName,
                                            double &cutOff) {
  if (strategyName == "Flat") {
    boost::scoped_ptr<WeightingStrategy> flatStrategy(new FlatWeighting);
    WeightedSum.swap(flatStrategy);
    g_log.information("Smoothing with Flat Weighting");
  } else if (strategyName == "Linear") {
    boost::scoped_ptr<WeightingStrategy> linearStrategy(
        new LinearWeighting(cutOff));
    WeightedSum.swap(linearStrategy);
    g_log.information("Smoothing with Linear Weighting");
  } else if (strategyName == "Parabolic") {
    boost::scoped_ptr<WeightingStrategy> parabolicStrategy(
        new ParabolicWeighting(cutOff));
    WeightedSum.swap(parabolicStrategy);
    g_log.information("Smoothing with Parabolic Weighting");
  } else if (strategyName == "Gaussian") {
    double sigma = getProperty("Sigma");
    boost::scoped_ptr<WeightingStrategy> gaussian1DStrategy(
        new GaussianWeightingnD(cutOff, sigma));
    WeightedSum.swap(gaussian1DStrategy);
    g_log.information("Smoothing with Gaussian Weighting");
  }
}

/**
Translate the radius into meters.
@param radiusUnits : The name of the radius units
@param enteredRadius : The numerical value of the radius in whatever units have
been specified
*/
double SmoothNeighbours::translateToMeters(const std::string &radiusUnits,
                                           const double &enteredRadius) const {
  double translatedRadius = 0;
  if (radiusUnits == "Meters") {
    // Nothing more to do.
    translatedRadius = enteredRadius;
  } else if (radiusUnits == "NumberOfPixels") {
    // Get the first idetector from the workspace index 0.
    const auto &firstDet = inWS->spectrumInfo().detector(0);
    // Find the bounding box of that detector
    BoundingBox bbox;
    firstDet.getBoundingBox(bbox);
    // Multiply (meters/pixels) by number of pixels, note that enteredRadius
    // takes on meaning of the number of pixels.
    translatedRadius = bbox.width().norm() * enteredRadius;
  } else {
    const std::string message =
        "SmoothNeighbours::translateToMeters, Unknown Unit: " + radiusUnits;
    throw std::invalid_argument(message);
  }
  return translatedRadius;
}

/**
Check whether the properties provided are all in their default state.
@param properties : Vector of mantid property pointers
@return True only if they are all default, otherwise False.
*/
bool areAllDefault(ConstVecProperties &properties) {
  for (auto property : properties) {
    if (!property->isDefault()) {
      return false;
    }
  }
  return true;
}

//--------------------------------------------------------------------------------------------
/** Executes the algorithm
 *
 */
void SmoothNeighbours::exec() {
  inWS = getProperty("InputWorkspace");

  PreserveEvents = getProperty("PreserveEvents");

  expandSumAllPixels = getProperty("ExpandSumAllPixels");

  // Use the unit type to translate the entered radius into meters.
  Radius = translateToMeters(getProperty("RadiusUnits"), getProperty("Radius"));

  setWeightingStrategy(getProperty("WeightedSum"), Radius);

  AdjX = getProperty("AdjX");
  AdjY = getProperty("AdjY");
  Edge = getProperty("ZeroEdgePixels");

  nNeighbours = getProperty("NumberOfNeighbours");

  // Progress reporting, first for the sorting
  m_progress =
      std::make_unique<Progress>(this, 0.0, 0.2, inWS->getNumberHistograms());

  // Run the appropriate method depending on the type of the instrument
  if (inWS->getInstrument()->containsRectDetectors() ==
      Instrument::ContainsState::Full)
    findNeighboursRectangular();
  else
    findNeighboursUbiqutious();

  EventWorkspace_sptr wsEvent =
      boost::dynamic_pointer_cast<EventWorkspace>(inWS);
  if (wsEvent)
    wsEvent->sortAll(TOF_SORT, m_progress.get());

  if (!wsEvent || !PreserveEvents)
    this->execWorkspace2D();
  else if (wsEvent)
    this->execEvent(wsEvent);
  else
    throw std::runtime_error("This algorithm requires a Workspace2D or "
                             "EventWorkspace as its input.");
}

//--------------------------------------------------------------------------------------------
/** Execute the algorithm for a Workspace2D/don't preserve events input */
void SmoothNeighbours::execWorkspace2D() {
  m_progress->resetNumSteps(inWS->getNumberHistograms(), 0.5, 1.0);

  // Get some stuff from the input workspace
  const size_t numberOfSpectra = outWI;

  const size_t YLength = inWS->blocksize();

  MatrixWorkspace_sptr outWS;
  // Make a brand new Workspace2D
  if (boost::dynamic_pointer_cast<OffsetsWorkspace>(inWS)) {
    g_log.information() << "Creating new OffsetsWorkspace\n";
    outWS = MatrixWorkspace_sptr(new OffsetsWorkspace(inWS->getInstrument()));
  } else {
    outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        API::WorkspaceFactory::Instance().create("Workspace2D", numberOfSpectra,
                                                 YLength + 1, YLength));
  }
  this->setProperty("OutputWorkspace", outWS);

  setupNewInstrument(*outWS);

  // Copy geometry over.
  // API::WorkspaceFactory::Instance().initializeFromParent(inWS, outWS, false);

  // Go through all the output workspace
  PARALLEL_FOR_IF(Kernel::threadSafe(*inWS, *outWS))
  for (int outWIi = 0; outWIi < int(numberOfSpectra); outWIi++) {
    PARALLEL_START_INTERUPT_REGION

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

      const auto &inSpec = inWS->getSpectrum(inWI);
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
      if (inWS->isHistogramData()) {
        outX[YLength] = inX[YLength];
      }
    } //(each neighbour)

    // Now un-square the error, since we summed it in quadrature
    for (size_t i = 0; i < YLength; i++)
      outE[i] = sqrt(outE[i]);

    // Copy the single detector ID (of the center) and spectrum number from the
    // input workspace
    // outSpec->copyInfoFrom(*inWS->getSpectrum(outWIi));

    m_progress->report("Summing");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  if (expandSumAllPixels)
    spreadPixels(outWS);
}

//--------------------------------------------------------------------------------------------
/** Build the instrument/detector setup in workspace
 */
void SmoothNeighbours::setupNewInstrument(MatrixWorkspace &outws) const {
  // Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(*inWS, outws, false);

  // Go through all the output workspace
  size_t numberOfSpectra = outws.getNumberHistograms();

  for (int outWIi = 0; outWIi < int(numberOfSpectra); outWIi++) {
    auto &outSpec = outws.getSpectrum(outWIi);

    // Reset detectors
    outSpec.clearDetectorIDs();

    // Which are the neighbours?
    for (const auto &neighbor : m_neighbours[outWIi]) {
      const auto &inSpec = inWS->getSpectrum(neighbor.first);
      outSpec.addDetectorIDs(inSpec.getDetectorIDs());
    }
  }
}
//--------------------------------------------------------------------------------------------
/** Spread the average over all the pixels
 */
void SmoothNeighbours::spreadPixels(MatrixWorkspace_sptr outws) {
  // Get some stuff from the input workspace
  const size_t numberOfSpectra = inWS->getNumberHistograms();

  const size_t YLength = inWS->blocksize();

  MatrixWorkspace_sptr outws2;
  // Make a brand new Workspace2D
  if (boost::dynamic_pointer_cast<OffsetsWorkspace>(inWS)) {
    g_log.information() << "Creating new OffsetsWorkspace\n";
    outws2 = MatrixWorkspace_sptr(new OffsetsWorkspace(inWS->getInstrument()));
  } else {
    outws2 = boost::dynamic_pointer_cast<MatrixWorkspace>(
        API::WorkspaceFactory::Instance().create("Workspace2D", numberOfSpectra,
                                                 YLength + 1, YLength));
  }

  // Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(*inWS, *outws2, false);
  // Go through all the input workspace
  for (int outWIi = 0; outWIi < int(numberOfSpectra); outWIi++) {
    const auto &inSpec = inWS->getSpectrum(outWIi);
    auto &outSpec2 = outws2->getSpectrum(outWIi);
    outSpec2.mutableX() = inSpec.x();
    outSpec2.addDetectorIDs(inSpec.getDetectorIDs());
    // Zero the Y and E vectors
    outSpec2.clearData();
  }

  // Go through all the output workspace
  const size_t numberOfSpectra2 = outws->getNumberHistograms();
  for (int outWIi = 0; outWIi < int(numberOfSpectra2); outWIi++) {

    // Which are the neighbours?
    for (const auto &neighbor : m_neighbours[outWIi]) {
      outws2->setHistogram(neighbor.first, outws->histogram(outWIi));
    }
  }
  this->setProperty("OutputWorkspace", outws2);
}
//--------------------------------------------------------------------------------------------
/** Execute the algorithm for a EventWorkspace input
 * @param ws :: EventWorkspace
 */
void SmoothNeighbours::execEvent(Mantid::DataObjects::EventWorkspace_sptr &ws) {
  m_progress->resetNumSteps(inWS->getNumberHistograms(), 0.5, 1.0);

  // Get some stuff from the input workspace
  const size_t numberOfSpectra = outWI;
  const auto YLength = static_cast<int>(inWS->blocksize());

  EventWorkspace_sptr outWS;
  // Make a brand new EventWorkspace
  outWS = boost::dynamic_pointer_cast<EventWorkspace>(
      API::WorkspaceFactory::Instance().create(
          "EventWorkspace", numberOfSpectra, YLength + 1, YLength));
  // Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(*ws, *outWS, false);
  // Ensure thread-safety
  outWS->sortAll(TOF_SORT, nullptr);

  this->setProperty("OutputWorkspace",
                    boost::dynamic_pointer_cast<MatrixWorkspace>(outWS));

  // Go through all the output workspace
  PARALLEL_FOR_IF(Kernel::threadSafe(*ws, *outWS))
  for (int outWIi = 0; outWIi < int(numberOfSpectra); outWIi++) {
    PARALLEL_START_INTERUPT_REGION

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

    // Copy the single detector ID (of the center) and spectrum number from the
    // input workspace
    // if (!sum) outEL.copyInfoFrom(*ws->getSpectrum(outWIi));

    m_progress->report("Summing");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Give the 0-th X bins to all the output spectra.
  outWS->setAllX(inWS->binEdges(0));
  if (expandSumAllPixels)
    spreadPixels(outWS);
}

} // namespace Algorithms
} // namespace Mantid
