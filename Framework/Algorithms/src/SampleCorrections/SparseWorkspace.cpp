// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SampleCorrections/SparseWorkspace.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidHistogramData/HistogramIterator.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>

namespace {
/** Check all detectors have the same EFixed value.
 *  @param eFixed An EFixedProvider object.
 *  @param detIDs A vector containing detector ids.
 *  @return True, if all EFixed values match, false otherwise.
 */
bool constantIndirectEFixed(const Mantid::API::ExperimentInfo &info,
                            const std::vector<Mantid::detid_t> &detIDs) {
  const auto e = info.getEFixed(detIDs[0]);
  for (size_t i = 1; i < detIDs.size(); ++i) {
    if (e != info.getEFixed(detIDs[i])) {
      return false;
    }
  }
  return true;
}
} // namespace

namespace Mantid {
namespace Algorithms {

SparseWorkspace::SparseWorkspace(const API::MatrixWorkspace &modelWS,
                                 const size_t wavelengthPoints,
                                 const size_t rows, const size_t columns)
    : Workspace2D() {
  double minLat, maxLat, minLong, maxLong;
  std::tie(minLat, maxLat, minLong, maxLong) = extremeAngles(modelWS);
  m_gridDef = std::make_unique<Algorithms::DetectorGridDefinition>(
      minLat, maxLat, rows, minLong, maxLong, columns);
  const size_t numSpectra = rows * columns;
  const auto h = modelHistogram(modelWS, wavelengthPoints);
  initialize(numSpectra, h);

  // Build a quite standard and somewhat complete instrument.
  auto instrument =
      std::make_shared<Geometry::Instrument>("MC_simulation_instrument");
  const auto refFrame = modelWS.getInstrument()->getReferenceFrame();

  instrument->setReferenceFrame(
      std::make_shared<Geometry::ReferenceFrame>(*refFrame));
  // The sparse instrument is build around origin.
  constexpr Kernel::V3D samplePos{0.0, 0.0, 0.0};
  auto sample =
      std::make_unique<Geometry::Component>("sample", instrument.get());
  sample->setPos(samplePos);
  instrument->add(sample.get());
  instrument->markAsSamplePos(sample.release());
  constexpr double R = 1.0; // This will be the default L2 distance.
  // Add source behind the sample.
  const Kernel::V3D sourcePos = [&]() {
    Kernel::V3D p;
    p[refFrame->pointingAlongBeam()] = -2.0 * R;
    return p;
  }();
  auto source = std::make_unique<Geometry::ObjComponent>("source", nullptr,
                                                         instrument.get());
  source->setPos(sourcePos);
  instrument->add(source.get());
  instrument->markAsSource(source.release());

  auto detShape = makeCubeShape();
  for (size_t col = 0; col < columns; ++col) {
    const auto lon = m_gridDef->longitudeAt(col);
    for (size_t row = 0; row < rows; ++row) {
      const auto lat = m_gridDef->latitudeAt(row);
      const size_t index = col * rows + row;
      const auto detID = static_cast<int>(index);
      std::ostringstream detName;
      detName << "det-" << detID;
      auto det = std::make_unique<Geometry::Detector>(
          detName.str(), detID, detShape, instrument.get());
      const Kernel::V3D pos = [&]() {
        Kernel::V3D p;
        p[refFrame->pointingHorizontal()] = R * std::sin(lon) * std::cos(lat);
        p[refFrame->pointingUp()] = R * std::sin(lat);
        p[refFrame->pointingAlongBeam()] = R * std::cos(lon) * std::cos(lat);
        return p;
      }();
      det->setPos(pos);
      getSpectrum(index).setDetectorID(detID);
      instrument->add(det.get());
      instrument->markAsDetector(det.release());
    }
  }
  setInstrument(instrument);

  // Copy things needed for the simulation from the model workspace.
  auto &paramMap = instrumentParameters();
  auto parametrizedInstrument = getInstrument();
  // Copy beam parameters.
  const auto modelSource = modelWS.getInstrument()->getSource();
  const auto beamWidthParam = modelSource->getNumberParameter("beam-width");
  const auto beamHeightParam = modelSource->getNumberParameter("beam-height");
  if (beamWidthParam.size() == 1 && beamHeightParam.size() == 1) {
    auto parametrizedSource = parametrizedInstrument->getSource();
    paramMap.add("double", parametrizedSource.get(), "beam-width",
                 beamWidthParam[0]);
    paramMap.add("double", parametrizedSource.get(), "beam-height",
                 beamHeightParam[0]);
  }
  // Add information about EFixed in a proper place.
  const auto eMode = modelWS.getEMode();
  mutableRun().addProperty("deltaE-mode", Kernel::DeltaEMode::asString(eMode));
  if (eMode == Kernel::DeltaEMode::Direct) {
    mutableRun().addProperty("Ei", modelWS.getEFixed());
  } else if (eMode == Kernel::DeltaEMode::Indirect) {
    const auto &detIDs = modelWS.detectorInfo().detectorIDs();
    if (!constantIndirectEFixed(modelWS, detIDs)) {
      throw std::runtime_error(
          "Sparse instrument with variable EFixed not supported.");
    }
    const auto e = modelWS.getEFixed(detIDs[0]);
    const auto &sparseDetIDs = detectorInfo().detectorIDs();
    for (int sparseDetID : sparseDetIDs) {
      setEFixed(sparseDetID, e);
    }
  }
}

/** Find the latitude and longitude intervals the detectors
 *  of the given workspace span as seen from the sample.
 *  Just do this for detectors that have a histogram in the ws
 *  @param ws A workspace.
 *  @return A tuple containing the latitude and longitude ranges.
 */
std::tuple<double, double, double, double>
SparseWorkspace::extremeAngles(const API::MatrixWorkspace &ws) {
  const auto &spectrumInfo = ws.spectrumInfo();
  const auto refFrame = ws.getInstrument()->getReferenceFrame();
  double minLat = std::numeric_limits<double>::max();
  double maxLat = std::numeric_limits<double>::lowest();
  double minLong = std::numeric_limits<double>::max();
  double maxLong = std::numeric_limits<double>::lowest();
  for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
    double lat, lon;
    std::tie(lat, lon) = spectrumInfo.geographicalAngles(i);
    if (lat < minLat) {
      minLat = lat;
    }
    if (lat > maxLat) {
      maxLat = lat;
    }
    if (lon < minLong) {
      minLong = lon;
    }
    if (lon > maxLong) {
      maxLong = lon;
    }
  }
  return std::make_tuple(minLat, maxLat, minLong, maxLong);
}

/** Find the maximum and minimum wavelength points over the entire workpace.
 *  @param ws A workspace to investigate.
 *  @return A tuple containing the wavelength range.
 */
std::tuple<double, double>
SparseWorkspace::extremeWavelengths(const API::MatrixWorkspace &ws) {
  double currentMin = std::numeric_limits<double>::max();
  double currentMax = std::numeric_limits<double>::lowest();
  for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
    const auto h = ws.histogram(i);
    const auto first = h.begin();
    currentMin = std::min(first->center(), currentMin);
    const auto last = std::prev(h.end());
    currentMax = std::max(last->center(), currentMax);
  }
  return std::make_tuple(currentMin, currentMax);
}

