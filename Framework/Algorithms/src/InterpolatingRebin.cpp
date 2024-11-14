// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/InterpolatingRebin.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_interp.h>
#include <gsl/gsl_spline.h>

#include <boost/lexical_cast.hpp>

namespace Mantid::Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(InterpolatingRebin)

using namespace Kernel;
using namespace API;
using namespace HistogramData;
using namespace DataObjects;

/** Overwrites the parent (Rebin) init method
 * These properties will be declares instead
 */
void InterpolatingRebin::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                  "Workspace containing the input data");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name to give the output workspace");

  declareProperty(std::make_unique<ArrayProperty<double>>("Params", std::make_shared<RebinParamsValidator>()),
                  "A comma separated list of first bin boundary, width, last bin boundary. "
                  "Optionally "
                  "this can be followed by a comma and more widths and last boundary "
                  "pairs. "
                  "Optionally this can also be a single number, which is the bin width. "
                  "In this case, the boundary of binning will be determined by minimum and "
                  "maximum TOF "
                  "values among all events, or previous binning boundary, in case of event "
                  "Workspace, or "
                  "non-event Workspace, respectively. Negative width values indicate "
                  "logarithmic binning. ");
}

/**
 * Overwrites the parent (Rebin) validateInputs
 * Avoids all bother about binning modes and powers
 */
/// Validate that the input properties are sane.
std::map<std::string, std::string> InterpolatingRebin::validateInputs() {
  std::map<std::string, std::string> helpMessages;

  // as part of validation, force the binwidth to be compatible with set bin mode
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  std::vector<double> rbParams = getProperty("Params");
  try {
    Rebin::rebinParamsFromInput(rbParams, *inputWS, g_log);
  } catch (std::exception &err) {
    helpMessages["Params"] = err.what();
  }
  return helpMessages;
}

/** Executes the rebin algorithm
 *  @throw runtime_error Thrown if the bin range does not intersect the range of
 *the input workspace
 */
void InterpolatingRebin::exec() {
  // Get the input workspace
  MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");

  // retrieve the properties
  std::vector<double> rb_params = Rebin::rebinParamsFromInput(getProperty("Params"), *inputW, g_log);
  HistogramData::BinEdges XValues_new(0);
  // create new output X axis
  const int ntcnew = VectorHelper::createAxisFromRebinParams(rb_params, XValues_new.mutableRawData());

  const auto nHists = static_cast<int>(inputW->getNumberHistograms());
  // make output Workspace the same type as the input but with the new axes
  MatrixWorkspace_sptr outputW = create<MatrixWorkspace>(*inputW, BinEdges(ntcnew));
  // Copy over the 'vertical' axis
  if (inputW->axes() > 1)
    outputW->replaceAxis(1, std::unique_ptr<Axis>(inputW->getAxis(1)->clone(outputW.get())));
  outputW->setDistribution(true);

  // this calculation requires a distribution workspace but deal with the
  // situation when we don't get this
  bool distCon(false);
  if (!inputW->isDistribution()) {
    g_log.debug() << "Converting the input workspace to a distribution\n";
    WorkspaceHelpers::makeDistribution(inputW);
    distCon = true;
  }

  try {
    // evaluate the rebinned data
    outputYandEValues(inputW, XValues_new, outputW);
  } catch (std::exception &) {

    if (distCon) {
      // we need to return the input workspace to the state we found it in
      WorkspaceHelpers::makeDistribution(inputW, false);
    }
    throw;
  }

  // check if there was a convert to distribution done previously
  if (distCon) {
    g_log.debug() << "Converting the input and output workspaces _from_ distributions\n";
    WorkspaceHelpers::makeDistribution(inputW, false);
    // the calculation produces a distribution workspace but if they passed a
    // non-distribution workspace they maybe not expect it, so convert back to
    // the same form that was given
    WorkspaceHelpers::makeDistribution(outputW, false);
    outputW->setDistribution(false);
  }

  // Now propagate any masking correctly to the output workspace
  // More efficient to have this in a separate loop because
  // MatrixWorkspace::maskBins blocks multi-threading
  for (int i = 0; i < nHists; ++i) {
    if (inputW->hasMaskedBins(i)) // Does the current spectrum have any masked bins?
    {
      this->propagateMasks(inputW, outputW, i);
    }
  }

  for (int i = 0; i < outputW->axes(); ++i) {
    outputW->getAxis(i)->unit() = inputW->getAxis(i)->unit();
  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputW);
}
/** Calls the interpolation function for each histogram in the workspace
 *  @param[in] inputW workspace with un-interpolated data
 *  @param[in] XValues_new new x-values to interpolated to
 *  @param[out] outputW this will contain the interpolated data, the lengths of
 * the histograms must corrospond with the number of x-values in XValues_new
 */
