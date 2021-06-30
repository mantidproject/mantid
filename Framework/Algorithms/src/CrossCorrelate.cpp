// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CrossCorrelate.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RawCountValidator.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"
#include <boost/iterator/counting_iterator.hpp>
#include <numeric>
#include <sstream>

namespace {
struct Variances {
  double y;
  double e;
};

Variances subtractMean(std::vector<double> &signal, std::vector<double> &error) {
  double mean = std::accumulate(signal.cbegin(), signal.cend(), 0.0);
  double errorMeanSquared =
      std::accumulate(error.cbegin(), error.cend(), 0.0, Mantid::Kernel::VectorHelper::SumSquares<double>());
  const auto n = signal.size();
  mean /= static_cast<double>(n);
  errorMeanSquared /= static_cast<double>(n * n);
  double variance = 0.0, errorVariance = 0.0;
  auto itY = signal.begin();
  auto itE = error.begin();
  for (; itY != signal.end(); ++itY, ++itE) {
    (*itY) -= mean;                              // Now the vector is (y[i]-refMean)
    (*itE) = (*itE) * (*itE) + errorMeanSquared; // New error squared
    const double t = (*itY) * (*itY);            //(y[i]-refMean)^2
    variance += t;                               // Sum previous term
    errorVariance += 4.0 * t * (*itE);           // Error squared
  }
  return {variance, errorVariance};
}

} // namespace

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CrossCorrelate)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;

