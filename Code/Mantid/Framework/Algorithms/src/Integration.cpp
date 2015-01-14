//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Integration.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidAPI/Progress.h"
#include <cmath>

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Integration)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

/** Initialisation method.
 *
 */
void Integration::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "The input workspace to integrate.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The output workspace with the results of the integration.");

  declareProperty("RangeLower", EMPTY_DBL(),
                  "The lower integration limit (an X value).");
  declareProperty("RangeUpper", EMPTY_DBL(),
                  "The upper integration limit (an X value).");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive,
                  "Index of the first spectrum to integrate.");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive,
                  "Index of the last spectrum to integrate.");
  declareProperty("IncludePartialBins", false,
                  "If true then partial bins from the beginning and end of the "
                  "input range are also included in the integration.");
}

/**
 * Std-style comparision function object (satisfies the requirements of Compare)
 * @return true if first argument < second argument (with some
 * tolerance/epsilon)
 */
struct tolerant_less : public std::binary_function<double, double, bool> {
public:
  bool operator()(const double &left, const double &right) const {
    // soft equal, if the diff left-right is below a numerical error
    // (uncertainty) threshold, we cannot say
    return (left < right) && (std::abs(left - right) >
                              1 * std::numeric_limits<double>::epsilon());
  }
};

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void Integration::exec() {
  // Try and retrieve the optional properties
  m_MinRange = getProperty("RangeLower");
  m_MaxRange = getProperty("RangeUpper");
  m_MinSpec = getProperty("StartWorkspaceIndex");
  m_MaxSpec = getProperty("EndWorkspaceIndex");
  m_IncPartBins = getProperty("IncludePartialBins");

  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = this->getInputWorkspace();

  const int numberOfSpectra =
      static_cast<int>(localworkspace->getNumberHistograms());

  // Check 'StartSpectrum' is in range 0-numberOfSpectra
  if (m_MinSpec > numberOfSpectra) {
    g_log.warning("StartSpectrum out of range! Set to 0.");
    m_MinSpec = 0;
  }
  if (isEmpty(m_MaxSpec))
    m_MaxSpec = numberOfSpectra - 1;
  if (m_MaxSpec > numberOfSpectra - 1 || m_MaxSpec < m_MinSpec) {
    g_log.warning("EndSpectrum out of range! Set to max detector number");
    m_MaxSpec = numberOfSpectra;
  }
  if (m_MinRange > m_MaxRange) {
    g_log.warning("Range_upper is less than Range_lower. Will integrate up to "
                  "frame maximum.");
    m_MaxRange = 0.0;
  }

  // Create the 2D workspace (with 1 bin) for the output
  MatrixWorkspace_sptr outputWorkspace =
      this->getOutputWorkspace(localworkspace);

  bool is_distrib = outputWorkspace->isDistribution();
  Progress progress(this, 0, 1, m_MaxSpec - m_MinSpec + 1);

  const bool axisIsText = localworkspace->getAxis(1)->isText();
  const bool axisIsNumeric = localworkspace->getAxis(1)->isNumeric();

  // Loop over spectra
  PARALLEL_FOR2(localworkspace, outputWorkspace)
  for (int i = m_MinSpec; i <= m_MaxSpec; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Workspace index on the output
    const int outWI = i - m_MinSpec;

    // Copy Axis values from previous workspace
    if (axisIsText) {
      Mantid::API::TextAxis *newAxis =
          dynamic_cast<Mantid::API::TextAxis *>(outputWorkspace->getAxis(1));
      newAxis->setLabel(outWI, localworkspace->getAxis(1)->label(i));
    } else if (axisIsNumeric) {
      Mantid::API::NumericAxis *newAxis =
          dynamic_cast<Mantid::API::NumericAxis *>(outputWorkspace->getAxis(1));
      newAxis->setValue(outWI, (*(localworkspace->getAxis(1)))(i));
    }

    // This is the output
    ISpectrum *outSpec = outputWorkspace->getSpectrum(outWI);
    // This is the input
    const ISpectrum *inSpec = localworkspace->getSpectrum(i);

    // Copy spectrum number, detector IDs
    outSpec->copyInfoFrom(*inSpec);

    // Retrieve the spectrum into a vector
    const MantidVec &X = inSpec->readX();
    const MantidVec &Y = inSpec->readY();
    const MantidVec &E = inSpec->readE();

    // If doing partial bins, we want to set the bin boundaries to the specified
    // values
    // regardless of whether they're 'in range' for this spectrum
    // Have to do this here, ahead of the 'continue' a bit down from here.
    if (m_IncPartBins) {
      outSpec->dataX()[0] = m_MinRange;
      outSpec->dataX()[1] = m_MaxRange;
    }

    // Find the range [min,max]
    MantidVec::const_iterator lowit, highit;
    if (m_MinRange == EMPTY_DBL()) {
      lowit = X.begin();
    } else {
      lowit = std::lower_bound(X.begin(), X.end(), m_MinRange, tolerant_less());
    }

    if (m_MaxRange == EMPTY_DBL()) {
      highit = X.end();
    } else {
      highit = std::upper_bound(lowit, X.end(), m_MaxRange, tolerant_less());
    }

    // If range specified doesn't overlap with this spectrum then bail out
    if (lowit == X.end() || highit == X.begin())
      continue;

    // Upper limit is the bin before, i.e. the last value smaller than MaxRange
    --highit; // (note: decrementing 'end()' is safe for vectors, at least
              // according to the C++ standard)

    MantidVec::difference_type distmin = std::distance(X.begin(), lowit);
    MantidVec::difference_type distmax = std::distance(X.begin(), highit);

    double sumY = 0.0;
    double sumE = 0.0;
    if (distmax <= distmin) {
      sumY = 0.;
      sumE = 0.;
    } else {
      if (!is_distrib) {
        // Sum the Y, and sum the E in quadrature
        {
          sumY = std::accumulate(Y.begin() + distmin, Y.begin() + distmax, 0.0);
          sumE = std::accumulate(E.begin() + distmin, E.begin() + distmax, 0.0,
                                 VectorHelper::SumSquares<double>());
        }
      } else {
        // Sum Y*binwidth and Sum the (E*binwidth)^2.
        std::vector<double> widths(X.size());
        // highit+1 is safe while input workspace guaranteed to be histogram
        std::adjacent_difference(lowit, highit + 1, widths.begin());
        sumY = std::inner_product(Y.begin() + distmin, Y.begin() + distmax,
                                  widths.begin() + 1, 0.0);
        sumE = std::inner_product(E.begin() + distmin, E.begin() + distmax,
                                  widths.begin() + 1, 0.0, std::plus<double>(),
                                  VectorHelper::TimesSquares<double>());
      }
    }
    // If partial bins are included, set integration range to exact range
    // given and add on contributions from partial bins either side of range.
    if (m_IncPartBins) {
      if (distmin > 0) {
        const double lower_bin = *lowit;
        const double prev_bin = *(lowit - 1);
        double fraction = (lower_bin - m_MinRange);
        if (!is_distrib) {
          fraction /= (lower_bin - prev_bin);
        }
        const MantidVec::size_type val_index = distmin - 1;
        sumY += Y[val_index] * fraction;
        const double eval = E[val_index];
        sumE += eval * eval * fraction * fraction;
      }
      if (highit < X.end() - 1) {
        const double upper_bin = *highit;
        const double next_bin = *(highit + 1);
        double fraction = (m_MaxRange - upper_bin);
        if (!is_distrib) {
          fraction /= (next_bin - upper_bin);
        }
        sumY += Y[distmax] * fraction;
        const double eval = E[distmax];
        sumE += eval * eval * fraction * fraction;
      }
    } else {
      outSpec->dataX()[0] = lowit == X.end() ? *(lowit - 1) : *(lowit);
      outSpec->dataX()[1] = *highit;
    }

    outSpec->dataY()[0] = sumY;
    outSpec->dataE()[0] = sqrt(sumE); // Propagate Gaussian error

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputWorkspace);

  return;
}

