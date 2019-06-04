// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Transpose.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidDataObjects/RebinnedOutput.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Transpose)

using namespace Kernel;
using namespace API;

void Transpose::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<CommonBinsValidator>()),
                  "The input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The output workspace.");
}

void Transpose::exec() {
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWorkspace = createOutputWorkspace(inputWorkspace);

  // Things to take care of RebinnedOutput workspaces
  DataObjects::RebinnedOutput_const_sptr inRebinWorkspace =
      boost::dynamic_pointer_cast<const DataObjects::RebinnedOutput>(
          inputWorkspace);
  DataObjects::RebinnedOutput_sptr outRebinWorkspace =
      boost::dynamic_pointer_cast<DataObjects::RebinnedOutput>(outputWorkspace);

  size_t newNhist = outputWorkspace->getNumberHistograms();
  size_t newXsize = outputWorkspace->x(0).size();
  size_t newYsize = outputWorkspace->blocksize();

  // Create a shareable X vector for the output workspace
  Axis *inputYAxis = getVerticalAxis(inputWorkspace);
  std::vector<double> newXValues(newXsize);
  for (size_t i = 0; i < newXsize; ++i) {
    newXValues[i] = (*inputYAxis)(i);
  }
  auto newXVector =
      Kernel::make_cow<HistogramData::HistogramX>(std::move(newXValues));

  Progress progress(this, 0.0, 1.0, newNhist * newYsize);
  progress.report("Swapping data values");
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWorkspace, *outputWorkspace))
  for (int64_t i = 0; i < static_cast<int64_t>(newNhist); ++i) {
    PARALLEL_START_INTERUPT_REGION

    outputWorkspace->setSharedX(i, newXVector);

    auto &outY = outputWorkspace->mutableY(i);
    auto &outE = outputWorkspace->mutableE(i);

    // because setF wants a COW pointer
    Kernel::cow_ptr<std::vector<double>> F;
    std::vector<double> &outF = F.access();

    if (outRebinWorkspace) {
      outF.resize(newYsize);
    }

    for (int64_t j = 0; j < int64_t(newYsize); ++j) {
      progress.report();
      outY[j] = inputWorkspace->y(j)[i];
      outE[j] = inputWorkspace->e(j)[i];
      if (outRebinWorkspace) {
        outF[j] = inRebinWorkspace->dataF(j)[i];
      }
    }
    if (outRebinWorkspace) {
      outRebinWorkspace->setF(i, F);
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

/**
 * Creates the output workspace for this algorithm
 * @param inputWorkspace A parent workspace to initialize from.
 * @return A pointer to the output workspace.
 */
API::MatrixWorkspace_sptr Transpose::createOutputWorkspace(
    API::MatrixWorkspace_const_sptr inputWorkspace) {
  Mantid::API::Axis *yAxis = getVerticalAxis(inputWorkspace);
  const size_t oldNhist = inputWorkspace->getNumberHistograms();
  const auto &inX = inputWorkspace->x(0);
  const size_t oldYlength = inputWorkspace->blocksize();
  const size_t oldVerticalAxislength = yAxis->length();

  // The input Y axis may be binned so the new X data should be too
  size_t newNhist(oldYlength), newXsize(oldVerticalAxislength),
      newYsize(oldNhist);
  MatrixWorkspace_sptr outputWorkspace = inputWorkspace->cloneEmpty();
  outputWorkspace->initialize(newNhist, newXsize, newYsize);
  outputWorkspace->setTitle(inputWorkspace->getTitle());
  outputWorkspace->setComment(inputWorkspace->getComment());
  outputWorkspace->copyExperimentInfoFrom(inputWorkspace.get());
  outputWorkspace->setYUnit(inputWorkspace->YUnit());
  outputWorkspace->setYUnitLabel(inputWorkspace->YUnitLabel());
  outputWorkspace->setDistribution(inputWorkspace->isDistribution());

  // Create a new numeric axis for Y the same length as the old X array
  // Values come from input X
  API::NumericAxis *newYAxis(nullptr);
  if (inputWorkspace->isHistogramData()) {
    newYAxis = new API::BinEdgeAxis(inX.rawData());
  } else {
    newYAxis = new API::NumericAxis(inX.rawData());
  }

  newYAxis->unit() = inputWorkspace->getAxis(0)->unit();
  outputWorkspace->getAxis(0)->unit() = inputWorkspace->getAxis(1)->unit();
  outputWorkspace->replaceAxis(1, newYAxis);
  setProperty("OutputWorkspace", outputWorkspace);
  return outputWorkspace;
}

/**
 * Returns the input vertical
 * @param workspace :: A pointer to a workspace
 * @return An axis pointer for the vertical axis of the input workspace
 */
API::Axis *
Transpose::getVerticalAxis(API::MatrixWorkspace_const_sptr workspace) const {
  API::Axis *yAxis;
  try {
    yAxis = workspace->getAxis(1);
  } catch (Kernel::Exception::IndexError &) {
    throw std::invalid_argument("Axis(1) not found on input workspace.");
  }
  // Can't put text in dataX
  if (yAxis->isText()) {
    throw std::invalid_argument(
        "Axis(1) is a text axis. Transpose is unable to cope with text axes.");
  }
  return yAxis;
}

} // namespace Algorithms
} // namespace Mantid
