// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ExtractSpectra2.h"
#include "MantidAPI/Algorithm.hxx"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidIndexing/IndexInfo.h"

namespace Mantid {
using namespace API;
using namespace DataObjects;
using namespace Kernel;
namespace Algorithms {

// Currently we DO NOT REGISTER the algorithm into the AlgorithmFactory. The API
// is different from version 1 and thus cannot replace it without breaking
// scripts. It can be used internally directly without being registered.

/// Algorithms name for identification. @see Algorithm::name
const std::string ExtractSpectra2::name() const { return "ExtractSpectra2"; }

/// Algorithm's version for identification. @see Algorithm::version
int ExtractSpectra2::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ExtractSpectra2::category() const { return "Transforms\\Splitting"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ExtractSpectra2::summary() const {
  return "Extracts a list of spectra from a workspace and places them in a new "
         "workspace.";
}

/// Initialize the algorithm's properties.
void ExtractSpectra2::init() {
  declareWorkspaceInputProperties<MatrixWorkspace, static_cast<int>(IndexType::SpectrumNum) |
                                                       static_cast<int>(IndexType::WorkspaceIndex)>(
      "InputWorkspace", "The input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace");
}

/// Executes the algorithm
void ExtractSpectra2::exec() {
  std::shared_ptr<MatrixWorkspace> inputWS;
  Indexing::SpectrumIndexSet indexSet;
  std::tie(inputWS, indexSet) = getWorkspaceAndIndices<MatrixWorkspace>("InputWorkspace");

  auto outputWS = create<MatrixWorkspace>(
      *inputWS, dynamic_cast<IndexProperty *>(getPointerToProperty("InputWorkspaceIndexSet"))->getFilteredIndexInfo(),
      HistogramData::BinEdges(2));

  Axis *inAxis1(nullptr);
  TextAxis *outTxtAxis(nullptr);
  NumericAxis *outNumAxis(nullptr);
  bool isBinEdgeAxis(false);
  if (inputWS->axes() > 1) {
    inAxis1 = inputWS->getAxis(1);
    auto outAxis1 = outputWS->getAxis(1);
    outTxtAxis = dynamic_cast<TextAxis *>(outAxis1);
    if (!outTxtAxis) {
      outNumAxis = dynamic_cast<NumericAxis *>(outAxis1);
      isBinEdgeAxis = dynamic_cast<BinEdgeAxis *>(inAxis1) != nullptr;
    }
  }

  Progress prog(this, 0.0, 1.0, indexSet.size());
  for (size_t j = 0; j < indexSet.size(); ++j) {
    // Rely on Indexing::IndexSet preserving index order.
    const size_t i = indexSet[j];
    // Copy spectrum data, automatically setting up sharing for histogram.
    outputWS->getSpectrum(j).copyDataFrom(inputWS->getSpectrum(i));

    // Copy axis entry, SpectraAxis is implicit in workspace creation
    if (outTxtAxis)
      outTxtAxis->setLabel(j, inAxis1->label(i));
    else if (outNumAxis)
      outNumAxis->setValue(j, inAxis1->operator()(i));

    // Copy bin masking if it exists.
    if (inputWS->hasMaskedBins(i))
      outputWS->setMaskedBins(j, inputWS->maskedBins(i));

    prog.report();
  }

  if (isBinEdgeAxis) {
    if (!indexSet.isContiguous()) {
      throw std::invalid_argument("Cannot extract non-contiguous set of "
                                  "spectra when the vertical axis has bin "
                                  "edges.");
    }
    const auto outIndex = indexSet.size();
    const auto inIndex = indexSet[indexSet.size() - 1] + 1;
    outNumAxis->setValue(outIndex, inAxis1->operator()(inIndex));
  }

  setProperty("OutputWorkspace", std::move(outputWS));
}

} // namespace Algorithms
} // namespace Mantid
