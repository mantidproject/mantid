#include "MantidAlgorithms/MaskNonOverlappingBins.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/MultiThreaded.h"

namespace {
/// Constants for the algorithm's property names.
namespace Prop {
std::string const COMPARISON_WS{"ComparisonWorkspace"};
std::string const INPUT_WS{"InputWorkspace"};
std::string const MASK_PARTIAL{"MaskPartiallyOverlapping"};
std::string const OUTPUT_WS{"OutputWorkspace"};
} // namespace Prop
} // namespace

namespace Mantid {
namespace Algorithms {
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

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
  bool const maskPartial = getProperty(Prop::MASK_PARTIAL);
  auto const nHist = inputWS->getNumberHistograms();
  API::Progress progress(this, 0., 1., nHist);
  // Tried to parallelize this loop but the performance test showed
  // regression.
  for (size_t histogramIndex = 0; histogramIndex < nHist; ++histogramIndex) {
    auto const &inXs = inputWS->x(histogramIndex);
    auto const &comparisonXs = comparisonWS->x(histogramIndex);
    // At the moment we only support increasing X.
    auto const startX = comparisonXs.front();
    auto const endX = comparisonXs.back();
    // There is no Y corresponding to the last bin edge,
    // therefore iterate only to cend() - 1.
    auto frontEnd = std::lower_bound(inXs.cbegin(), inXs.cend() - 1, startX);
    if (!maskPartial && frontEnd != inXs.cbegin() && *frontEnd != startX) {
      --frontEnd;
    }
    auto backBegin = std::lower_bound(frontEnd, inXs.cend() - 1, endX);
    if (maskPartial && backBegin != inXs.cbegin() && *backBegin > endX) {
      --backBegin;
    }
    auto const frontEndIndex =
        static_cast<size_t>(std::distance(inXs.cbegin(), frontEnd));
    auto const backBeginIndex =
        static_cast<size_t>(std::distance(inXs.cbegin(), backBegin));
    for (size_t binIndex = 0; binIndex < frontEndIndex; ++binIndex) {
      outputWS->flagMasked(histogramIndex, binIndex);
    }
    for (size_t binIndex = backBeginIndex; binIndex < inXs.size() - 1;
         ++binIndex) {
      outputWS->flagMasked(histogramIndex, binIndex);
    }
    progress.report("Masking nonoverlapping bins");
  }
  setProperty(Prop::OUTPUT_WS, outputWS);
}

} // namespace Algorithms
} // namespace Mantid
