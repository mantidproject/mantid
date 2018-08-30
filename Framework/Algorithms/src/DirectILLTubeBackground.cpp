#include "MantidAlgorithms/DirectILLTubeBackground.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/HistogramIterator.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

namespace {
namespace Prop {
const std::string COMPONENTS{"Components"};
const std::string DIAGNOSTICS_WS{"DiagnosticsWorkspace"};
const std::string EPP_WS{"EPPWorkspace"};
const std::string INPUT_WS{"InputWorkspace"};
const std::string OUTPUT_WS{"OutputWorkspace"};
const std::string SIGMA_MULTIPLIER{"NonBkgRegionInSigmas"};
} // namespace Prop

std::vector<double> bkgFittingRanges(Mantid::API::MatrixWorkspace const &ws,
                                     Mantid::API::Column const &statuses) {
  std::vector<double> ranges;
  auto const &spectrumInfo = ws.spectrumInfo();
  bool needsRangeStart{true};
  bool needsRangeEnd{false};
  for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
    if (spectrumInfo.isMasked(i) ||
        statuses.cell<std::string>(i) != "success") {
      if (needsRangeEnd) {
        needsRangeStart = true;
        needsRangeEnd = false;
        // Current spectrum number is the first to exclude.
        ranges.emplace_back(static_cast<double>(i) - 0.5);
      }
      continue;
    }
    if (needsRangeStart) {
      needsRangeStart = false;
      needsRangeEnd = true;
      ranges.emplace_back(static_cast<double>(i) - 0.5);
    }
  }
  if (needsRangeEnd) {
    ranges.emplace_back(static_cast<double>(ws.getNumberHistograms()) - 0.5);
  }
  return ranges;
}

struct PeakBounds {
  std::vector<double> peakStarts;
  std::vector<double> peakEnds;
};

PeakBounds peakBounds(size_t const firstIndex, size_t const lastIndex,
                      double const sigmaMultiplier,
                      Mantid::API::Column const &peakCentreColumn,
                      Mantid::API::Column const &sigmaColumn,
                      Mantid::API::Column const &fitStatusColumn) {
  PeakBounds bounds;
  size_t const boundsSize = lastIndex - firstIndex + 1;
  bounds.peakStarts.resize(boundsSize);
  bounds.peakEnds.resize(boundsSize);
  for (size_t i = firstIndex; i <= lastIndex; ++i) {
    auto const boundsIndex = i - firstIndex;
    if (fitStatusColumn.cell<std::string>(i) == "success") {
      auto const peakCentre = peakCentreColumn.cell<double>(i);
      auto const peakWidth = sigmaMultiplier * sigmaColumn.cell<double>(i);
      bounds.peakStarts[boundsIndex] = peakCentre - peakWidth;
      bounds.peakEnds[boundsIndex] = peakCentre + peakWidth;
    } else {
      bounds.peakStarts[boundsIndex] = std::numeric_limits<double>::lowest();
      bounds.peakEnds[boundsIndex] = std::numeric_limits<double>::max();
    }
  }
  return bounds;
}

void checkEPPColumnsExist(Mantid::API::Column_sptr const &centreColumn,
                          Mantid::API::Column_sptr const &sigmaColumn,
                          Mantid::API::Column_sptr const &statusColumn) {
  using namespace Mantid::Kernel::Exception;
  if (!centreColumn) {
    throw NotFoundError("EPPWorkspace does not contain 'PeakCentre' column.",
                        "PeakCentre");
  }
  if (!sigmaColumn) {
    throw NotFoundError("EPPWorkspace does not contain 'Sigma' column",
                        "Sigma");
  }
  if (!statusColumn) {
    throw NotFoundError("EPPWorkspace does not contain 'FitStatus' column.",
                        "FitStatus");
  }
}

void checkComponentExists(std::string const &componentName,
                          Mantid::Geometry::Instrument const &instrument) {
  using namespace Mantid::Kernel::Exception;
  auto const component = instrument.getComponentByName(componentName);
  if (!component) {
    throw NotFoundError("Component not found in InputWorkspace's instrument.",
                        componentName);
  }
}

} // namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DirectILLTubeBackground)

/// Algorithms name for identification. @see Algorithm::name
const std::string DirectILLTubeBackground::name() const {
  return "DirectILLTubeBackground";
}