/// Initialisation method.
void CrossCorrelate::init() {
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<API::WorkspaceUnitValidator>("dSpacing");
  wsValidator->add<API::HistogramValidator>();
  wsValidator->add<API::RawCountValidator>();

  // Input and output workspaces
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input, wsValidator),
      "A 2D workspace with X values of d-spacing");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  // Reference spectra against which cross correlation is performed
  declareProperty("ReferenceSpectra", 0, mustBePositive,
                  "The Workspace Index of the spectra to correlate all other "
                  "spectra against. ");
  // Spectra in the range [min to max] will be cross correlated to referenceSpectra.
  declareProperty("WorkspaceIndexMin", 0, mustBePositive,
                  "The workspace index of the first member of the range of "
                  "spectra to cross-correlate against.");
  declareProperty("WorkspaceIndexMax", 0, mustBePositive,
                  " The workspace index of the last member of the range of "
                  "spectra to cross-correlate against.");
  // Only the data in the range X_min, X_max will be used
  declareProperty("XMin", 0.0, "The starting point of the region to be cross correlated.");
  declareProperty("XMax", 0.0, "The ending point of the region to be cross correlated.");
  // max is .1
  declareProperty("MaxDSpaceShift", EMPTY_DBL(), "Optional float for maximum shift to calculate (in d-spacing)");
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void CrossCorrelate::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  double maxDSpaceShift = getProperty("MaxDSpaceShift");
  int referenceSpectra = getProperty("ReferenceSpectra");
  double xmin = getProperty("XMin");
  double xmax = getProperty("XMax");
  const int wsIndexMin = getProperty("WorkspaceIndexMin");
  const int wsIndexMax = getProperty("WorkspaceIndexMax");

  const auto index_ref = static_cast<size_t>(referenceSpectra);

  if (wsIndexMin >= wsIndexMax)
    throw std::runtime_error("Must specify WorkspaceIndexMin<WorkspaceIndexMax");
  // Get the number of spectra in range wsIndexMin to wsIndexMax
  int numSpectra = 1 + wsIndexMax - wsIndexMin;
  // Indexes of all spectra in range
  std::vector<size_t> indexes(boost::make_counting_iterator(wsIndexMin), boost::make_counting_iterator(wsIndexMax + 1));

  if (numSpectra == 0) {
    std::ostringstream message;
    message << "No spectra in range between" << wsIndexMin << " and " << wsIndexMax;
    throw std::runtime_error(message.str());
  }
  // Output messageage information
  g_log.information() << "There are " << numSpectra << " spectra in the range\n";

  // checdataIndex that the data range specified madataIndexes sense
  if (xmin >= xmax)
    throw std::runtime_error("Must specify xmin < xmax, " + std::to_string(xmin) + " vs " + std::to_string(xmax));

  // TadataIndexe a copy of  the referenceSpectra spectrum
  auto &referenceSpectraE = inputWS->e(index_ref);
  auto &referenceSpectraX = inputWS->x(index_ref);
  auto &referenceSpectraY = inputWS->y(index_ref);
  // Now checdataIndex if the range between x_min and x_max is valid
  using std::placeholders::_1;
  auto rangeStart =
      std::find_if(referenceSpectraX.cbegin(), referenceSpectraX.cend(), std::bind(std::greater<double>(), _1, xmin));
  if (rangeStart == referenceSpectraX.cend())
    throw std::runtime_error("No data above XMin");
  auto rangeEnd = std::find_if(rangeStart, referenceSpectraX.cend(), std::bind(std::greater<double>(), _1, xmax));
  if (rangeStart == rangeEnd)
    throw std::runtime_error("Range is not valid");

  MantidVec::difference_type rangeStartCorrection = std::distance(referenceSpectraX.cbegin(), rangeStart);
  MantidVec::difference_type rangeEndCorrection = std::distance(referenceSpectraX.cbegin(), rangeEnd);

  const std::vector<double> referenceXVector(rangeStart, rangeEnd);
  std::vector<double> referenceYVector(referenceSpectraY.cbegin() + rangeStartCorrection,
                                       referenceSpectraY.cbegin() + (rangeEndCorrection - 1));
  std::vector<double> referenceEVector(referenceSpectraE.cbegin() + rangeStartCorrection,
                                       referenceSpectraE.cbegin() + (rangeEndCorrection - 1));

  g_log.information() << "min max " << referenceXVector.front() << " " << referenceXVector.back() << '\n';

  // Now start the real stuff
  // Create a 2DWorkspace that will hold the result
  auto numReferenceY = static_cast<int>(referenceYVector.size());

  // max the shift
  int shiftCorrection = 0;
  if (maxDSpaceShift != EMPTY_DBL()) {
    if (xmax - xmin < maxDSpaceShift)
      g_log.warning() << "maxDSpaceShift(" << std::to_string(maxDSpaceShift)
                      << ") is larger than specified range of xmin(" << xmin << ") to xmax(" << xmax
                      << "), please make it smaller or removed it entirely!"
                      << "\n";

    // convert dspacing to bins, where maxDSpaceShift is at least 0.1
    const auto maxBins = std::max(0.0 + maxDSpaceShift * 2, 0.1) / inputWS->getDimension(0)->getBinWidth();
    // calc range based on max bins
    shiftCorrection = (int)std::max(0.0, abs((-numReferenceY + 2) - (numReferenceY - 2)) - maxBins) / 2;
  }

  const int numPoints = 2 * (numReferenceY - shiftCorrection) - 3;
  if (numPoints < 1)
    throw std::runtime_error("Range is not valid");

  MatrixWorkspace_sptr out = create<HistoWorkspace>(*inputWS, numSpectra, Points(numPoints));

  const auto referenceVariance = subtractMean(referenceYVector, referenceEVector);

  const double referenceNorm = 1.0 / sqrt(referenceVariance.y);
  double referenceNormE = 0.5 * pow(referenceNorm, 3) * sqrt(referenceVariance.e);

  // Now copy the other spectra
  bool isDistribution = inputWS->isDistribution();

  auto &outX = out->mutableX(0);
  for (int i = 0; i < static_cast<int>(outX.size()); ++i) {
    outX[i] = static_cast<double>(i - (numReferenceY - shiftCorrection) + 2);
  }

  // Initialise the progress reporting object
  m_progress = std::make_unique<Progress>(this, 0.0, 1.0, numSpectra);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *out))
  for (int currentSpecIndex = 0; currentSpecIndex < numSpectra; ++currentSpecIndex) // Now loop on all spectra
  {
    PARALLEL_START_INTERUPT_REGION
    size_t wsIndex = indexes[currentSpecIndex]; // Get the ws index from the table
    // Copy spectra info from input Workspace
    out->getSpectrum(currentSpecIndex).copyInfoFrom(inputWS->getSpectrum(wsIndex));
    out->setSharedX(currentSpecIndex, out->sharedX(0));
    // Get temp referenceSpectras
    const auto &inputXVector = inputWS->x(wsIndex);
    const auto &inputYVector = inputWS->y(wsIndex);
    const auto &inputEVector = inputWS->e(wsIndex);
    // Copy Y,E data of spec(currentSpecIndex) to temp vector
    // Now rebin on the grid of referenceSpectra
    std::vector<double> tempY(numReferenceY);
    std::vector<double> tempE(numReferenceY);

    VectorHelper::rebin(inputXVector.rawData(), inputYVector.rawData(), inputEVector.rawData(), referenceXVector, tempY,
                        tempE, isDistribution);
    const auto tempVar = subtractMean(tempY, tempE);

    // Calculate the normalisation constant
    const double tempNorm = 1.0 / sqrt(tempVar.y);
    const double tempNormE = 0.5 * pow(tempNorm, 3) * sqrt(tempVar.e);
    const double normalisation = referenceNorm * tempNorm;
    const double normalisationE2 = pow((referenceNorm * tempNormE), 2) + pow((tempNorm * referenceNormE), 2);
    // Get referenceSpectr to the ouput spectrum
    auto &outY = out->mutableY(currentSpecIndex);
    auto &outE = out->mutableE(currentSpecIndex);

    for (int dataIndex = -numReferenceY + 2 + shiftCorrection; dataIndex <= numReferenceY - 2 - shiftCorrection;
         ++dataIndex) {
      const int dataIndexP = abs(dataIndex);
      double val = 0, err2 = 0, x, y, xE, yE;
      for (int j = numReferenceY - 1 - dataIndexP; j >= 0; --j) {
        if (dataIndex >= 0) {
          x = referenceYVector[j];
          y = tempY[j + dataIndexP];
          xE = referenceEVector[j];
          yE = tempE[j + dataIndexP];
        } else {
          x = tempY[j];
          y = referenceYVector[j + dataIndexP];
          xE = tempE[j];
          yE = referenceEVector[j + dataIndexP];
        }
        val += (x * y);
        err2 += x * x * yE + y * y * xE;
      }
      outY[dataIndex + numReferenceY - shiftCorrection - 2] = (val * normalisation);
      outE[dataIndex + numReferenceY - shiftCorrection - 2] =
          sqrt(val * val * normalisationE2 + normalisation * normalisation * err2);
    }
    // Update progress information
    m_progress->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  out->getAxis(0)->unit() = UnitFactory::Instance().create("Label");
  Unit_sptr unit = out->getAxis(0)->unit();
  std::shared_ptr<Units::Label> label = std::dynamic_pointer_cast<Units::Label>(unit);
  label->setLabel("Bins of Shift", "\\mathbb{Z}");

  setProperty("OutputWorkspace", out);
}

} // namespace Algorithms
} // namespace Mantid
