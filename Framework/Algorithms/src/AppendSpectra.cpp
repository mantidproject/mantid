// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/AppendSpectra.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/SingletonHolder.h"

using namespace Mantid::Indexing;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace Algorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AppendSpectra)

/// Algorithm's name for identification. @see Algorithm::name
const std::string AppendSpectra::name() const { return "AppendSpectra"; }

/// Algorithm's version for identification. @see Algorithm::version
int AppendSpectra::version() const { return 1; }

/** Initialize the algorithm's properties.
 */
void AppendSpectra::init() {
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace1", "", Direction::Input,
                      boost::make_shared<CommonBinsValidator>()),
                  "The name of the first input workspace");
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace2", "", Direction::Input,
                      boost::make_shared<CommonBinsValidator>()),
                  "The name of the second input workspace");

  declareProperty(
      "ValidateInputs", true,
      "Perform a set of checks that the two input workspaces are compatible.");

  declareProperty("Number", 1,
                  boost::make_shared<BoundedValidator<int>>(1, EMPTY_INT()),
                  "Append the spectra from InputWorkspace2 multiple times.");

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the output workspace");

  declareProperty("MergeLogs", false,
                  "Whether to combine the logs of the two input workspaces");
}

/** Execute the algorithm.
 */
void AppendSpectra::exec() {
  // Retrieve the input workspaces
  MatrixWorkspace_const_sptr ws1 = getProperty("InputWorkspace1");
  MatrixWorkspace_const_sptr ws2 = getProperty("InputWorkspace2");
  DataObjects::EventWorkspace_const_sptr eventWs1 =
      boost::dynamic_pointer_cast<const EventWorkspace>(ws1);
  DataObjects::EventWorkspace_const_sptr eventWs2 =
      boost::dynamic_pointer_cast<const EventWorkspace>(ws2);

  // Make sure that we are not mis-matching EventWorkspaces and other types of
  // workspaces
  if (((eventWs1) && (!eventWs2)) || ((!eventWs1) && (eventWs2))) {
    const std::string message("Only one of the input workspaces are of type "
                              "EventWorkspace; please use matching workspace "
                              "types (both EventWorkspace's or both "
                              "Workspace2D's).");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

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
    DataObjects::EventWorkspace_sptr eOutput =
        this->execEvent(*eventWs1, *eventWs2);
    for (int i = 1; i < number; i++) {
      eOutput = this->execEvent(*eOutput, *eventWs2);
    }
    output = boost::static_pointer_cast<MatrixWorkspace>(eOutput);
  } else { // So it is a workspace 2D.
    // The only restriction, even with ValidateInputs=false
    if (ws1->blocksize() != ws2->blocksize())
      throw std::runtime_error(
          "Workspace2D's must have the same number of bins.");

    output = execWS2D(*ws1, *ws2);
    for (int i = 1; i < number; i++) {
      output = execWS2D(*output, *ws2);
    }
  }

  if (mergeLogs)
    combineLogs(ws1->run(), ws2->run(), output->mutableRun());

  // Set the output workspace
  setProperty("OutputWorkspace",
              boost::dynamic_pointer_cast<MatrixWorkspace>(output));
}

/** If there is an overlap in spectrum numbers between ws1 and ws2,
 * then the spectrum numbers are reset as a simple 1-1 correspondence
 * with the workspace index.
 *
 * @param ws1 The first workspace supplied to the algorithm.
 * @param ws2 The second workspace supplied to the algorithm.
 * @param output The workspace that is going to be returned by the algorithm.
 */
void AppendSpectra::fixSpectrumNumbers(const MatrixWorkspace &ws1,
                                       const MatrixWorkspace &ws2,
                                       MatrixWorkspace &output) {
  specnum_t ws1min;
  specnum_t ws1max;
  getMinMax(ws1, ws1min, ws1max);

  specnum_t ws2min;
  specnum_t ws2max;
  getMinMax(ws2, ws2min, ws2max);

  // is everything possibly ok?
  if (ws2min > ws1max)
    return;

  auto indexInfo = output.indexInfo();
  indexInfo.setSpectrumNumbers(
      0, static_cast<int32_t>(output.getNumberHistograms() - 1));
  output.setIndexInfo(indexInfo);

  const int yAxisNum = 1;
  const auto yAxisWS1 = ws1.getAxis(yAxisNum);
  const auto yAxisWS2 = ws2.getAxis(yAxisNum);
  auto outputYAxis = output.getAxis(yAxisNum);
  const auto ws1len = ws1.getNumberHistograms();

  const bool isTextAxis = yAxisWS1->isText() && yAxisWS2->isText();
  const bool isNumericAxis = yAxisWS1->isNumeric() && yAxisWS2->isNumeric();

  auto outputTextAxis = dynamic_cast<TextAxis *>(outputYAxis);
  for (size_t i = 0; i < output.getNumberHistograms(); ++i) {
    if (isTextAxis) {
      // check if we're outside the spectra of the first workspace
      const std::string inputLabel =
          i < ws1len ? yAxisWS1->label(i) : yAxisWS2->label(i - ws1len);
      outputTextAxis->setLabel(i, !inputLabel.empty() ? inputLabel : "");

    } else if (isNumericAxis) {
      // check if we're outside the spectra of the first workspace
      const double inputVal =
          i < ws1len ? yAxisWS1->getValue(i) : yAxisWS2->getValue(i - ws1len);
      outputYAxis->setValue(i, inputVal);
    }
  }
}

void AppendSpectra::combineLogs(const API::Run &lhs, const API::Run &rhs,
                                API::Run &ans) {
  // No need to worry about ordering here as for Plus - they have to be
  // different workspaces
  if (&lhs != &rhs) {
    ans = lhs;
    ans += rhs;
  }
}

} // namespace Algorithms
} // namespace Mantid
