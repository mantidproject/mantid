// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SampleCorrections/SparseInstrument.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/SampleCorrections/DetectorGridDefinition.h"
#include "MantidDataObjects/Workspace2D.h"
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
namespace SparseInstrument {

/** Calculate latitude and longitude for given vector.
 *  @param p A Mantid vector.
 *  @param refFrame A reference frame where p lives.
 *  @return A pair containing the latitude and longitude values.
 */
std::pair<double, double>
geographicalAngles(const Kernel::V3D &p,
                   const Geometry::ReferenceFrame &refFrame) {
  const double upCoord = p[refFrame.pointingUp()];
  const double beamCoord = p[refFrame.pointingAlongBeam()];
  const double leftoverCoord = p[refFrame.pointingHorizontal()];
  const double lat = std::atan2(upCoord, std::hypot(leftoverCoord, beamCoord));
  const double lon = std::atan2(leftoverCoord, beamCoord);
  return std::pair<double, double>(lat, lon);
}

/** Find the latitude and longitude intervals the detectors
 *  of the given workspace spawn as seen from the sample.
 *  @param ws A workspace.
 *  @return A tuple containing the latitude and longitude ranges.
 */
std::tuple<double, double, double, double>
extremeAngles(const API::MatrixWorkspace &ws) {
  const auto &spectrumInfo = ws.spectrumInfo();
  const auto samplePos = spectrumInfo.samplePosition();
  const auto refFrame = ws.getInstrument()->getReferenceFrame();
  double minLat = std::numeric_limits<double>::max();
  double maxLat = std::numeric_limits<double>::lowest();
  double minLong = std::numeric_limits<double>::max();
  double maxLong = std::numeric_limits<double>::lowest();
  for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
    double lat, lon;
    const auto detPos = spectrumInfo.position(i) - samplePos;
    std::tie(lat, lon) = geographicalAngles(detPos, *refFrame);
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
std::tuple<double, double> extremeWavelengths(const API::MatrixWorkspace &ws) {
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
modelHistogram(const API::MatrixWorkspace &modelWS,
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
Geometry::IObject_sptr makeCubeShape() {
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

/** Create a workspace whose instrument approximates that of modelWS.
 *  @param modelWS A workspace to model.
 *  @param grid An object defining the detector grid of the output workspace.
 *  @param wavelengthPoints Number of points in the output workspace.
 *  @return A workspace with sparse instrument.
 * @throw runtime_error If creation fails.
 */
API::MatrixWorkspace_uptr
createSparseWS(const API::MatrixWorkspace &modelWS,
               const Algorithms::DetectorGridDefinition &grid,
               const size_t wavelengthPoints) {
  // Build a quite standard and somewhat complete instrument.
  auto instrument =
      boost::make_shared<Geometry::Instrument>("MC_simulation_instrument");
  const auto refFrame = modelWS.getInstrument()->getReferenceFrame();

  instrument->setReferenceFrame(
      boost::make_shared<Geometry::ReferenceFrame>(*refFrame));
  // The sparse instrument is build around origin.
  constexpr Kernel::V3D samplePos{0.0, 0.0, 0.0};
  auto sample = std::make_unique<Geometry::ObjComponent>("sample", nullptr,
                                                         instrument.get());
  sample->setPos(samplePos);
  instrument->add(sample.get());
  instrument->markAsSamplePos(sample.release());
  const double R = 1.0; // This will be the default L2 distance.
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
  // Add detectors and link them to spectra.
  const size_t numSpectra = grid.numberColumns() * grid.numberRows();
  const auto h = modelHistogram(modelWS, wavelengthPoints);
  auto ws = DataObjects::create<DataObjects::Workspace2D>(numSpectra, h);
  auto detShape = makeCubeShape();
  for (size_t col = 0; col < grid.numberColumns(); ++col) {
    const auto lon = grid.longitudeAt(col);
    for (size_t row = 0; row < grid.numberRows(); ++row) {
      const auto lat = grid.latitudeAt(row);
      const size_t index = col * grid.numberRows() + row;
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
      ws->getSpectrum(index).setDetectorID(detID);
      instrument->add(det.get());
      instrument->markAsDetector(det.release());
    }
  }
  ws->setInstrument(instrument);
  // Copy things needed for the simulation from the model workspace.
  auto &paramMap = ws->instrumentParameters();
  auto parametrizedInstrument = ws->getInstrument();
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
  ws->mutableRun().addProperty("deltaE-mode",
                               Kernel::DeltaEMode::asString(eMode));
  if (eMode == Kernel::DeltaEMode::Direct) {
    ws->mutableRun().addProperty("Ei", modelWS.getEFixed());
  } else if (eMode == Kernel::DeltaEMode::Indirect) {
    const auto &detIDs = modelWS.detectorInfo().detectorIDs();
    if (!constantIndirectEFixed(modelWS, detIDs)) {
      throw std::runtime_error(
          "Sparse instrument with variable EFixed not supported.");
    }
    const auto e = modelWS.getEFixed(detIDs[0]);
    const auto &sparseDetIDs = ws->detectorInfo().detectorIDs();
    for (int sparseDetID : sparseDetIDs) {
      ws->setEFixed(sparseDetID, e);
    }
  }
  return API::MatrixWorkspace_uptr(ws.release());
}

/** Calculate the distance between two points on a unit sphere.
 *  @param lat1 Latitude of the first point.
 *  @param long1 Longitude of the first point.
 *  @param lat2 Latitude of the second point.
 *  @param long2 Longitude of the second point.
 *  @return The distance between the points.
 */
double greatCircleDistance(const double lat1, const double long1,
                           const double lat2, const double long2) {
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
std::array<double, 4>
inverseDistanceWeights(const std::array<double, 4> &distances) {
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

/** Spatially interpolate a single histogram from four nearby detectors.
 *  @param lat Latitude of the interpolated detector.
 *  @param lon Longitude of the interpolated detector.
 *  @param ws A workspace containing the detectors used for the interpolation.
 *  @param indices Indices to the nearest neighbour detectors.
 *  @return An interpolated histogram.
 */
HistogramData::Histogram
interpolateFromDetectorGrid(const double lat, const double lon,
                            const API::MatrixWorkspace &ws,
                            const std::array<size_t, 4> &indices) {
  auto h = ws.histogram(0);
  const auto &spectrumInfo = ws.spectrumInfo();
  const auto refFrame = ws.getInstrument()->getReferenceFrame();
  std::array<double, 4> distances;
  for (size_t i = 0; i < 4; ++i) {
    double detLat, detLong;
    std::tie(detLat, detLong) =
        geographicalAngles(spectrumInfo.position(indices[i]), *refFrame);
    distances[i] = greatCircleDistance(lat, lon, detLat, detLong);
  }
  const auto weights = inverseDistanceWeights(distances);
  auto weightSum = weights[0];
  h.mutableY() = weights[0] * ws.y(indices[0]);
  for (size_t i = 1; i < 4; ++i) {
    weightSum += weights[i];
    h.mutableY() += weights[i] * ws.y(indices[i]);
  }
  h.mutableY() /= weightSum;
  return h;
}

/** Creates a detector grid definition for a sparse instrument.
 *  @param modelWS A workspace the sparse instrument approximates.
 *  @param rows Number of rows in the detector grid.
 *  @param columns Number of columns in the detector gris.
 *  @return A unique pointer pointing to the grid definition.
 */
std::unique_ptr<const DetectorGridDefinition>
createDetectorGridDefinition(const API::MatrixWorkspace &modelWS,
                             const size_t rows, const size_t columns) {
  double minLat, maxLat, minLong, maxLong;
  std::tie(minLat, maxLat, minLong, maxLong) = extremeAngles(modelWS);
  return std::make_unique<Algorithms::DetectorGridDefinition>(
      minLat, maxLat, rows, minLong, maxLong, columns);
}
} // namespace SparseInstrument
} // namespace Algorithms
} // namespace Mantid