void InterpolatingRebin::outputYandEValues(const API::MatrixWorkspace_const_sptr &inputW,
                                           const HistogramData::BinEdges &XValues_new,
                                           const API::MatrixWorkspace_sptr &outputW) {
  g_log.debug() << "Preparing to calculate y-values using splines and estimate errors\n";

  // prepare to use GSL functions but don't let them terminate Mantid
  gsl_error_handler_t *old_handler = gsl_set_error_handler(nullptr);

  const auto histnumber = static_cast<int>(inputW->getNumberHistograms());
  Progress prog(this, 0.0, 1.0, histnumber);
  for (int hist = 0; hist < histnumber; ++hist) {

    try {
      // output data arrays are implicitly filled by function
      outputW->setHistogram(hist, cubicInterpolation(inputW->histogram(hist), XValues_new));
    } catch (std::exception &ex) {
      g_log.error() << "Error in rebin function: " << ex.what() << '\n';
      throw;
    }

    // Populate the output workspace X values
    outputW->setBinEdges(hist, XValues_new);

    prog.report();
  }

  gsl_set_error_handler(old_handler);
}

/**Uses cubic splines to interpolate the mean rate of change of the integral
 *  over the inputed data bins to that for the user supplied bins.
 *  Note that this algorithm was implemented to provide a little more resolution
 *  on high count rate data. Whether it is more accurate than the standard rebin
 *  for all, or your, application needs more thought.
 *  The input data must be a distribution (proportional to the rate of change
 *e.g.
 *  raw_counts/bin_widths) but note that these mean rate of counts data
 *  are integrals not (instanteously) sampled data. The error values on each
 *point
 *  are a weighted mean of the error values from the surrounding input data.
 *This makes sense if the interpolation error is low compared to the statistical
 *  errors on each input data point. The weighting is inversely proportional to
 *  the distance from the original data point to the new interpolated one.
 *
 *  @param[in] oldHistogram :: the histogram of the output workspace that will
 *be interpolated
 *  @param[in] xNew :: x-values to rebin to, must be monotonically increasing
 *  @return Histogram :: A new Histogram containing the BinEdges xNew and
 *the calculated HistogramY and HistogramE
 *  @throw runtime_error :: if there is a problem executing one of the GSL
 *functions
 *  @throw invalid_argument :: if any output x-values are outside the range of
 *input
 *x-values
 **/
