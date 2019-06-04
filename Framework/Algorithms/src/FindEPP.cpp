// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FindEPP.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"


#include <cmath>
#include <sstream>

namespace Mantid {
namespace Algorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindEPP)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string FindEPP::name() const { return "FindEPP"; }

/// Algorithm's version for identification. @see Algorithm::version
int FindEPP::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string FindEPP::category() const {
  return "Workflow\\MLZ\\TOFTOF;Utility";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string FindEPP::summary() const {
  return "Performs Gaussian fits over each spectrum to find the Elastic Peak "
         "Position (EPP).";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void FindEPP::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void FindEPP::exec() {
  m_inWS = getProperty("InputWorkspace");

  initWorkspace();

  int64_t numberspectra = static_cast<int64_t>(m_inWS->getNumberHistograms());

  // Loop over spectra
  PARALLEL_FOR_IF(threadSafe(*m_inWS, *m_outWS))
  for (int64_t index = 0; index < numberspectra; ++index) {
    PARALLEL_START_INTERUPT_REGION
    fitGaussian(index);
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", m_outWS);
}

/* Call Fit as child algorithm for each spectra
 * @param index : the workspace index
 */
void FindEPP::fitGaussian(int64_t index) {
  size_t spectrum = static_cast<size_t>(index);
  m_outWS->cell<int>(spectrum, 0) = static_cast<int>(spectrum);

  const auto &x = m_inWS->x(spectrum).rawData();
  const auto &y = m_inWS->y(spectrum).rawData();
  const auto &e = m_inWS->e(spectrum).rawData();

  // Find the maximum value and it's index
  const auto maxIt = std::max_element(y.begin(), y.end());
  const double height = *maxIt;
  size_t maxIndex = static_cast<size_t>(std::distance(y.begin(), maxIt));

  if (height > 0) {
    // Find how many bins are around maximum, that are above half-maximum
    // Initialize the distances of the half-maxima bins from maximum
    size_t leftHalf = maxIndex, rightHalf = x.size() - maxIndex - 1;

    // Find the first bin on the right side of maximum, that drops below
    // half-maximum
    for (auto it = maxIt; it != y.end(); ++it) {
      if (*it < 0.5 * height) {
        rightHalf = it - maxIt - 1;
        break;
      }
    }

    // Find the first bin on the left side of maximum, that drops below
    // half-maximum
    for (auto it = maxIt; it != y.begin(); --it) {
      if (*it < 0.5 * height) {
        leftHalf = maxIt - it - 1;
        break;
      }
    }
    g_log.debug() << "Peak in spectrum #" << spectrum
                  << " has last bins above 0.5*max at " << leftHalf << "\t"
                  << rightHalf << "\n";

    // We want to fit only if there are at least 3 bins (including the maximum
    // itself) above half-maximum
    if (rightHalf + leftHalf >= 2) {

      // Prepare the initial parameters for the fit
      double fwhm = x[maxIndex + rightHalf] - x[maxIndex - leftHalf];
      double sigma = fwhm / (2. * sqrt(2. * log(2.)));
      double center = x[maxIndex];
      double start = center - 3. * fwhm;
      double end = center + 3. * fwhm;

      std::stringstream function;
      function << "name=Gaussian,PeakCentre=";
      function << center << ",Height=" << height << ",Sigma=" << sigma;

      g_log.debug() << "Fitting spectrum #" << spectrum
                    << " with: " << function.str() << "\n";

      IAlgorithm_sptr fitAlg = createChildAlgorithm("Fit", 0., 0., false);
      fitAlg->setProperty("Function", function.str());
      fitAlg->setProperty("InputWorkspace", m_inWS);
      fitAlg->setProperty("WorkspaceIndex", static_cast<int>(spectrum));
      fitAlg->setProperty("StartX", start);
      fitAlg->setProperty("EndX", end);
      fitAlg->setProperty("CreateOutput", true);
      fitAlg->setProperty("OutputParametersOnly", true);
      fitAlg->executeAsChildAlg();

      const std::string status = fitAlg->getProperty("OutputStatus");
      ITableWorkspace_sptr fitResult = fitAlg->getProperty("OutputParameters");

      if (status == "success") {
        m_outWS->cell<double>(spectrum, 1) = fitResult->cell<double>(1, 1);
        m_outWS->cell<double>(spectrum, 2) = fitResult->cell<double>(1, 2);
        m_outWS->cell<double>(spectrum, 3) = fitResult->cell<double>(2, 1);
        m_outWS->cell<double>(spectrum, 4) = fitResult->cell<double>(2, 2);
        m_outWS->cell<double>(spectrum, 5) = fitResult->cell<double>(0, 1);
        m_outWS->cell<double>(spectrum, 6) = fitResult->cell<double>(0, 2);
        m_outWS->cell<double>(spectrum, 7) = fitResult->cell<double>(3, 1);
        m_outWS->cell<std::string>(spectrum, 8) = status;
      } else {
        g_log.debug() << "Fit failed in spectrum #" << spectrum
                      << ". \nReason :" << status
                      << ". \nSetting the maximum.\n";
        m_outWS->cell<std::string>(spectrum, 8) = "fitFailed";
        m_outWS->cell<double>(spectrum, 1) = x[maxIndex];
        m_outWS->cell<double>(spectrum, 2) = 0.;
        m_outWS->cell<double>(spectrum, 5) = height;
        m_outWS->cell<double>(spectrum, 6) = e[maxIndex];
      }

    } else {
      g_log.information() << "Found <=3 bins above half maximum in spectrum #"
                          << index << ". Not fitting.\n";
      m_outWS->cell<std::string>(spectrum, 8) = "narrowPeak";
      m_outWS->cell<double>(spectrum, 1) = x[maxIndex];
      m_outWS->cell<double>(spectrum, 2) = 0.;
      m_outWS->cell<double>(spectrum, 5) = height;
      m_outWS->cell<double>(spectrum, 6) = e[maxIndex];
    }
  } else {
    g_log.notice() << "Negative maximum in spectrum #" << spectrum
                   << ". Skipping.\n";
    m_outWS->cell<std::string>(spectrum, 8) = "negativeMaximum";
  }
  m_progress->report();
}

/**
 * Initializes the output workspace
 */
void FindEPP::initWorkspace() {

  m_outWS = boost::make_shared<TableWorkspace>();

  const std::vector<std::string> columns = {
      "PeakCentre", "PeakCentreError", "Sigma", "SigmaError",
      "Height",     "HeightError",     "chiSq"};

  m_outWS->addColumn("int", "WorkspaceIndex");
  m_outWS->getColumn(0)->setPlotType(1);
  for (const auto &column : columns) {
    m_outWS->addColumn("double", column);
  }
  m_outWS->addColumn("str", "FitStatus");

  const size_t numberSpectra = m_inWS->getNumberHistograms();
  m_progress = std::make_unique<Progress>(this, 0.0, 1.0, numberSpectra);

  m_outWS->setRowCount(numberSpectra);
}

} // namespace Algorithms
} // namespace Mantid
