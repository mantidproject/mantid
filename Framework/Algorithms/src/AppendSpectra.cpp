// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAlgorithms/AppendSpectra.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/SingletonHolder.h"

using namespace Mantid::Indexing;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace {
std::vector<Mantid::detid_t> getDetectorsSkippingMonitors(const Mantid::Geometry::DetectorInfo &detectorInfo) {
  const auto &pixelIDs = detectorInfo.detectorIDs();
  std::vector<Mantid::detid_t> nonMonitorDetectors;
  nonMonitorDetectors.reserve(pixelIDs.size());
  // getting only non monitors in
  for (Mantid::detid_t detIdx = 0; std::cmp_less(detIdx, pixelIDs.size()); detIdx++) {
    if (!detectorInfo.isMonitor(detIdx)) {
      nonMonitorDetectors.push_back(pixelIDs.at(detIdx));
    }
  }
  return nonMonitorDetectors;
}
} // namespace

namespace Mantid::Algorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AppendSpectra)

/// Algorithm's name for identification. @see Algorithm::name
const std::string AppendSpectra::name() const { return "AppendSpectra"; }

/// Algorithm's version for identification. @see Algorithm::version
int AppendSpectra::version() const { return 1; }

/** Initialize the algorithm's properties.
 */
void AppendSpectra::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace1", "", Direction::Input,
                                                        std::make_shared<CommonBinsValidator>()),
                  "The name of the first input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace2", "", Direction::Input,
                                                        std::make_shared<CommonBinsValidator>()),
                  "The name of the second input workspace");

  declareProperty("ValidateInputs", true, "Perform a set of checks that the two input workspaces are compatible.");
  declareProperty("Number", 1, std::make_shared<BoundedValidator<int>>(1, EMPTY_INT()),
                  "Append the spectra from InputWorkspace2 multiple times.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace");

  declareProperty("MergeLogs", false, "Whether to combine the logs of the two input workspaces");
  declareProperty("AppendYAxisLabels", false,
                  "Whether to append y axis labels; this is done automatically if there is spectra overlap");
  declareProperty("RewriteSpectraMap", false,
                  "Rewrites the detectorID associated with each spectrum, moving each detector to the position on the "
                  "original workspace");
}

std::map<std::string, std::string> AppendSpectra::validateInputs() {
  std::map<std::string, std::string> inputs;
  const MatrixWorkspace_const_sptr ws1 = getProperty("InputWorkspace1");
  const MatrixWorkspace_const_sptr ws2 = getProperty("InputWorkspace2");
  const EventWorkspace_const_sptr eventWs1 = std::dynamic_pointer_cast<const EventWorkspace>(ws1);
  const EventWorkspace_const_sptr eventWs2 = std::dynamic_pointer_cast<const EventWorkspace>(ws2);

  // Make sure that we are not mis-matching EventWorkspaces and other types of workspaces
  if (((eventWs1) && (!eventWs2)) || ((!eventWs1) && (eventWs2))) {
    const std::string msg = "Only one of the input workspaces are of type "
                            "EventWorkspace; please use matching workspace "
                            "types (both EventWorkspace or both "
                            "Workspace2D). ";
    inputs["InputWorkspace1"] = msg;
    inputs["InputWorkspace2"] = msg;
  }
  if (!isDefault("RewriteSpectraMap")) {
    if (!isDefault("Number")) {
      inputs["RewriteSpectraMap"] += "Rewrite spectra only implemented when  number is  1. ";
    }
    if (const auto &inst1Name = ws1->getInstrumentName(), inst2Name = ws2->getInstrumentName();
        inst1Name.empty() || inst2Name.empty() || (inst1Name != inst2Name)) {
      inputs["RewriteSpectraMap"] += "Both input workspace must have valid instruments to rewrite the spectra maps.";
    } else {
      const auto countNonMonitorSpectra = [](const MatrixWorkspace_const_sptr &workspace) {
        const auto &spectrumInfo = workspace->spectrumInfo();
        size_t count = 0;
        for (size_t index = 0; index < workspace->getNumberHistograms(); ++index) {
          if (!spectrumInfo.isMonitor(index))
            ++count;
        }
        return count;
      };
      if (ws1->getInstrument()->getNumberDetectors(true) < countNonMonitorSpectra(ws1) + countNonMonitorSpectra(ws2)) {
        inputs["RewriteSpectraMap"] +=
            "There are less available detectors than total number of spectrum in the appended workspace. "
            "Not possible to rewrite their positions.";
      }
    }
  }
  return inputs;
}