Histogram InterpolatingRebin::cubicInterpolation(const Histogram &oldHistogram, const BinEdges &xNew) const {
  const auto &yOld = oldHistogram.y();

  const size_t size_old = yOld.size();
  if (size_old == 0)
    throw std::runtime_error("Empty spectrum found, aborting!");

  const size_t size_new = xNew.size() - 1; // -1 because BinEdges

  // get the bin centres of the input data
  auto xCensOld = oldHistogram.points();
  VectorHelper::convertToBinCentre(oldHistogram.x().rawData(), xCensOld.mutableRawData());
  // the centres of the output data
  Points xCensNew(size_new);
  VectorHelper::convertToBinCentre(xNew.rawData(), xCensNew.mutableRawData());

  // find the range of input values whose x-values just suround the output
  // x-values
  size_t oldIn1 = std::lower_bound(xCensOld.begin(), xCensOld.end(), xCensNew.front()) - xCensOld.begin();
  if (oldIn1 == 0) { // the lowest interpolation value might be out of range but
                     // if it is almost on the boundary let it through
    if (std::abs(xCensOld.front() - xCensNew.front()) < 1e-8 * (xCensOld.back() - xCensOld.front())) {
      oldIn1 = 1;
      // make what should be a very small correction
      xCensNew.mutableRawData().front() = xCensOld.front();
    }
  }

  size_t oldIn2 = std::lower_bound(xCensOld.begin(), xCensOld.end(), xCensNew.back()) - xCensOld.begin();
  if (oldIn2 == size_old) { // the highest point is nearly out of range of the
                            // input data but if it's very near the boundary let
                            // it through
    if (std::abs(xCensOld.back() - xCensNew.back()) < 1e-8 * (xCensOld.back() - xCensOld.front())) {
      oldIn2 = size_old - 1;
      // make what should be a very small correction
      xCensNew.mutableRawData().back() = xCensOld.back();
    }
  }

  // check that the intepolation points fit well enough within the data for
  // reliable intepolation to be done
  bool goodRangeLow(false), goodRangeHigh(false), canInterpol(false);
  if (oldIn1 > 1) { // set the range of the fit, including more input data to
                    // improve accuracy
    oldIn1 -= 2;
    goodRangeLow = true;
    canInterpol = true;
  } else {
    if (oldIn1 > 0) {
      canInterpol = true;
      oldIn1--;
    }
  }

  if (oldIn2 < size_old - 1) {
    oldIn2++;
    goodRangeHigh = true;
  } else {
    if (oldIn2 >= size_old) {
      canInterpol = false;
    }
  }

  const auto &xOld = oldHistogram.x();
  const auto &eOld = oldHistogram.e();

  // No Interpolation branch
  if (!canInterpol) {
    if (VectorHelper::isConstantValue(yOld.rawData())) {
      // this copies the single y-value into the output array, errors are still
      // calculated from the nearest input data points
      // this is as much as we need to do in this (trival) case
      return noInterpolation(oldHistogram, xNew);
    } else { // some points are two close to the edge of the data

      throw std::invalid_argument(std::string("At least one x-value to interpolate to is outside the "
                                              "range of the original data.\n") +
                                  "original data range: " + boost::lexical_cast<std::string>(xOld.front()) + " to " +
                                  boost::lexical_cast<std::string>(xOld.back()) + "\n" +
                                  "range to try to interpolate to " + boost::lexical_cast<std::string>(xNew.front()) +
                                  " to " + boost::lexical_cast<std::string>(xNew.back()));
    }
  }

  // Can Interpolate
  // Create Histogram
  Histogram newHistogram{xNew, Frequencies(xNew.size() - 1)};

  auto &yNew = newHistogram.mutableY();
  auto &eNew = newHistogram.mutableE();

  if ((!goodRangeLow) || (!goodRangeHigh)) {
    g_log.information() << "One or more points in the interpolation are near "
                           "the boundary of the input data, these points will "
                           "have slightly less accuracy\n";
  }

  // get the GSL to allocate the memory
  gsl_interp_accel *acc = nullptr;
  gsl_spline *spline = nullptr;
  try {
    acc = gsl_interp_accel_alloc();
    const size_t nPoints = oldIn2 - oldIn1 + 1;
    spline = gsl_spline_alloc(gsl_interp_cspline, nPoints);

    // test the allocation
    if (!acc || !spline ||
        // calculate those splines, GSL uses pointers to the vector array (which
        // is always contiguous)
        gsl_spline_init(spline, &xCensOld[oldIn1], &yOld[oldIn1], nPoints)) {
      throw std::runtime_error("Error setting up GSL spline functions");
    }

    for (size_t i = 0; i < size_new; ++i) {
      yNew[i] = gsl_spline_eval(spline, xCensNew[i], acc);
      //(basic) error estimate the based on a weighted mean of the errors of the
      // surrounding input data points
      eNew[i] = estimateError(xCensOld, eOld, xCensNew[i]);
    }
  }
  // for GSL to clear up its memory use
  catch (std::exception &) {
    if (acc) {
      if (spline) {
        gsl_spline_free(spline);
      }
      gsl_interp_accel_free(acc);
    }
    throw;
  }
  gsl_spline_free(spline);
  gsl_interp_accel_free(acc);

  return newHistogram;
}
/** This can be used whenever the original spectrum is filled with only one
 * value. It is intended allow
 *  some spectra with null like values, for example all zeros
 *  @param[in] oldHistogram :: the histogram of the output workspace that will
 *be interpolated
 *  @param[in] xNew :: x-values to rebin to, must be monotonically increasing
 *  @return Histogram :: A new Histogram containing the BinEdges xNew and
 *the calculated HistogramY and HistogramE
 */
