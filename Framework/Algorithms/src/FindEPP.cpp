#include "MantidAlgorithms/FindEPP.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <cmath>
#include <sstream>

namespace Mantid {
namespace Algorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindEPP)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string FindEPP::name() const { return "FindEPP"; }

/// Algorithm's version for identification. @see Algorithm::version
int FindEPP::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string FindEPP::category() const { return "Utility"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string FindEPP::summary() const {
  return "Performs Gaussian fit of all spectra to find the elastic peak "
         "position.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void FindEPP::init() {
  declareProperty(Kernel::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace.");
  declareProperty(Kernel::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void FindEPP::exec() {
  m_inWS = getProperty("InputWorkspace");
  m_outWS = WorkspaceFactory::Instance().createTable("TableWorkspace");

  const std::vector<std::string> columns = {
      "PeakCentre", "PeakCentreError", "Sigma", "SigmaError",
      "Height",     "HeightError",     "chiSq"};

  m_outWS->addColumn("size_t", "WorkspaceIndex");
  for (const auto &column : columns) {
    m_outWS->addColumn("double", column);
  }
  m_outWS->addColumn("str", "FitStatus");

  const size_t numberSpectra = m_inWS->getNumberHistograms();
  m_progress = std::make_unique<Progress>(this, 0, 1, numberSpectra);

  for (size_t i = 0; i < numberSpectra; ++i) {
    m_outWS->appendRow();
  }

  // Loop over spectra
  PARALLEL_FOR_IF(threadSafe(*m_inWS, *m_outWS))
  for (size_t index = 0; index < numberSpectra; ++index) {
    PARALLEL_START_INTERUPT_REGION
    fitGaussian(index);
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", m_outWS);
}

/* Call Fit as child algorithm for each spectra
 * @param index : the input workspace index
 */
void FindEPP::fitGaussian(size_t index) {
  const auto &x = m_inWS->x(index).rawData();
  const auto &y = m_inWS->y(index).rawData();
  const auto &e = m_inWS->e(index).rawData();

  const auto maxIt = std::max_element(y.begin(), y.end());
  const double height = *maxIt;
  size_t maxIndex = static_cast<size_t>(std::distance(y.begin(), maxIt));

  m_outWS->cell<size_t>(index, 0) = index;

  if (height > 0) {
    size_t leftHalf = maxIndex, rightHalf = x.size() - maxIndex - 1;
    for (auto it = maxIt; it != y.end(); ++it) {
      if (*it < 0.5 * height) {
        rightHalf = it - maxIt;
        break;
      }
    }
    for (auto it = maxIt; it != y.begin(); --it) {
      if (*it < 0.5 * height) {
        leftHalf = maxIt - it;
        break;
      }
    }

    if (rightHalf + leftHalf > 3) {

      double fwhm = x[maxIndex + rightHalf] - x[maxIndex - leftHalf];
      double sigma = fwhm / (2. * sqrt(2. * log(2.)));
      double center = x[maxIndex];
      double start = center - 3. * fwhm;
      double end = center + 3. * fwhm;

      std::stringstream function;
      function << "name=Gaussian,PeakCentre=";
      function << center << ",Height=" << height << ",Sigma=" << sigma;

      IAlgorithm_sptr fitAlg = createChildAlgorithm("Fit", 0., 0., false);
      fitAlg->setProperty("Function", function.str());
      fitAlg->setProperty("InputWorkspace", m_inWS);
      fitAlg->setProperty("WorkspaceIndex", static_cast<int>(index));
      fitAlg->setProperty("StartX", start);
      fitAlg->setProperty("EndX", end);
      fitAlg->setProperty("CreateOutput", true);
      fitAlg->setProperty("OutputParametersOnly", true);
      fitAlg->executeAsChildAlg();

      const std::string status = fitAlg->getProperty("OutputStatus");
      ITableWorkspace_sptr fitResult = fitAlg->getProperty("OutputParameters");

      m_outWS->cell<double>(index, 1) = fitResult->cell<double>(1, 1);
      m_outWS->cell<double>(index, 2) = fitResult->cell<double>(1, 2);
      m_outWS->cell<double>(index, 3) = fitResult->cell<double>(2, 1);
      m_outWS->cell<double>(index, 4) = fitResult->cell<double>(2, 2);
      m_outWS->cell<double>(index, 5) = fitResult->cell<double>(0, 1);
      m_outWS->cell<double>(index, 6) = fitResult->cell<double>(0, 2);
      m_outWS->cell<double>(index, 7) = fitResult->cell<double>(3, 1);
      m_outWS->cell<std::string>(index, 8) = status;

    } else {
      g_log.warning() << "Found <=3 bins above half maximum in spectrum #"
                      << index << ". Not fitting.\n";
      m_outWS->cell<double>(index, 1) = x[maxIndex];
      m_outWS->cell<double>(index, 2) = 0.;
      m_outWS->cell<double>(index, 5) = height;
      m_outWS->cell<double>(index, 6) = e[maxIndex];
      m_outWS->cell<std::string>(index, 8) = "NarrowPeak";
    }
  } else {
    g_log.warning() << "Negative maximum in spectrum #" << index
                    << ". Skipping.\n";
    m_outWS->cell<std::string>(index, 8) = "NegativeMaximum";
  }
  m_progress->report();
}

} // namespace Algorithms
} // namespace Mantid
