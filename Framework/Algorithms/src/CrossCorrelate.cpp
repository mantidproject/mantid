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
#include "MantidAPI/HistoWorkspace.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RawCountValidator.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"
#include <boost/iterator/counting_iterator.hpp>
#include <numeric>

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

namespace Mantid::Algorithms {

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
  // Alternatively to min and max index, a list of indices can be supplied
  declareProperty(std::make_unique<ArrayProperty<size_t>>("WorkspaceIndexList"),
                  "A comma-separated list of individual workspace indices of "
                  "spectra to cross-correlate against.");
  // Only the data in the range X_min, X_max will be used
  declareProperty("XMin", 0.0, "The starting point of the region to be cross correlated.");
  declareProperty("XMax", 0.0, "The ending point of the region to be cross correlated.");
  // max is .1
  declareProperty("MaxDSpaceShift", EMPTY_DBL(), "Optional float for maximum shift to calculate (in d-spacing)");
}

/// Validate that input properties are sane
std::map<std::string, std::string> CrossCorrelate::validateInputs() {
  std::map<std::string, std::string> helpMessages;

  // Unless a list was specified, check that workspace index min and max make sense
  if (isDefault("WorkspaceIndexList")) {
    const int wsIndexMin = getProperty("WorkspaceIndexMin");
    const int wsIndexMax = getProperty("WorkspaceIndexMax");
    if (wsIndexMin >= wsIndexMax) {
      helpMessages["WorkspaceIndexMin"] = "Must specify WorkspaceIndexMin < WorkspaceIndexMax";
      helpMessages["WorkspaceIndexMax"] = "Must specify WorkspaceIndexMin < WorkspaceIndexMax";
    }
  }

  // Valid input is either min and max workspace index OR list but not both
  if (!isDefault("WorkspaceIndexList") && (!isDefault("WorkspaceIndexMin") || !isDefault("WorkspaceIndexMax"))) {
    const std::string msg = "Must specify either WorkspaceIndexMin and WorkspaceIndexMax, "
                            "or WorkspaceIndexList, but not both.";
    helpMessages["WorkspaceIndexMin"] = msg;
    helpMessages["WorkspaceIndexMax"] = msg;
    helpMessages["WorkspaceIndexList"] = msg;
  }

  // Check that the data range specified makes sense
  const double xmin = getProperty("XMin");
  const double xmax = getProperty("XMax");
  if (xmin >= xmax) {
    helpMessages["XMin"] = "Must specify XMin < XMax";
    helpMessages["XMax"] = "Must specify XMin < XMax";
  }

  return helpMessages;
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void CrossCorrelate::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const double maxDSpaceShift = getProperty("MaxDSpaceShift");
  const int referenceSpectra = getProperty("ReferenceSpectra");
  const double xmin = getProperty("XMin");
  const double xmax = getProperty("XMax");

  const auto index_ref = static_cast<size_t>(referenceSpectra);

  // Get indices of spectra either based on min and max index or from a list
  std::vector<size_t> indexes = getProperty("WorkspaceIndexList");
  if (indexes.empty()) {
    const int wsIndexMin = getProperty("WorkspaceIndexMin");
    const int wsIndexMax = getProperty("WorkspaceIndexMax");
    indexes.reserve(static_cast<size_t>(wsIndexMax - wsIndexMin + 1)); // validated in validateInputs
    std::copy(boost::make_counting_iterator(wsIndexMin), boost::make_counting_iterator(wsIndexMax + 1),
              std::back_inserter(indexes));
  }
  const int numSpectra = static_cast<int>(indexes.size());

  // Output message information
  g_log.information() << "There are " << numSpectra << " spectra in the range\n";

  // Take a copy of the referenceSpectra spectrum
  auto &referenceSpectraE = inputWS->e(index_ref);
  auto &referenceSpectraX = inputWS->x(index_ref);
  auto &referenceSpectraY = inputWS->y(index_ref);
  // Now check if the range between x_min and x_max is valid
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
  const auto numReferenceY = static_cast<int>(referenceYVector.size());

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
    shiftCorrection = static_cast<int>(std::max(0.0, abs((-numReferenceY + 2) - (numReferenceY - 2)) - maxBins)) / 2;
  }

  const int numPoints = 2 * (numReferenceY - shiftCorrection) - 3;
  if (numPoints < 1)
    throw std::runtime_error("Range is not valid");

  MatrixWorkspace_sptr out = create<HistoWorkspace>(*inputWS, numSpectra, Points(static_cast<size_t>(numPoints)));

  const auto referenceVariance = subtractMean(referenceYVector, referenceEVector);

  const double referenceNorm = 1.0 / sqrt(referenceVariance.y);
  double referenceNormE = 0.5 * pow(referenceNorm, 3) * sqrt(referenceVariance.e);

  // Now copy the other spectra
  const bool isDistribution = inputWS->isDistribution();

  auto &outX = out->mutableX(0);
  // this has to be signed to allow for negative values
  for (int i = 0; i < static_cast<int>(outX.size()); ++i) {
    outX[i] = static_cast<double>(i - (numReferenceY - shiftCorrection) + 2);
  }

  // Initialise the progress reporting object
  m_progress = std::make_unique<Progress>(this, 0.0, 1.0, numSpectra);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *out))
  for (int currentSpecIndex = 0; currentSpecIndex < numSpectra; ++currentSpecIndex) // Now loop on all spectra
  {
    PARALLEL_START_INTERRUPT_REGION
    const size_t currentSpecIndex_szt = static_cast<size_t>(currentSpecIndex);
    const size_t wsIndex = indexes[currentSpecIndex_szt]; // Get the ws index from the table
    // Copy spectra info from input Workspace
    out->getSpectrum(currentSpecIndex_szt).copyInfoFrom(inputWS->getSpectrum(wsIndex));
    out->setSharedX(currentSpecIndex_szt, out->sharedX(0));
    // Get temp referenceSpectras
    const auto &inputXVector = inputWS->x(wsIndex);
    const auto &inputYVector = inputWS->y(wsIndex);
    const auto &inputEVector = inputWS->e(wsIndex);
    // Copy Y,E data of spec(currentSpecIndex) to temp vector
    // Now rebin on the grid of referenceSpectra
    std::vector<double> tempY(static_cast<size_t>(numReferenceY));
    std::vector<double> tempE(static_cast<size_t>(numReferenceY));

    VectorHelper::rebin(inputXVector.rawData(), inputYVector.rawData(), inputEVector.rawData(), referenceXVector, tempY,
                        tempE, isDistribution);
    const auto tempVar = subtractMean(tempY, tempE);

    // Calculate the normalisation constant
    const double tempNorm = 1.0 / sqrt(tempVar.y);
    const double tempNormE = 0.5 * pow(tempNorm, 3) * sqrt(tempVar.e);
    const double normalisation = referenceNorm * tempNorm;
    const double normalisationE2 = pow((referenceNorm * tempNormE), 2) + pow((tempNorm * referenceNormE), 2);

    // Get referenceSpectr to the ouput spectrum
    auto &outY = out->mutableY(currentSpecIndex_szt);
    auto &outE = out->mutableE(currentSpecIndex_szt);

    for (int dataIndex = -numReferenceY + 2 + shiftCorrection; dataIndex <= numReferenceY - 2 - shiftCorrection;
         ++dataIndex) {
      const auto dataIndexMagnitude = static_cast<int>(abs(dataIndex));
      const auto dataIndexPositive = bool(dataIndex >= 0);
      double val = 0, err2 = 0;
      double x, y, xE, yE;
      // loop over bin number
      for (int binIndex = numReferenceY - 1 - dataIndexMagnitude; binIndex >= 0; --binIndex) {
        if (dataIndexPositive) {
          x = referenceYVector[binIndex];
          y = tempY[binIndex + dataIndexMagnitude];
          xE = referenceEVector[binIndex];
          yE = tempE[binIndex + dataIndexMagnitude];
        } else {
          x = tempY[binIndex];
          y = referenceYVector[binIndex + dataIndexMagnitude];
          xE = tempE[binIndex];
          yE = referenceEVector[binIndex + dataIndexMagnitude];
        }
        val += (x * y);
        err2 += x * x * yE + y * y * xE;
      }
      const size_t dataIndex_corrected = static_cast<size_t>(dataIndex + numReferenceY - shiftCorrection - 2);
      outY[dataIndex_corrected] = (val * normalisation);
      outE[dataIndex_corrected] = sqrt(val * val * normalisationE2 + normalisation * normalisation * err2);
    }
    // Update progress information
    m_progress->report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  out->getAxis(0)->unit() = UnitFactory::Instance().create("Label");
  Unit_sptr unit = out->getAxis(0)->unit();
  std::shared_ptr<Units::Label> label = std::dynamic_pointer_cast<Units::Label>(unit);
  label->setLabel("Bins of Shift", "\\mathbb{Z}");

  setProperty("OutputWorkspace", out);
}

} // namespace Mantid::Algorithms