/** Create a template histogram for the sparse instrument workspace.
 *  @param modelWS A workspace the sparse instrument is approximating.
 *  @param wavelengthPoints Number of points in the output histogram.
 *  @return A template histogram.
 */
Mantid::HistogramData::Histogram
SparseWorkspace::modelHistogram(const API::MatrixWorkspace &modelWS,
                                const size_t wavelengthPoints) {
  double minWavelength, maxWavelength;
  std::tie(minWavelength, maxWavelength) = extremeWavelengths(modelWS);
  HistogramData::Frequencies ys(wavelengthPoints, 0.0);
  HistogramData::FrequencyVariances es(wavelengthPoints, 0.0);
  HistogramData::Points ps(wavelengthPoints, 0.0);
  HistogramData::Histogram h(ps, ys, es);
  auto &xs = h.mutableX();
  if (wavelengthPoints > 1) {
    const double step = (maxWavelength - minWavelength) /
                        static_cast<double>(wavelengthPoints - 1);
    for (size_t i = 0; i < xs.size() - 1; ++i) {
      xs[i] = minWavelength + step * static_cast<double>(i);
    }
    // Force last point as otherwise it might be slightly off due to
    // small rounding errors in the calculation above.
    xs.back() = maxWavelength;
  } else {
    xs.front() = (minWavelength + maxWavelength) / 2.0;
  }
  return h;
}

/** Creates a rectangular cuboid shape.
 *  @return A cube shape.
 */