Histogram InterpolatingRebin::noInterpolation(const Histogram &oldHistogram, const BinEdges &xNew) const {
  Histogram newHistogram{xNew, Frequencies(xNew.size() - 1)};
  auto &yNew = newHistogram.mutableY();
  auto &eNew = newHistogram.mutableE();

  yNew.assign(yNew.size(), oldHistogram.y().front());

  const auto &xPointData = oldHistogram.points();
  const auto &eOld = oldHistogram.e();

  // -1 because xNew.size is 1 bigger than eNew
  std::transform(xNew.cbegin(), xNew.cend() - 1, eNew.begin(),
                 [&](double x) { return estimateError(xPointData, eOld, x); });

  return newHistogram;
}
/**Estimates the error on each interpolated point by assuming it is similar to
 * the errors in
 *  near by input data points. Output points with the same x-value as an input
 * point have the
 *  same error as the input point. Points between input points have a error
 * value that is a weighted mean of the closest input points
 *  @param[in] xsOld x-values of the input data around the point of interested
 *  @param[in] esOld error values for the same points in the input data as xsOld
 *  @param[in] xNew the value of x for at the point of interest
 *  @return the estimated error at that point
 */
double InterpolatingRebin::estimateError(const Points &xsOld, const HistogramE &esOld, const double xNew) const {

  // find the first point in the array that has a higher value of x, we'll base
  // some of the error estimate on the error on this point

  const size_t indAbove = std::lower_bound(xsOld.begin(), xsOld.end(), xNew) - xsOld.begin();

  // if the point's x-value is out of the range covered by the x-values in the
  // input data return the error value at the end of the range
  if (indAbove == 0) {
    return esOld.front();
  }
  // xsOld is 1 longer than xsOld
  if (indAbove >= esOld.size()) { // cubicInterpolation() checks that that there
                                  // are no empty histograms
    return esOld.back();
  }

  const double error1 = esOld[indAbove];
  // ratio of weightings will be inversely proportional to the distance between
  // the points
  double weight1 = xsOld[indAbove] - xNew;
  // check if the points are close enough agnoring any spurious effects that can
  // occur with exact comparisons of floating point numbers
  if (weight1 < 1e-100) {
    // the point is on an input point, all the weight is on this point ignore
    // the other
    return error1;
  }
  weight1 = 1 / weight1;

  // if p were zero lower_bound must have found xCensNew <= xCensOld.front() but
  // in that situation we should have exited before now
  const double error2 = esOld[indAbove - 1];
  double weight2 = xNew - xsOld[indAbove - 1];
  if (weight2 < 1e-100) {
    // the point is on an input point, all the weight is on this point ignore
    // the other
    return error2;
  }
  weight2 = 1 / weight2;

  return (weight1 * error1 + weight2 * error2) / (weight1 + weight2);
}

} // namespace Mantid::Algorithms