/// Algorithm's version for identification. @see Algorithm::version
int DirectILLTubeBackground::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string DirectILLTubeBackground::category() const {
  return "CorrectionFunctions\\BackgroundCorrections;ILL\\Direct";
}

/// Return a vector of related algorithms.
const std::vector<std::string> DirectILLTubeBackground::seeAlso() const {
  return {"CalculateFlatBackground", "CalculatePolynomialBackground",
          "CreateUserDefinedBackground", "RemoveBackground",
          "SplineBackground"};
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string DirectILLTubeBackground::summary() const {
  return "Fits polynomial backgrounds over the pixels of position sensitive "
         "tubes.";
}

/** Initialize the algorithm's properties.
 */
void DirectILLTubeBackground::init() {
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::INPUT_WS, "", Kernel::Direction::Input),
      "A workspace to fit the backgrounds to.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::OUTPUT_WS, "", Kernel::Direction::Output),
      "The fitted backgrounds.");
  declareProperty(
      Kernel::make_unique<Kernel::ArrayProperty<std::string>>(
          Prop::COMPONENTS, std::vector<std::string>()),
      "A list of component names for which to calculate the backgrounds.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
          Prop::EPP_WS, "", Kernel::Direction::Input),
      "A table workspace containing results from the FindEPP algorithm.");
  auto positiveDouble = boost::make_shared<Kernel::BoundedValidator<double>>();
  positiveDouble->setLowerExclusive(0.);
  declareProperty(
      Prop::SIGMA_MULTIPLIER, 6., positiveDouble,
      "Half width of the range excluded from background around "
      "the elastic peaks in multiplies of 'Sigma' in the EPP table.'.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::DIAGNOSTICS_WS, "", Kernel::Direction::Input, API::PropertyMode::Optional),
      "Detector diagnostics workspace for masking.");
}

/** Execute the algorithm.
 */
void DirectILLTubeBackground::exec() {
  API::MatrixWorkspace_sptr inWS = getProperty(Prop::INPUT_WS);
  auto ws = applyDiagnostics(inWS->clone());
  API::MatrixWorkspace_sptr bkgWS =
      DataObjects::create<DataObjects::Workspace2D>(*ws);
  API::ITableWorkspace_sptr eppWS = getProperty(Prop::EPP_WS);
  double const sigmaMultiplier = getProperty(Prop::SIGMA_MULTIPLIER);
  auto peakCentreColumn = eppWS->getColumn("PeakCentre");
  auto sigmaColumn = eppWS->getColumn("Sigma");
  auto fitStatusColumn = eppWS->getColumn("FitStatus");
  checkEPPColumnsExist(peakCentreColumn, sigmaColumn, fitStatusColumn);
  auto instrument = ws->getInstrument();
  std::vector<std::string> const componentNames = components(*instrument);
  API::Progress progress(this, 0.0, 1.0, componentNames.size());
  for (auto const &componentName : componentNames) {
    progress.report("Processing " + componentName);
    checkComponentExists(componentName, *instrument);
    API::MatrixWorkspace_sptr componentWS = cropToComponent(ws, componentName);
    auto const bkgRanges = bkgFittingRanges(*componentWS, *fitStatusColumn);
    if (bkgRanges.empty()) {
      continue;
    }
    auto const firstComponentSpectrumNo =
        componentWS->getSpectrum(0).getSpectrumNo();
    auto const firstIndex =
        ws->getIndexFromSpectrumNumber(firstComponentSpectrumNo);
    auto const nComponentHistograms = componentWS->getNumberHistograms();
    auto const lastComponentSpectrumNo =
        componentWS->getSpectrum(nComponentHistograms - 1).getSpectrumNo();
    auto const lastIndex =
        ws->getIndexFromSpectrumNumber(lastComponentSpectrumNo);
    auto const bounds =
        peakBounds(firstIndex, lastIndex, sigmaMultiplier, *peakCentreColumn,
                   *sigmaColumn, *fitStatusColumn);
    auto averageWS = peakExcludingAverage(*componentWS, bounds.peakStarts, bounds.peakEnds);
    API::MatrixWorkspace_sptr fittedComponentBkg =
        fitComponentBackground(averageWS, bkgRanges);
    auto const &bkgYs = fittedComponentBkg->y(0);
    auto const &bkgEs = fittedComponentBkg->e(0);
    for (size_t i = 0; i < bkgYs.size(); ++i) {
      bkgWS->mutableY(firstIndex + i) = bkgYs[i];
      bkgWS->mutableE(firstIndex + i) = bkgEs[i];
    }
  }
  setProperty(Prop::OUTPUT_WS, bkgWS);
}

