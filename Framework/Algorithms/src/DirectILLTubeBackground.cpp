// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
/// Namespace containing the algorithm's property names.
namespace Prop {
const std::string COMPONENTS{"Components"};
const std::string DIAGNOSTICS_WS{"DiagnosticsWorkspace"};
const std::string EPP_WS{"EPPWorkspace"};
const std::string INPUT_WS{"InputWorkspace"};
const std::string OUTPUT_WS{"OutputWorkspace"};
const std::string POLYNOMIAL_DEGREE{"Degree"};
const std::string SIGMA_MULTIPLIER{"NonBkgRegionInSigmas"};
} // namespace Prop

/**
 * @brief Returns the background fitting range in workspace indices.
 * @param ws a workspace
 * @param statuses the fit status column of a EPP workspace
 * @return a vector of [begin, end) pairs
 */
std::vector<double> bkgFittingRanges(Mantid::API::MatrixWorkspace const &ws,
                                     Mantid::API::Column const &statuses,
                                     size_t const firstColumnIndex) {
  std::vector<double> ranges;
  auto const &spectrumInfo = ws.spectrumInfo();
  bool needsRangeStart{true};
  bool needsRangeEnd{false};
  for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
    if (spectrumInfo.isMasked(i) ||
        statuses.cell<std::string>(i + firstColumnIndex) != "success") {
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

/// A list of peak limits.
struct PeakBounds {
  /// A vector of peak's lower X limits.
  std::vector<double> peakStarts;
  /// A vector of peak's upper X limits.
  std::vector<double> peakEnds;
};

/**
 * @brief Make a list of peak lower and upper X limits.
 * @param firstIndex first row to consider
 * @param lastIndex last row to consider
 * @param sigmaMultiplier half-width sigma multiplier for peak width
 * @param peakCentreColumn a column of peak centres
 * @param sigmaColumn a column of sigma values (measures of peak width)
 * @param fitStatusColumn a column of EPP fit statuses
 * @return a PeakBounds object containing the peak limits
 */
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

/**
 * @brief Check if the given columns are nullptr
 * @param centreColumn an EPP peak centre column
 * @param sigmaColumn an EPP sigma column
 * @param statusColumn an EPP fit status column
 * @throw NotFoundError if any of the parameters is nullptr
 */
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

/**
 * @brief Check if given component exists in the instrument.
 * @param componentName the name of the component
 * @param instrument an instrument
 * @throw NotFoundError if the instrument does not contain the component
 */
void checkComponentExists(std::string const &componentName,
                          Mantid::Geometry::Instrument const &instrument) {
  using namespace Mantid::Kernel::Exception;
  auto const component = instrument.getComponentByName(componentName);
  if (!component) {
    throw NotFoundError("Component not found in InputWorkspace's instrument.",
                        componentName);
  }
}

/// An inclusive workspace index range.
struct Range {
  /// First workspace index.
  size_t first{0};
  /// Last workspace index.
  size_t last{0};
};

/**
 * @brief Find the corresponding ws indices from the original workspace.
 * @param componentWS a component workspace cropped from originalWS
 * @param originalWS the original workspace
 * @return a Range of workspace indices
 */
Range componentWSIndexRange(Mantid::API::MatrixWorkspace const &componentWS,
                            Mantid::API::MatrixWorkspace const &originalWS) {
  Range range;
  auto const firstComponentSpectrumNo =
      componentWS.getSpectrum(0).getSpectrumNo();
  range.first = originalWS.getIndexFromSpectrumNumber(firstComponentSpectrumNo);
  auto const nComponentHistograms = componentWS.getNumberHistograms();
  auto const lastComponentSpectrumNo =
      componentWS.getSpectrum(nComponentHistograms - 1).getSpectrumNo();
  range.last = originalWS.getIndexFromSpectrumNumber(lastComponentSpectrumNo);
  return range;
}

/**
 * @brief Write Y values and errors to targetWS
 * @param componentBkgWS the source workspace
 * @param targetWS the target workspace
 * @param firstTargetWSIndex begin writing at this workspace index
 */
void writeComponentBackgroundToOutput(
    Mantid::API::MatrixWorkspace const &componentBkgWS,
    Mantid::API::MatrixWorkspace &targetWS, size_t const firstTargetWSIndex) {
  auto const &ys = componentBkgWS.y(0);
  auto const &es = componentBkgWS.e(0);
  for (size_t i = 0; i < ys.size(); ++i) {
    targetWS.mutableY(firstTargetWSIndex + i) = ys[i];
    targetWS.mutableE(firstTargetWSIndex + i) = es[i];
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
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::INPUT_WS, "", Kernel::Direction::Input),
      "A workspace to fit the backgrounds to.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::OUTPUT_WS, "", Kernel::Direction::Output),
      "The fitted backgrounds.");
  declareProperty(
      std::make_unique<Kernel::ArrayProperty<std::string>>(
          Prop::COMPONENTS, std::vector<std::string>()),
      "A list of component names for which to calculate the backgrounds.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
          Prop::EPP_WS, "", Kernel::Direction::Input),
      "A table workspace containing results from the FindEPP algorithm.");
  auto positiveDouble = boost::make_shared<Kernel::BoundedValidator<double>>();
  positiveDouble->setLowerExclusive(0.);
  declareProperty(
      Prop::SIGMA_MULTIPLIER, 6., positiveDouble,
      "Half width of the range excluded from background around "
      "the elastic peaks in multiplies of 'Sigma' in the EPP table.'.");
  auto nonnegativeInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  nonnegativeInt->setLower(0);
  declareProperty(Prop::POLYNOMIAL_DEGREE, 0, nonnegativeInt,
                  "The degree of the background polynomial.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::DIAGNOSTICS_WS, "", Kernel::Direction::Input,
          API::PropertyMode::Optional),
      "Detector diagnostics workspace for masking.");
}