Geometry::IObject_sptr SparseWorkspace::makeCubeShape() {
  using namespace Poco::XML;
  const double dimension = 0.05;
  AutoPtr<Document> shapeDescription = new Document;
  AutoPtr<Element> typeElement = shapeDescription->createElement("type");
  typeElement->setAttribute("name", "detector");
  AutoPtr<Element> shapeElement = shapeDescription->createElement("cuboid");
  shapeElement->setAttribute("id", "cube");
  const std::string posCoord = std::to_string(dimension / 2);
  const std::string negCoord = std::to_string(-dimension / 2);
  AutoPtr<Element> element =
      shapeDescription->createElement("left-front-bottom-point");
  element->setAttribute("x", negCoord);
  element->setAttribute("y", negCoord);
  element->setAttribute("z", posCoord);
  shapeElement->appendChild(element);
  element = shapeDescription->createElement("left-front-top-point");
  element->setAttribute("x", negCoord);
  element->setAttribute("y", posCoord);
  element->setAttribute("z", posCoord);
  shapeElement->appendChild(element);
  element = shapeDescription->createElement("left-back-bottom-point");
  element->setAttribute("x", negCoord);
  element->setAttribute("y", negCoord);
  element->setAttribute("z", negCoord);
  shapeElement->appendChild(element);
  element = shapeDescription->createElement("right-front-bottom-point");
  element->setAttribute("x", posCoord);
  element->setAttribute("y", negCoord);
  element->setAttribute("z", posCoord);
  shapeElement->appendChild(element);
  typeElement->appendChild(shapeElement);
  AutoPtr<Element> algebraElement = shapeDescription->createElement("algebra");
  algebraElement->setAttribute("val", "cube");
  typeElement->appendChild(algebraElement);
  Geometry::ShapeFactory shapeFactory;
  return shapeFactory.createShape(typeElement);
}

/** Calculate the distance between two points on a unit sphere.
 *  @param lat1 Latitude of the first point.
 *  @param long1 Longitude of the first point.
 *  @param lat2 Latitude of the second point.
 *  @param long2 Longitude of the second point.
 *  @return The distance between the points.
 */
double SparseWorkspace::greatCircleDistance(const double lat1,
                                            const double long1,
                                            const double lat2,
                                            const double long2) {
  const double latD = std::sin((lat2 - lat1) / 2.0);
  const double longD = std::sin((long2 - long1) / 2.0);
  const double S =
      latD * latD + std::cos(lat1) * std::cos(lat2) * longD * longD;
  return 2.0 * std::asin(std::sqrt(S));
}

/** Calculate the inverse distance weights for the given distances.
 *  @param distances The distances.
 *  @return An array of inverse distance weights.
 */
std::array<double, 4> SparseWorkspace::inverseDistanceWeights(
    const std::array<double, 4> &distances) {
  std::array<double, 4> weights;
  for (size_t i = 0; i < weights.size(); ++i) {
    if (distances[i] == 0.0) {
      weights.fill(0.0);
      weights[i] = 1.0;
      return weights;
    }
    weights[i] = 1.0 / distances[i] / distances[i];
  }
  return weights;
}

/** Spatially interpolate a single histogram from nearby detectors.
 *  @param lat Latitude of the interpolated detector.
 *  @param lon Longitude of the interpolated detector.
 *  @return An interpolated histogram.
 */
HistogramData::Histogram
SparseWorkspace::interpolateFromDetectorGrid(const double lat,
                                             const double lon) const {
  const auto indices = m_gridDef->nearestNeighbourIndices(lat, lon);

  auto h = histogram(0);

  const auto refFrame = getInstrument()->getReferenceFrame();
  std::array<double, 4> distances;
  for (size_t i = 0; i < 4; ++i) {
    double detLat, detLong;
    std::tie(detLat, detLong) = spectrumInfo().geographicalAngles(indices[i]);
    distances[i] = greatCircleDistance(lat, lon, detLat, detLong);
  }
  const auto weights = inverseDistanceWeights(distances);
  auto weightSum = weights[0];
  h.mutableY() = weights[0] * y(indices[0]);
  for (size_t i = 1; i < 4; ++i) {
    weightSum += weights[i];
    h.mutableY() += weights[i] * y(indices[i]);
  }
  h.mutableY() /= weightSum;
  return h;
}

} // namespace Algorithms
} // namespace Mantid