API::MatrixWorkspace_sptr
DirectILLTubeBackground::applyDiagnostics(API::MatrixWorkspace_sptr ws) {
  if (isDefault(Prop::DIAGNOSTICS_WS)) {
    return ws;
  }
  API::MatrixWorkspace_sptr diagnosticsWS = getProperty(Prop::DIAGNOSTICS_WS);
  auto mask = createChildAlgorithm("MaskDetectors");
  mask->setProperty("Workspace", ws);
  mask->setProperty("MaskedWorkspace", diagnosticsWS);
  mask->execute();
  return ws;
}

std::vector<std::string>
DirectILLTubeBackground::components(Geometry::Instrument const &instrument) {
  if (isDefault(Prop::COMPONENTS)) {
    std::string const COMPONENTS_PARAMETER{"components-for-backgrounds"};
    if (!instrument.hasParameter(COMPONENTS_PARAMETER)) {
      throw std::runtime_error("Could not find '" + COMPONENTS_PARAMETER +
                               "' in instrument parameters file. Component "
                               "names must be given using the '" +
                               Prop::COMPONENTS + "' property.");
    }
    auto const componentList =
        instrument.getStringParameter(COMPONENTS_PARAMETER).front();
    setPropertyValue(Prop::COMPONENTS, componentList);
  }
  return getProperty(Prop::COMPONENTS);
}

API::MatrixWorkspace_sptr
DirectILLTubeBackground::cropToComponent(API::MatrixWorkspace_sptr &ws,
                                         std::string const &componentName) {
  auto crop = createChildAlgorithm("CropToComponent");
  crop->setProperty("InputWorkspace", ws);
  crop->setProperty("OutputWorkspace", "_unused");
  crop->setProperty("ComponentNames", std::vector<std::string>{componentName});
  crop->execute();
  return crop->getProperty("OutputWorkspace");
}

API::MatrixWorkspace_sptr DirectILLTubeBackground::fitComponentBackground(
    API::MatrixWorkspace_sptr &ws, std::vector<double> const &xRanges) {
  auto calculateBkg = createChildAlgorithm("CalculatePolynomialBackground");
  calculateBkg->setProperty("InputWorkspace", ws);
  calculateBkg->setProperty("OutputWorkspace", "_unused");
  calculateBkg->setProperty("XRanges", xRanges);
  calculateBkg->setProperty("CostFunction", "Unweighted least squares");
  calculateBkg->execute();
  return calculateBkg->getProperty("OutputWorkspace");
}

API::MatrixWorkspace_sptr DirectILLTubeBackground::peakExcludingAverage(
    API::MatrixWorkspace const &ws, std::vector<double> const &peakStarts,
    std::vector<double> const &peakEnds) {
  HistogramData::Points wsIndices{ws.getNumberHistograms(),
                                  HistogramData::LinearGenerator(0., 1.)};
  HistogramData::Counts zeroCounts(ws.getNumberHistograms(), 0.);
  HistogramData::Histogram modelHistogram{wsIndices, zeroCounts};
  API::MatrixWorkspace_sptr averageWS =
      DataObjects::create<DataObjects::Workspace2D>(1, modelHistogram);
  for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
    auto const peakStart = peakStarts[i];
    auto const peakEnd = peakEnds[i];
    auto histogram = ws.histogram(i);
    auto points = histogram.points();
    size_t itemCount{0};
    double &sum = averageWS->mutableY(0)[i];
    double &error = averageWS->mutableE(0)[i];
    for (auto &histogramItem : ws.histogram(i)) {
      auto const center = histogramItem.center();
      if (center >= peakStart && center <= peakEnd) {
        continue;
      }
      sum += histogramItem.frequency();
      auto const stdDev = histogramItem.frequencyStandardDeviation();
      error = std::sqrt(error * error + stdDev * stdDev);
      ++itemCount;
    }
    if (itemCount != 0) {
      sum /= static_cast<double>(itemCount);
      error /= static_cast<double>(itemCount);
    }
  }
  return averageWS;
}

} // namespace Algorithms
} // namespace Mantid