/**
 * This function gets the input workspace. In the case for a RebinnedOutput
 * workspace, it must be cleaned before proceeding. Other workspaces are
 * untouched.
 * @return the input workspace, cleaned if necessary
 */
MatrixWorkspace_const_sptr Integration::getInputWorkspace() {
  MatrixWorkspace_sptr temp = getProperty("InputWorkspace");

  if (temp->id() == "RebinnedOutput") {
    // Clean the input workspace in the RebinnedOutput case for nan's and
    // inf's in order to treat the data correctly later.
    IAlgorithm_sptr alg = this->createChildAlgorithm("ReplaceSpecialValues");
    alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", temp);
    std::string outName = "_" + temp->getName() + "_clean";
    alg->setProperty("OutputWorkspace", outName);
    alg->setProperty("NaNValue", 0.0);
    alg->setProperty("NaNError", 0.0);
    alg->setProperty("InfinityValue", 0.0);
    alg->setProperty("InfinityError", 0.0);
    alg->executeAsChildAlg();
    temp = alg->getProperty("OutputWorkspace");
  }

  // To integrate point data it will be converted to histograms
  if ( !temp->isHistogramData() )
  {
    auto alg = this->createChildAlgorithm("ConvertToHistogram");
    alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", temp);
    std::string outName = "_" + temp->getName() + "_histogram";
    alg->setProperty("OutputWorkspace", outName);
    alg->executeAsChildAlg();
    temp = alg->getProperty("OutputWorkspace");
    temp->isDistribution(true);
  }

  return temp;
}

/**
 * This function creates the output workspace. In the case of a RebinnedOutput
 * workspace, the resulting workspace only needs to be a Workspace2D to handle
 * the integration. Other workspaces are handled normally.
 * @return the output workspace
 */
MatrixWorkspace_sptr
Integration::getOutputWorkspace(MatrixWorkspace_const_sptr inWS) {
  if (inWS->id() == "RebinnedOutput") {
    MatrixWorkspace_sptr outWS = API::WorkspaceFactory::Instance().create(
        "Workspace2D", m_MaxSpec - m_MinSpec + 1, 2, 1);
    API::WorkspaceFactory::Instance().initializeFromParent(inWS, outWS, true);
    return outWS;
  } else {
    return API::WorkspaceFactory::Instance().create(
        inWS, m_MaxSpec - m_MinSpec + 1, 2, 1);
  }
}

} // namespace Algorithms
} // namespace Mantid
