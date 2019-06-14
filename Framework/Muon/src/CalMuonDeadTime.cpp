// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/CalMuonDeadTime.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PhysicalConstants.h"

#include <cmath>
#include <vector>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace DataObjects;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CalMuonDeadTime)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void CalMuonDeadTime::init() {

  declareProperty(std::make_unique<API::WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the input workspace");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
          "DeadTimeTable", "", Direction::Output),
      "The name of the TableWorkspace in which to store the list "
      "of deadtimes for each spectrum");

  declareProperty("FirstGoodData", 0.5,
                  "The first good data point in units of "
                  "micro-seconds as measured from time "
                  "zero (default to 0.5)",
                  Direction::Input);

  declareProperty("LastGoodData", 5.0,
                  "The last good data point in units of "
                  "micro-seconds as measured from time "
                  "zero (default to 5.0)",
                  Direction::Input);

  declareProperty(std::make_unique<API::WorkspaceProperty<API::Workspace>>(
                      "DataFitted", "", Direction::Output),
                  "The data which the deadtime equation is fitted to");
}

/** Executes the algorithm
 *
 */
void CalMuonDeadTime::exec() {
  // Muon lifetime

  const double muonLifetime = Mantid::PhysicalConstants::MuonLifetime *
                              1e6; // in units of micro-seconds

  // get input properties

  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const double firstgooddata = getProperty("FirstGoodData");
  const double lastgooddata = getProperty("LastGoodData");

  // Seem to have to do this to avoid MantidPlot to crash when
  // running this algorithm where the "DataFitted" WS already exists

  std::string dataFittedName = getPropertyValue("DataFitted");
  if (API::AnalysisDataService::Instance().doesExist(dataFittedName))
    API::AnalysisDataService::Instance().remove(dataFittedName);

  // Get number of good frames from Run object. This also serves as
  // a test to see if valid input workspace has been provided

  const double numGoodFrames = [&inputWS]() {
    const API::Run &run = inputWS->run();
    if (run.hasProperty("goodfrm")) {
      return boost::lexical_cast<double>(run.getProperty("goodfrm")->value());
    } else {
      throw std::runtime_error(
          "To calculate Muon deadtime requires that goodfrm (number of "
          "good frames) is stored in InputWorkspace Run object");
    }
  }();

  // Do the initial setup of the ouput table-workspace

  API::ITableWorkspace_sptr outTable = boost::make_shared<TableWorkspace>();
  outTable->addColumn("int", "spectrum");
  outTable->addColumn("double", "dead-time");

  // Start created a temperary workspace with data we are going to fit
  // against. First step is to crop to only include data between firstgooddata
  // and lastgooddata

  std::string wsName = "TempForMuonCalDeadTime";
  API::IAlgorithm_sptr cropWS;
  cropWS = createChildAlgorithm("CropWorkspace", -1, -1);
  cropWS->setProperty("InputWorkspace", inputWS);
  cropWS->setPropertyValue("OutputWorkspace", "croppedWS");
  cropWS->setProperty("XMin", firstgooddata);
  cropWS->setProperty("XMax", lastgooddata);
  cropWS->executeAsChildAlg();

  // get cropped input workspace

  boost::shared_ptr<API::MatrixWorkspace> wsCrop =
      cropWS->getProperty("OutputWorkspace");

  // next step is to take these data. Create a point workspace
  // which will change the x-axis values to mid-point time values
  // and populate
  // x-axis with measured counts
  // y-axis with measured counts * exp(t/t_mu)

  API::IAlgorithm_sptr convertToPW;
  convertToPW = createChildAlgorithm("ConvertToPointData", -1, -1);
  convertToPW->setProperty("InputWorkspace", wsCrop);
  convertToPW->setPropertyValue("OutputWorkspace", wsName);
  convertToPW->executeAsChildAlg();

  // get pointworkspace

  boost::shared_ptr<API::MatrixWorkspace> wsFitAgainst =
      convertToPW->getProperty("OutputWorkspace");

  const size_t numSpec = wsFitAgainst->getNumberHistograms();
  size_t timechannels = wsFitAgainst->y(0).size();
  for (size_t i = 0; i < numSpec; i++) {
    auto &fitX = wsFitAgainst->mutableX(i);
    auto &fitY = wsFitAgainst->mutableY(i);
    auto &fitE = wsFitAgainst->mutableE(i);
    auto &cFitX = wsFitAgainst->x(i);
    auto &cropY = wsCrop->y(i);
    auto &cropE = wsCrop->e(i);

    for (size_t t = 0; t < timechannels; t++) {
      const double time = cFitX[t]; // mid-point time value because point WS
      const double decayFac = exp(time / muonLifetime);
      if (cropY[t] > 0) {
        fitY[t] = cropY[t] * decayFac;
        fitX[t] = cropY[t];
        fitE[t] = cropE[t] * decayFac;
      } else {
        // For the Muon data which I have looked at when zero counts
        // the errors are zero which is likely nonsense. Hence to get
        // around this problem treat such counts to be 0.1 with standard
        // of one........

        fitY[t] = 0.1 * decayFac;
        fitX[t] = 0.1;
        fitE[t] = decayFac;
      }
    }
  }

  // This property is returned for instrument scientists to
  // play with on the odd occasion

  setProperty("DataFitted", wsFitAgainst);

  // cal deadtime for each spectrum

  for (size_t i = 0; i < numSpec; i++) {
    // Do linear fit

    const double in_bg0 = inputWS->y(i)[0];
    const double in_bg1 = 0.0;

    API::IAlgorithm_sptr fit;
    fit = createChildAlgorithm("Fit", -1, -1, true);

    std::stringstream ss;
    ss << "name=LinearBackground,A0=" << in_bg0 << ",A1=" << in_bg1;
    std::string function = ss.str();

    fit->setPropertyValue("Function", function);
    const int wsindex = static_cast<int>(i);
    fit->setProperty("InputWorkspace", wsFitAgainst);
    fit->setProperty("WorkspaceIndex", wsindex);
    fit->setPropertyValue("Minimizer", "Levenberg-MarquardtMD");

    fit->executeAsChildAlg();

    std::string fitStatus = fit->getProperty("OutputStatus");
    // std::vector<double> params = fit->getProperty("Parameters");
    // std::vector<std::string> paramnames = fit->getProperty("ParameterNames");
    API::IFunction_sptr result = fit->getProperty("Function");

    // Check order of names
    if (result->parameterName(0) != "A0") {
      g_log.error() << "Parameter 0 should be A0, but is "
                    << result->parameterName(0) << '\n';
      throw std::invalid_argument(
          "Parameters are out of order @ 0, should be A0");
    }
    if (result->parameterName(1) != "A1") {
      g_log.error() << "Parameter 1 should be A1, but is "
                    << result->parameterName(1) << '\n';
      throw std::invalid_argument(
          "Parameters are out of order @ 0, should be A1");
    }

    // time bin - assumed constant for histogram
    const double time_bin = inputWS->x(i)[1] - inputWS->x(i)[0];

    if (fitStatus == "success") {
      const double A0 = result->getParameter(0);
      const double A1 = result->getParameter(1);

      // add row to output table
      API::TableRow t = outTable->appendRow();
      t << wsindex + 1 << -(A1 / A0) * time_bin * numGoodFrames;
    } else {
      g_log.warning() << "Fit falled. Status = " << fitStatus
                      << "\nFor workspace index " << i << '\n';
    }
  }

  // finally calculate alpha

  setProperty("DeadTimeTable", outTable);
}

} // namespace Algorithms
} // namespace Mantid