/// Validate input properties.
std::map<std::string, std::string> DirectILLTubeBackground::validateInputs() {
  std::map<std::string, std::string> issues;
  API::MatrixWorkspace_sptr inWS = getProperty(Prop::INPUT_WS);
  API::ITableWorkspace_sptr eppWS = getProperty(Prop::EPP_WS);
  if (inWS->getNumberHistograms() != eppWS->rowCount()) {
    issues[Prop::EPP_WS] =
        "Wrong EPP workspace? The number of the table rows "
        "does not match the number of histograms in InputWorkspace.";
  }
  if (!isDefault(Prop::DIAGNOSTICS_WS)) {
    API::MatrixWorkspace_sptr maskWS = getProperty(Prop::DIAGNOSTICS_WS);
    if (inWS->getNumberHistograms() != maskWS->getNumberHistograms()) {
      issues[Prop::EPP_WS] =
          "Wrong diagnostics workspace? The number of histograms "
          "does not match with InputWorkspace.";
    }
  }
  return issues;
}

/** Execute the algorithm.
 */
void DirectILLTubeBackground::exec() {
  API::MatrixWorkspace_sptr inWS = getProperty(Prop::INPUT_WS);
  auto ws = applyDiagnostics(inWS->clone());
  API::MatrixWorkspace_sptr bkgWS =
      DataObjects::create<DataObjects::Workspace2D>(*ws);
  for (size_t i = 0; i < bkgWS->getNumberHistograms(); ++i) {
    bkgWS->convertToFrequencies(i);
  }
  API::ITableWorkspace_sptr eppWS = getProperty(Prop::EPP_WS);
  double const sigmaMultiplier = getProperty(Prop::SIGMA_MULTIPLIER);
  auto peakCentreColumn = eppWS->getColumn("PeakCentre");
  auto sigmaColumn = eppWS->getColumn("Sigma");
  auto fitStatusColumn = eppWS->getColumn("FitStatus");
  checkEPPColumnsExist(peakCentreColumn, sigmaColumn, fitStatusColumn);
  auto instrument = ws->getInstrument();
  std::vector<std::string> const componentNames = components(*instrument);
  API::Progress progress(this, 0.0, 1.0, componentNames.size());
  PARALLEL_FOR_IF(Kernel::threadSafe(*ws))
  for (int64_t i = 0; i < static_cast<int64_t>(componentNames.size()); ++i) {
    PARALLEL_START_INTERUPT_REGION
    auto const &componentName = componentNames[static_cast<size_t>(i)];
    progress.report("Processing " + componentName);
    checkComponentExists(componentName, *instrument);
    auto componentWS = cropToComponent(ws, componentName);
    auto const wsIndexRange = componentWSIndexRange(*componentWS, *ws);
    auto const bkgRanges =
        bkgFittingRanges(*componentWS, *fitStatusColumn, wsIndexRange.first);
    if (bkgRanges.empty()) {
      continue;
    }
    auto const bounds =
        peakBounds(wsIndexRange.first, wsIndexRange.last, sigmaMultiplier,
                   *peakCentreColumn, *sigmaColumn, *fitStatusColumn);
    auto averageWS =
        peakExcludingAverage(*componentWS, bounds.peakStarts, bounds.peakEnds);
    auto fittedComponentBkg = fitComponentBackground(averageWS, bkgRanges);
    writeComponentBackgroundToOutput(*fittedComponentBkg, *bkgWS,
                                     wsIndexRange.first);
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  if (!ws->isDistribution()) {
    for (size_t i = 0; i < bkgWS->getNumberHistograms(); ++i) {
      bkgWS->convertToCounts(i);
    }
  }
  setProperty(Prop::OUTPUT_WS, bkgWS);
}

/**
 * @brief Apply a mask workspace (if given) to ws.
 * @param ws a workspace to apply the mask to
 * @return a masked workspace
 */
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

/**
 * @brief Return a list of component names for the algorithm to process.
 * @param instrument an instrument
 * @return a list of comopnent names
 * @throw std::runtime_error if the names cannot be found anywhere
 */
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

/**
 * @brief Crop a component workspace out of ws.
 * @param ws a workspace to crop
 * @param componentName name of the component to crop
 * @return a component workspace
 */
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

/**
 * @brief Fits a polynomial background.
 * @param ws a workspace to fit the background to
 * @param xRanges fitting ranges
 * @return the fitted backgrounds
 */
API::MatrixWorkspace_sptr DirectILLTubeBackground::fitComponentBackground(
    API::MatrixWorkspace_sptr &ws, std::vector<double> const &xRanges) {
  int const degree = getProperty(Prop::POLYNOMIAL_DEGREE);
  auto calculateBkg = createChildAlgorithm("CalculatePolynomialBackground");
  calculateBkg->setProperty("InputWorkspace", ws);
  calculateBkg->setProperty("OutputWorkspace", "_unused");
  calculateBkg->setProperty("Degree", degree);
  calculateBkg->setProperty("XRanges", xRanges);
  calculateBkg->setProperty("CostFunction", "Unweighted least squares");
  calculateBkg->execute();
  return calculateBkg->getProperty("OutputWorkspace");
}

/**
 * @brief Average the histograms of a workspace.
 * @param ws a workspace to average
 * @param peakStarts start X values of an exclusion range
 * @param peakEnds end X values of an exclusion range
 * @return a single histogram workspace containing the averages
 */
API::MatrixWorkspace_sptr DirectILLTubeBackground::peakExcludingAverage(
    API::MatrixWorkspace const &ws, std::vector<double> const &peakStarts,
    std::vector<double> const &peakEnds) {
  HistogramData::Points wsIndices{ws.getNumberHistograms(),
                                  HistogramData::LinearGenerator(0., 1.)};
  // zeroCounts actually holds the mean frequencies but since its
  // point data the type has to be Counts.
  HistogramData::Counts zeroCounts(ws.getNumberHistograms(), 0.);
  HistogramData::Histogram modelHistogram{wsIndices, zeroCounts};
  API::MatrixWorkspace_sptr averageWS =
      DataObjects::create<DataObjects::Workspace2D>(1, modelHistogram);
  for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
    auto const peakStart = peakStarts[i];
    auto const peakEnd = peakEnds[i];
    auto histogram = ws.histogram(i);
    size_t itemCount{0};
    double &sum = averageWS->mutableY(0)[i];
    double &error = averageWS->mutableE(0)[i];
    for (auto &histogramItem : ws.histogram(i)) {
      auto const center = histogramItem.center();
      if (peakStart <= center && center <= peakEnd) {
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
