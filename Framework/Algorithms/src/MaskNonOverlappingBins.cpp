// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MaskNonOverlappingBins.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Unit.h"

namespace {
/// Constants for the algorithm's property names.
namespace Prop {
std::string const CHECK_SORTING{"CheckSortedX"};
std::string const COMPARISON_WS{"ComparisonWorkspace"};
std::string const INPUT_WS{"InputWorkspace"};
std::string const MASK_PARTIAL{"MaskPartiallyOverlapping"};
std::string const OUTPUT_WS{"OutputWorkspace"};
std::string const RAGGEDNESS{"RaggedInputs"};
} // namespace Prop
/// Constants for the RaggedInputs property.
namespace Raggedness {
std::string const CHECK{"Check"};
std::string const RAGGED{"Ragged"};
std::string const NONRAGGED{"Common Bins"};
} // namespace Raggedness

/// Return true if X data is sorted in ascending order.
bool isXSorted(Mantid::API::MatrixWorkspace const &ws) {
  int unsorted{0};
  PARALLEL_FOR_IF(Mantid::Kernel::threadSafe(ws))
  for (int64_t i = 0; i < static_cast<int64_t>(ws.getNumberHistograms()); ++i) {
    auto const &Xs = ws.x(i);
    if (!std::is_sorted(Xs.cbegin(), Xs.cend())) {
      PARALLEL_ATOMIC
      ++unsorted;
    }
  }
  return unsorted == 0;
}

/// Holds limiting bin indices for masking
struct BinIndices {
  size_t frontEndIndex;  ///< Mask from 0 to this index.
  size_t backBeginIndex; ///< Mask from this index to size.
};

/// Return the proper masking limits for non-overlapping bins.
BinIndices maskingLimits(Mantid::API::MatrixWorkspace const &ws,
                         Mantid::API::MatrixWorkspace const &comparisonWS,
                         bool const maskPartial, size_t const histogramIndex) {
  auto const &Xs = ws.x(histogramIndex);
  auto const &comparisonXs = comparisonWS.x(histogramIndex);
  // At the moment we only support increasing X.
  auto const startX = comparisonXs.front();
  auto const endX = comparisonXs.back();
  // There is no Y corresponding to the last bin edge,
  // therefore iterate only to cend() - 1.
  auto frontEnd = std::lower_bound(Xs.cbegin(), Xs.cend() - 1, startX);
  if (!maskPartial && frontEnd != Xs.cbegin() && *frontEnd != startX) {
    --frontEnd;
  }
  auto backBegin = std::lower_bound(frontEnd, Xs.cend() - 1, endX);
  if (maskPartial && backBegin != Xs.cbegin() && *backBegin > endX) {
    --backBegin;
  }
  BinIndices limits;
  limits.frontEndIndex =
      static_cast<size_t>(std::distance(Xs.cbegin(), frontEnd));
  limits.backBeginIndex =
      static_cast<size_t>(std::distance(Xs.cbegin(), backBegin));
  return limits;
}

/// Mask the start and end of histogram at histogram index.
void maskBinsWithinLimits(Mantid::API::MatrixWorkspace &ws,
                          size_t const histogramIndex,
                          BinIndices const &limits) {
  auto const xSize = ws.x(histogramIndex).size();
  for (size_t binIndex = 0; binIndex < limits.frontEndIndex; ++binIndex) {
    ws.flagMasked(histogramIndex, binIndex);
  }
  for (size_t binIndex = limits.backBeginIndex; binIndex < xSize - 1;
       ++binIndex) {
    ws.flagMasked(histogramIndex, binIndex);
  }
}
} // namespace

namespace Mantid {
namespace Algorithms {
DECLARE_ALGORITHM(MaskNonOverlappingBins)

/// Algorithms name for identification. @see Algorithm::name
std::string const MaskNonOverlappingBins::name() const {
  return "MaskNonOverlappingBins";
}

/// Algorithm's version for identification. @see Algorithm::version
int MaskNonOverlappingBins::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
std::string const MaskNonOverlappingBins::category() const {
  return "Transforms\\Masking";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
std::string const MaskNonOverlappingBins::summary() const {
  return "Marks bins in InputWorkspace which are out of the X range of the "
         "second workspace.";
}

/// Return a list of the names of related algorithms.
std::vector<std::string> const MaskNonOverlappingBins::seeAlso() const {
  return {"MaskBins", "MaskBinsIf"};
}

/** Initialize the algorithm's properties.
 */
void MaskNonOverlappingBins::init() {
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<>>(
                      Prop::INPUT_WS, "", Kernel::Direction::Input),
                  "A workspace to mask.");
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<>>(
                      Prop::OUTPUT_WS, "", Kernel::Direction::Output),
                  "The masked workspace.");
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<>>(
                      Prop::COMPARISON_WS, "", Kernel::Direction::Input),
                  "A workspace to compare the InputWorkspace's binning to.");
  declareProperty(Prop::MASK_PARTIAL, false,
                  "If true, mask also bins that overlap only partially.");
  std::vector<std::string> const options{Raggedness::CHECK, Raggedness::RAGGED,
                                         Raggedness::NONRAGGED};
  auto raggednessOptions =
      boost::make_shared<Kernel::ListValidator<std::string>>(options);
  declareProperty(Prop::RAGGEDNESS, Raggedness::CHECK, raggednessOptions,
                  "Choose whether the input workspaces have common bins, are "
                  "ragged, or if the algorithm should check.");
  declareProperty(
      Prop::CHECK_SORTING, true,
      "If true, the algorithm ensures that both workspaces have X sorted in "
      "ascending order.");
}