/** Execute the algorithm.
 */
void AppendSpectra::exec() {
  // Retrieve the input workspaces
  MatrixWorkspace_const_sptr ws1 = getProperty("InputWorkspace1");
  MatrixWorkspace_const_sptr ws2 = getProperty("InputWorkspace2");
  DataObjects::EventWorkspace_const_sptr eventWs1 = std::dynamic_pointer_cast<const EventWorkspace>(ws1);
  DataObjects::EventWorkspace_const_sptr eventWs2 = std::dynamic_pointer_cast<const EventWorkspace>(ws2);

  bool ValidateInputs = this->getProperty("ValidateInputs");
  if (ValidateInputs) {
    // Check that the input workspaces meet the requirements for this algorithm
    this->checkCompatibility(*ws1, *ws2);
  }

  const bool mergeLogs = getProperty("MergeLogs");
  const int number = getProperty("Number");
  MatrixWorkspace_sptr output;

  if (eventWs1 && eventWs2) {
    // Both are event workspaces. Use the special method
    DataObjects::EventWorkspace_sptr eOutput = this->execEvent(*eventWs1, *eventWs2);
    for (int i = 1; i < number; i++) {
      eOutput = this->execEvent(*eOutput, *eventWs2);
    }
    output = std::static_pointer_cast<MatrixWorkspace>(eOutput);
  } else { // So it is a workspace 2D.
    // The only restriction, even with ValidateInputs=false
    if (ws1->blocksize() != ws2->blocksize())
      throw std::runtime_error("Workspace2D's must have the same number of bins.");

    output = execWS2D(*ws1, *ws2);
    for (int i = 1; i < number; i++) {
      output = execWS2D(*output, *ws2);
    }
  }
  if (!isDefault("RewriteSpectraMap")) {
    rewriteSpectraMap(ws1, ws2, output);
  }

  if (mergeLogs)
    combineLogs(ws1->run(), ws2->run(), output->mutableRun());

  // Set the output workspace
  setProperty("OutputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(output));
}

/** If there is an overlap in spectrum numbers between ws1 and ws2 or the y axis is a bin edges axis,
 * then the spectrum numbers are reset as a simple 1-1 correspondence
 * with the workspace index.
 *
 * @param ws1 The first workspace supplied to the algorithm.
 * @param ws2 The second workspace supplied to the algorithm.
 * @param output The workspace that is going to be returned by the algorithm.
 */
void AppendSpectra::fixSpectrumNumbers(const MatrixWorkspace &ws1, const MatrixWorkspace &ws2,
                                       MatrixWorkspace &output) {
  specnum_t ws1min;
  specnum_t ws1max;
  getMinMax(ws1, ws1min, ws1max);

  specnum_t ws2min;
  specnum_t ws2max;
  getMinMax(ws2, ws2min, ws2max);

  const bool appendYAxis = getProperty("AppendYAxisLabels");
  if (ws2min <= ws1max) {
    auto indexInfo = output.indexInfo();
    indexInfo.setSpectrumNumbers(0, static_cast<int32_t>(output.getNumberHistograms() - 1));
    output.setIndexInfo(indexInfo);

    appendYAxisLabels(ws1, ws2, output);
  } else if (appendYAxis) {
    appendYAxisLabels(ws1, ws2, output);
  }
}

void AppendSpectra::appendYAxisLabels(const MatrixWorkspace &ws1, const MatrixWorkspace &ws2,
                                      const MatrixWorkspace &output) {
  const auto yAxisNum = 1;
  const auto yAxisWS1 = ws1.getAxis(yAxisNum);
  const auto yAxisWS2 = ws2.getAxis(yAxisNum);
  const auto outputYAxis = output.getAxis(yAxisNum);
  const auto ws1Len = ws1.getNumberHistograms();

  const bool isSpectra = yAxisWS1->isSpectra() && yAxisWS2->isSpectra();
  const bool isTextAxis = yAxisWS1->isText() && yAxisWS2->isText();
  const bool isNumericAxis = yAxisWS1->isNumeric() && yAxisWS2->isNumeric();

  if (!isSpectra && !isTextAxis && !isNumericAxis) {
    const std::string message(
        "Y-Axis type mismatch. Ensure that the Y-axis types in both workspaces match. The Y-axis should be set to the "
        "same type in each workspace, whether it be Numeric, Spectra, or Text.");

    throw std::invalid_argument(message);
  }

  bool isBinEdgeAxis =
      dynamic_cast<BinEdgeAxis *>(yAxisWS1) != nullptr && dynamic_cast<BinEdgeAxis *>(yAxisWS2) != nullptr;
  auto outputTextAxis = dynamic_cast<TextAxis *>(outputYAxis);
  const auto outputLen = output.getNumberHistograms() + (isBinEdgeAxis ? 1 : 0);
  for (size_t i = 0; i < outputLen; ++i) {
    if (isTextAxis) {
      // check if we're outside the spectra of the first workspace
      const std::string inputLabel = i < ws1Len ? yAxisWS1->label(i) : yAxisWS2->label(i - ws1Len);
      outputTextAxis->setLabel(i, !inputLabel.empty() ? inputLabel : "");

    } else if (isNumericAxis) {
      // check if we're outside the spectra of the first workspace
      const double inputVal = i < ws1Len ? yAxisWS1->getValue(i) : yAxisWS2->getValue(i - ws1Len);
      outputYAxis->setValue(i, inputVal);
    }
  }
}

void AppendSpectra::combineLogs(const API::Run &lhs, const API::Run &rhs, API::Run &ans) {
  // No need to worry about ordering here as for Plus - they have to be
  // different workspaces
  if (&lhs != &rhs) {
    ans = lhs;
    ans += rhs;
  }
}

void AppendSpectra::rewriteSpectraMap(const MatrixWorkspace_const_sptr &ws1, const MatrixWorkspace_const_sptr &ws2,
                                      const MatrixWorkspace_sptr &output) {
  const size_t lenWs1 = ws1->getNumberHistograms();
  const size_t totLength = lenWs1 + ws2->getNumberHistograms();
  const auto &detectorInfo = output->detectorInfo();
  const auto &pixelIDs = getDetectorsSkippingMonitors(detectorInfo);

  auto &componentInfo = output->mutableComponentInfo();
  const auto &spInfo1 = ws1->spectrumInfo();
  const auto &spInfo2 = ws2->spectrumInfo();
  size_t detIndex = 0;
  for (size_t i = 0; i < totLength; i++) {
    if (i < lenWs1 ? spInfo1.isMonitor(i) : spInfo2.isMonitor(i - lenWs1)) {
      continue;
    }
    const auto pos = i < lenWs1 ? spInfo1.position(i) : spInfo2.position(i - lenWs1);
    const auto detID = pixelIDs.at(detIndex);
    // should be the same as the component
    const auto &detIDidx = detectorInfo.indexOf(detID);
    auto &sp = output->getSpectrum(i);
    // Do the move
    componentInfo.setPosition(detIDidx, pos);
    sp.setDetectorID(detID);
    detIndex++;
  }
}
} // namespace Mantid::Algorithms
