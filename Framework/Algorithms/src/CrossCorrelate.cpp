// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CrossCorrelate.h"
#include "MantidAPI/RawCountValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/VectorHelper.h"
#include <boost/iterator/counting_iterator.hpp>
#include <numeric>
#include <sstream>

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
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<API::WorkspaceUnitValidator>("dSpacing");
  wsValidator->add<API::RawCountValidator>();

  // Input and output workspaces
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "A 2D workspace with X values of d-spacing");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  // Reference spectra against which cross correlation is performed
  declareProperty("ReferenceSpectra", 0, mustBePositive,
                  "The Workspace Index of the spectra to correlate all other "
                  "spectra against. ");
  // Spectra in the range [min to max] will be cross correlated to reference.
  declareProperty("WorkspaceIndexMin", 0, mustBePositive,
                  "The workspace index of the first member of the range of "
                  "spectra to cross-correlate against.");
  declareProperty("WorkspaceIndexMax", 0, mustBePositive,
                  " The workspace index of the last member of the range of "
                  "spectra to cross-correlate against.");
  // Only the data in the range X_min, X_max will be used
  declareProperty("XMin", 0.0,
                  "The starting point of the region to be cross correlated.");
  declareProperty("XMax", 0.0,
                  "The ending point of the region to be cross correlated.");
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void CrossCorrelate::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  int reference = getProperty("ReferenceSpectra");
  const size_t index_ref = static_cast<size_t>(reference);

  // check that the data range specified makes sense
  double xmin = getProperty("XMin");
  double xmax = getProperty("XMax");
  if (xmin >= xmax)
    throw std::runtime_error("Must specify xmin < xmax");

  // Now check if the range between x_min and x_max is valid
  auto &referenceX = inputWS->x(index_ref);
  auto minIt = std::find_if(referenceX.cbegin(), referenceX.cend(),
                            std::bind2nd(std::greater<double>(), xmin));
  if (minIt == referenceX.cend())
    throw std::runtime_error("No data above XMin");
  auto maxIt = std::find_if(minIt, referenceX.cend(),
                            std::bind2nd(std::greater<double>(), xmax));
  if (minIt == maxIt)
    throw std::runtime_error("Range is not valid");

  MantidVec::difference_type difminIt =
      std::distance(referenceX.cbegin(), minIt);
  MantidVec::difference_type difmaxIt =
      std::distance(referenceX.cbegin(), maxIt);

  // Now loop on the spectra in the range spectra_min and spectra_max and get
  // valid spectra

  int specmin = getProperty("WorkspaceIndexMin");
  int specmax = getProperty("WorkspaceIndexMax");
  if (specmin >= specmax)
    throw std::runtime_error(
        "Must specify WorkspaceIndexMin<WorkspaceIndexMax");
  // Get the number of spectra in range specmin to specmax
  int nspecs = 1 + specmax - specmin;
  // Indexes of all spectra in range
  std::vector<size_t> indexes(boost::make_counting_iterator(specmin),
                              boost::make_counting_iterator(specmax + 1));

  std::ostringstream mess;
  if (nspecs == 0) // Throw if no spectra in range
  {
    mess << "No Workspaces in range between" << specmin << " and " << specmax;
    throw std::runtime_error(mess.str());
  }

  // Output message information
  mess << "There are " << nspecs << " Workspaces in the range\n";
  g_log.information(mess.str());
  mess.str("");

  // Take a copy of  the reference spectrum
  auto &referenceY = inputWS->y(index_ref);
  auto &referenceE = inputWS->e(index_ref);

  std::vector<double> refX(maxIt - minIt);
  std::vector<double> refY(maxIt - minIt - 1);
  std::vector<double> refE(maxIt - minIt - 1);

  std::copy(minIt, maxIt, refX.begin());
  mess << "min max" << refX.front() << " " << refX.back();
  g_log.information(mess.str());
  mess.str("");
  std::copy(referenceY.cbegin() + difminIt,
            referenceY.cbegin() + (difmaxIt - 1), refY.begin());
  std::copy(referenceE.cbegin() + difminIt,
            referenceE.cbegin() + (difmaxIt - 1), refE.begin());

  // Now start the real stuff
  // Create a 2DWorkspace that will hold the result
  const int nY = static_cast<int>(refY.size());
  const int npoints = 2 * nY - 3;
  if (npoints < 1)
    throw std::runtime_error("Range is not valid");

  MatrixWorkspace_sptr out =
      create<HistoWorkspace>(*inputWS, nspecs, Points(npoints));

  // Calculate the mean value of the reference spectrum and associated error
  // squared
  double refMean = std::accumulate(refY.cbegin(), refY.cend(), 0.0);
  double refMeanE2 = std::accumulate(refE.cbegin(), refE.cend(), 0.0,
                                     VectorHelper::SumSquares<double>());
  refMean /= static_cast<double>(nY);
  refMeanE2 /= static_cast<double>(nY * nY);
  auto itY = refY.begin();
  auto itE = refE.begin();

  double refVar = 0.0, refVarE = 0.0;
  for (; itY != refY.end(); ++itY, ++itE) {
    (*itY) -= refMean;                    // Now the vector is (y[i]-refMean)
    (*itE) = (*itE) * (*itE) + refMeanE2; // New error squared
    double t = (*itY) * (*itY);           //(y[i]-refMean)^2
    refVar += t;                          // Sum previous term
    refVarE += 4.0 * t * (*itE);          // Error squared
  }

  double refNorm = 1.0 / sqrt(refVar);
  double refNormE = 0.5 * pow(refNorm, 3) * sqrt(refVarE);

  // Now copy the other spectra
  bool is_distrib = inputWS->isDistribution();

  {
    std::vector<double> XX(npoints);
    for (int i = 0; i < npoints; ++i) {
      XX[i] = static_cast<double>(i - nY + 2);
    }
    out->mutableX(0) = std::move(XX);
  }
  // Initialise the progress reporting object
  m_progress = make_unique<Progress>(this, 0.0, 1.0, nspecs);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *out))
  for (int i = 0; i < nspecs; ++i) // Now loop on all spectra
  {
    PARALLEL_START_INTERUPT_REGION
    size_t wsIndex = indexes[i]; // Get the ws index from the table
    // Copy spectra info from input Workspace
    out->getSpectrum(i).copyInfoFrom(inputWS->getSpectrum(wsIndex));

    out->setSharedX(i, out->sharedX(0));

    // Get temp references
    const auto &iX = inputWS->x(wsIndex);
    const auto &iY = inputWS->y(wsIndex);
    const auto &iE = inputWS->e(wsIndex);
    // Copy Y,E data of spec(i) to temp vector
    // Now rebin on the grid of reference spectrum
    std::vector<double> tempY(nY);
    std::vector<double> tempE(nY);
    VectorHelper::rebin(iX.rawData(), iY.rawData(), iE.rawData(), refX, tempY,
                        tempE, is_distrib);
    // Calculate the mean value of tempY
    double tempMean = std::accumulate(tempY.begin(), tempY.end(), 0.0);
    tempMean /= static_cast<double>(nY);
    double tempMeanE2 = std::accumulate(tempE.begin(), tempE.end(), 0.0,
                                        VectorHelper::SumSquares<double>());
    tempMeanE2 /= static_cast<double>(nY * nY);
    //
    std::vector<double>::iterator itY;
    std::vector<double>::iterator itE;
    itY = tempY.begin();
    itE = tempE.begin();
    double tempVar = 0.0, tempVarE = 0.0;
    for (; itY != tempY.end(); ++itY, ++itE) {
      (*itY) -= tempMean;                    // Now the vector is (y[i]-refMean)
      (*itE) = (*itE) * (*itE) + tempMeanE2; // New error squared
      double t = (*itY) * (*itY);
      tempVar += t;
      tempVarE += 4.0 * t * (*itE);
    }

    // Calculate the normalisation constant
    double tempNorm = 1.0 / sqrt(tempVar);
    double tempNormE = 0.5 * pow(tempNorm, 3) * sqrt(tempVarE);
    double normalisation = refNorm * tempNorm;
    double normalisationE2 =
        pow((refNorm * tempNormE), 2) + pow((tempNorm * refNormE), 2);
    // Get reference to the ouput spectrum
    auto &outY = out->mutableY(i);
    auto &outE = out->mutableE(i);

    for (int k = -nY + 2; k <= nY - 2; ++k) {
      int kp = abs(k);
      double val = 0, err2 = 0, x, y, xE, yE;
      for (int j = nY - 1 - kp; j >= 0; --j) {
        if (k >= 0) {
          x = refY[j];
          y = tempY[j + kp];
          xE = refE[j];
          yE = tempE[j + kp];
        } else {
          x = tempY[j];
          y = refY[j + kp];
          xE = tempE[j];
          yE = refE[j + kp];
        }
        val += (x * y);
        err2 += x * x * yE + y * y * xE;
      }
      outY[k + nY - 2] = (val * normalisation);
      outE[k + nY - 2] = sqrt(val * val * normalisationE2 +
                              normalisation * normalisation * err2);
    }
    // Update progress information
    // double prog=static_cast<double>(i)/nspecs;
    // progress(prog);
    m_progress->report();
    // interruption_point();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", out);
}

} // namespace Algorithms
} // namespace Mantid
