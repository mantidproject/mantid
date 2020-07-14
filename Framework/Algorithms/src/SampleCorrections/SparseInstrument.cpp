// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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

/** Find the maximum and minimum wavelength points over the entire workpace.
 *  @param ws A workspace to investigate.
 *  @return A tuple containing the wavelength range.
 */
std::tuple<double, double>
SparseInstrument::extremeWavelengths(const API::MatrixWorkspace &ws) {
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
SparseInstrument::modelHistogram(const API::MatrixWorkspace &modelWS,
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
Geometry::IObject_sptr SparseInstrument::makeCubeShape() {
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
SparseInstrument::createSparseWS(const API::MatrixWorkspace &modelWS,
               const Algorithms::DetectorGridDefinition &grid,
               const size_t wavelengthPoints) {
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

/** Creates a detector grid definition for a sparse instrument.
 *  @param modelWS A workspace the sparse instrument approximates.
 *  @param rows Number of rows in the detector grid.
 *  @param columns Number of columns in the detector gris.
 *  @return A unique pointer pointing to the grid definition.
 */
std::unique_ptr<const DetectorGridDefinition>
SparseInstrument::createDetectorGridDefinition(
    const API::MatrixWorkspace &modelWS,
                             const size_t rows, const size_t columns) {
  double minLat, maxLat, minLong, maxLong;
  std::tie(minLat, maxLat, minLong, maxLong) = modelWS.spectrumInfo().extremeAngles();
  return std::make_unique<Algorithms::DetectorGridDefinition>(
      minLat, maxLat, rows, minLong, maxLong, columns);
}

} // namespace Algorithms
} // namespace Mantid