/// Returns a map from property name to message in case of invalid values.
std::map<std::string, std::string> MaskNonOverlappingBins::validateInputs() {
  std::map<std::string, std::string> issues;
  API::MatrixWorkspace_const_sptr inputWS = getProperty(Prop::INPUT_WS);
  API::MatrixWorkspace_const_sptr comparisonWS =
      getProperty(Prop::COMPARISON_WS);
  if (inputWS->getNumberHistograms() != comparisonWS->getNumberHistograms()) {
    issues[Prop::COMPARISON_WS] =
        "The number of histogams mismatches with " + Prop::INPUT_WS;
  }
  if (!inputWS->isHistogramData()) {
    issues[Prop::INPUT_WS] =
        "The workspace contains point data, not histograms.";
  }
  if (!comparisonWS->isHistogramData()) {
    issues[Prop::COMPARISON_WS] =
        "The workspace contains point data, not histograms.";
  }
  auto const inputAxis = inputWS->getAxis(0);
  auto const comparisonAxis = comparisonWS->getAxis(0);
  if (inputAxis && comparisonAxis) {
    if (*(inputAxis->unit()) != *(comparisonAxis->unit())) {
      issues[Prop::COMPARISON_WS] =
          "X units do not match with " + Prop::INPUT_WS;
    }
  }
  return issues;
}

/** Execute the algorithm.
 */
void MaskNonOverlappingBins::exec() {
  API::MatrixWorkspace_sptr inputWS = getProperty(Prop::INPUT_WS);
  API::MatrixWorkspace_sptr outputWS = getProperty(Prop::OUTPUT_WS);
  if (inputWS != outputWS) {
    outputWS = inputWS->clone();
  }
  API::MatrixWorkspace_const_sptr comparisonWS =
      getProperty(Prop::COMPARISON_WS);
  checkXSorting(*inputWS, *comparisonWS);
  if (isCommonBins(*inputWS, *comparisonWS)) {
    processNonRagged(*inputWS, *comparisonWS, *outputWS);
  } else {
    processRagged(*inputWS, *comparisonWS, *outputWS);
  }
  setProperty(Prop::OUTPUT_WS, outputWS);
}

/// Throw if the workspaces don't have sorted X.
void MaskNonOverlappingBins::checkXSorting(
    API::MatrixWorkspace const &inputWS,
    API::MatrixWorkspace const &comparisonWS) {
  bool const checkSorting = getProperty(Prop::CHECK_SORTING);
  if (checkSorting) {
    if (!isXSorted(inputWS)) {
      throw std::invalid_argument(Prop::INPUT_WS + " has unsorted X.");
    }
    if (!isXSorted(comparisonWS)) {
      throw std::invalid_argument(Prop::COMPARISON_WS + " has unsorted X.");
    }
  }
}

/// Return true if the workspaces should be considered as having common bins.
bool MaskNonOverlappingBins::isCommonBins(
    API::MatrixWorkspace const &inputWS,
    API::MatrixWorkspace const &comparisonWS) {
  std::string const choice = getProperty(Prop::RAGGEDNESS);
  if (choice == Raggedness::CHECK) {
    return inputWS.isCommonBins() && comparisonWS.isCommonBins();
  } else {
    return choice == Raggedness::NONRAGGED;
  }
}

/// Mask all types of workspaces, ragged or nonragged.
void MaskNonOverlappingBins::processRagged(
    API::MatrixWorkspace const &inputWS,
    API::MatrixWorkspace const &comparisonWS, API::MatrixWorkspace &outputWS) {
  bool const maskPartial = getProperty(Prop::MASK_PARTIAL);
  auto const nHist = inputWS.getNumberHistograms();
  API::Progress progress(this, 0., 1., nHist);
  // Tried to parallelize this loop but the performance test showed
  // regression. Hence no PARALLEL_FOR_IF.
  for (size_t histogramIndex = 0; histogramIndex < nHist; ++histogramIndex) {
    auto const limits =
        maskingLimits(inputWS, comparisonWS, maskPartial, histogramIndex);
    maskBinsWithinLimits(outputWS, histogramIndex, limits);
    progress.report("Masking nonoverlapping bins");
  }
}

/// Mask only workspace with same X in all histograms.
void MaskNonOverlappingBins::processNonRagged(
    API::MatrixWorkspace const &inputWS,
    API::MatrixWorkspace const &comparisonWS, API::MatrixWorkspace &outputWS) {
  bool const maskPartial = getProperty(Prop::MASK_PARTIAL);
  auto const nHist = inputWS.getNumberHistograms();
  API::Progress progress(this, 0., 1., nHist);
  auto const limits = maskingLimits(inputWS, comparisonWS, maskPartial, 0);
  for (size_t histogramIndex = 0; histogramIndex < nHist; ++histogramIndex) {
    maskBinsWithinLimits(outputWS, histogramIndex, limits);
    progress.report("Masking nonoverlapping bins");
  }
}

} // namespace Algorithms
} // namespace Mantid
