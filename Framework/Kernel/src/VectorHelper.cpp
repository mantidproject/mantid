// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <cmath>
#include <stdexcept>

#include "MantidKernel/VectorHelper.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <numeric>
#include <sstream>

using std::size_t;

namespace Mantid::Kernel::VectorHelper {

/** Creates a new output X array given a 'standard' set of rebinning parameters.
 *  @param[in]  params Rebin parameters input [x_1, delta_1,x_2, ...
 *,x_n-1,delta_n-1,x_n]
 *  @param[out] xnew   The newly created axis resulting from the input params
 *  @param[in] resize_xnew If false then the xnew vector is NOT resized. Useful
 *if the number of bins needs determining. (Default=True)
 *  @param[in] full_bins_only If true, bins of the size less than the current
 *step are not included. (Default=True)
 *  @param[in] xMinHint x_1 if params contains only delta_1.
 *  @param[in] xMaxHint x_2 if params contains only delta_1.
 *  @param[in] useReverseLogarithmic wheter or not to use reverse logarithmic for bins
 *  @param[in] power the power in case of inverse power sum. Must be between 0 and 1 or is ignored.
 *  @return The number of bin boundaries in the new axis
 **/
int MANTID_KERNEL_DLL createAxisFromRebinParams(const std::vector<double> &params, std::vector<double> &xnew,
                                                const bool resize_xnew, const bool full_bins_only,
                                                const double xMinHint, const double xMaxHint,
                                                const bool useReverseLogarithmic, const double power) {
  std::vector<double> tmp;
  const std::vector<double> &fullParams = [&params, &tmp, xMinHint, xMaxHint]() {
    if (params.size() == 1) {
      if (std::isnan(xMinHint) || std::isnan(xMaxHint)) {
        throw std::runtime_error("createAxisFromRebinParams: xMinHint and "
                                 "xMaxHint must be supplied if params "
                                 "contains only the bin width.");
      }
      tmp.resize(3);
      tmp = {xMinHint, params.front(), xMaxHint};
      return tmp;
    }
    return params;
  }();
  int ibound(2), istep(1), inew(1);
  // highest index in params array containing a bin boundary
  auto ibounds = static_cast<int>(fullParams.size());
  int isteps = ibounds - 1; // highest index in params array containing a step

  // This coefficitent represents the maximum difference between the size of the last bin and all
  // the other bins.
  double lastBinCoef(0.25);

  if (full_bins_only) {
    // For full_bin_only, we want it so that last bin couldn't be smaller than the previous bin
    lastBinCoef = 1.0;
  }

  double xs = 0;
  double xcurr = fullParams[0];

  xnew.clear();
  if (resize_xnew)
    xnew.emplace_back(xcurr);

  int currDiv = 1;

  bool isPower = power > 0 && power <= 1;

  while ((ibound <= ibounds) && (istep <= isteps)) {
    // if step is negative then it is logarithmic step
    bool isLogBin = (fullParams[istep] < 0.0);
    bool isReverseLogBin = isLogBin && useReverseLogarithmic;
    double alpha = std::fabs(fullParams[istep]);

    if (isReverseLogBin && xcurr == fullParams[ibound - 2]) {
      // we are starting a new bin, but since it is a rev log, xcurr needs to be at its end
      xcurr = fullParams[ibound];
    }
    if (!isPower) {
      if (!isLogBin)
        xs = fullParams[istep];
      else {
        if (useReverseLogarithmic) {
          // we go through a reverse log bin by starting from its end, and working our way back to the beginning
          // this way we can define the bins in a reccuring way, and with a more obvious closeness with the usual log.
          double x0 = fullParams[ibound - 2];
          double step = x0 + fullParams[ibound] - xcurr;

          xs = -step * alpha;

        } else
          xs = xcurr * fabs(fullParams[istep]);
      }
    } else {
      xs = fullParams[istep] * std::pow(currDiv, -power);
      ++currDiv;
    }

    if (fabs(xs) == 0.0) {
      // Someone gave a 0-sized step! What a dope.
      throw std::runtime_error("Invalid binning step provided! Can't create binning axis.");
    } else if (!std::isfinite(xs)) {
      throw std::runtime_error("An infinite or NaN value was found in the binning parameters.");
    }

    if ((!isReverseLogBin && xcurr + xs * (1.0 + lastBinCoef) <= fullParams[ibound]) ||
        (isReverseLogBin && xcurr + 2 * xs >= fullParams[ibound - 2])) {
      // If we can still fit current bin _plus_ specified portion of a last bin, continue
      xcurr += xs;

    } else {
      // If this is the start of the last bin, finish this range
      if (!isReverseLogBin) {
        if (full_bins_only)
          // For full_bins_only, finish the range by adding one more full bin, so that last bin is not bigger than the
          // previous one
          xcurr += xs;
        else
          // For non full_bins_only, finish by adding as much as is left from the range
          xcurr = fullParams[ibound];
      } else {
        // we have finished this range, because its starting time has already been added, so we jump back to the last
        // value of the bin and resume normal behaviour
        xcurr = fullParams[ibound];
      }
      istep += 2;
      ibound += 2;
    }
    if (resize_xnew)
      xnew.emplace_back(xcurr);
    inew++;
  }
  std::sort(xnew.begin(), xnew.end());
  return inew;
}

/** Rebins data according to a new output X array
 *
 *  @param[in] xold Old X array of data.
 *  @param[in] yold Old Y array of data. Must be 1 element shorter than xold.
 *  @param[in] eold Old error array of data. Must be same length as yold.
 *  @param[in] xnew X array of data to rebin to.
 *  @param[out] ynew Rebinned data. Must be 1 element shorter than xnew.
 *  @param[out] enew Rebinned errors. Must be same length as ynew.
 *  @param[in] distribution Flag defining if distribution data (true) or not
 *(false).
 *  @param[in] addition If true, rebinned values are added to the existing
 *ynew/enew vectors.
 *                      NOTE THAT, IN THIS CASE THE RESULTING enew WILL BE THE
 *SQUARED ERRORS
 *                      AND THE ynew WILL NOT HAVE THE BIN WIDTH DIVISION PUT
 *IN!
 *  @throw runtime_error Thrown if algorithm cannot execute.
 *  @throw invalid_argument Thrown if input to function is incorrect.
 **/
void rebin(const std::vector<double> &xold, const std::vector<double> &yold, const std::vector<double> &eold,
           const std::vector<double> &xnew, std::vector<double> &ynew, std::vector<double> &enew, bool distribution,
           bool addition) {
  // Make sure y and e vectors are of correct sizes
  const size_t size_xold = xold.size();
  if (size_xold != (yold.size() + 1) || size_xold != (eold.size() + 1))
    throw std::runtime_error("rebin: old y and error vectors should be of same "
                             "size & 1 shorter than x");
  const size_t size_xnew = xnew.size();
  if (size_xnew != (ynew.size() + 1) || size_xnew != (enew.size() + 1))
    throw std::runtime_error("rebin: new y and error vectors should be of same "
                             "size & 1 shorter than x");

  size_t size_yold = yold.size();
  size_t size_ynew = ynew.size();

  if (!addition) {
    // Make sure ynew & enew contain zeroes
    std::fill(ynew.begin(), ynew.end(), 0.0);
    std::fill(enew.begin(), enew.end(), 0.0);
  }

  size_t iold = 0, inew = 0;
  double width;

  while ((inew < size_ynew) && (iold < size_yold)) {
    double xo_low = xold[iold];
    double xo_high = xold[iold + 1];
    double xn_low = xnew[inew];
    double xn_high = xnew[inew + 1];
    if (xn_high <= xo_low)
      inew++; /* old and new bins do not overlap */
    else if (xo_high <= xn_low)
      iold++; /* old and new bins do not overlap */
    else {
      //        delta is the overlap of the bins on the x axis
      // delta = std::min(xo_high, xn_high) - std::max(xo_low, xn_low);
      double delta = xo_high < xn_high ? xo_high : xn_high;
      delta -= xo_low > xn_low ? xo_low : xn_low;
      width = xo_high - xo_low;
      if ((delta <= 0.0) || (width <= 0.0)) {
        // No need to throw here, just return (ynew & enew will be empty)
        // throw std::runtime_error("rebin: no bin overlap detected");
        return;
      }
      /*
       *        yoldp contains counts/unit time, ynew contains counts
       *	       enew contains counts**2
       *        ynew has been filled with zeros on creation
       */
      if (distribution) {
        // yold/eold data is distribution
        ynew[inew] += yold[iold] * delta;
        // this error is calculated in the same way as opengenie
        enew[inew] += eold[iold] * eold[iold] * delta * width;
      } else {
        // yold/eold data is not distribution
        // do implicit division of yold by width in summing.... avoiding the
        // need for temporary yold array
        // this method is ~7% faster and uses less memory
        ynew[inew] += yold[iold] * delta / width; // yold=yold/width
        // eold=eold/width, so divide by width**2 compared with distribution
        // calculation
        enew[inew] += eold[iold] * eold[iold] * delta / width;
      }
      if (xn_high > xo_high) {
        iold++;
      } else {
        inew++;
      }
    }
  }

  if (!addition) // If using the addition facility, have to do bin width and
                 // sqrt errors externally
  {
    if (distribution) {
      /*
       * convert back to counts/unit time
       */
      for (size_t i = 0; i < size_ynew; ++i) {
        {
          width = xnew[i + 1] - xnew[i];
          if (width != 0.0) {
            ynew[i] /= width;
            enew[i] = sqrt(enew[i]) / width;
          } else {
            throw std::invalid_argument("rebin: Invalid output X array, contains consecutive X values");
          }
        }
      }
    } else {
      // non-distribution, just square root final error value
      using pf = double (*)(double);
      pf uf = std::sqrt;
      std::transform(enew.begin(), enew.end(), enew.begin(), uf);
    }
  }
}

//-------------------------------------------------------------------------------------------------
/** Rebins histogram data according to a new output X array. Should be faster
 *than previous one.
 *  @author Laurent Chapon 10/03/2009
 *
 *  @param[in] xold Old X array of data.
 *  @param[in] yold Old Y array of data. Must be 1 element shorter than xold.
 *  @param[in] eold Old error array of data. Must be same length as yold.
 *  @param[in] xnew X array of data to rebin to.
 *  @param[out] ynew Rebinned data. Must be 1 element shorter than xnew.
 *  @param[out] enew Rebinned errors. Must be same length as ynew.
 *  @param[in] addition If true, rebinned values are added to the existing
 *ynew/enew vectors.
 *                      NOTE THAT, IN THIS CASE THE RESULTING enew WILL BE THE
 *SQUARED ERRORS!
 *  @throw runtime_error Thrown if vector sizes are inconsistent
 **/
void rebinHistogram(const std::vector<double> &xold, const std::vector<double> &yold, const std::vector<double> &eold,
                    const std::vector<double> &xnew, std::vector<double> &ynew, std::vector<double> &enew,
                    bool addition) {
  // Make sure y and e vectors are of correct sizes
  const size_t size_yold = yold.size();
  if (xold.size() != (size_yold + 1) || size_yold != eold.size())
    throw std::runtime_error("rebin: y and error vectors should be of same size & 1 shorter than x");
  const size_t size_ynew = ynew.size();
  if (xnew.size() != (size_ynew + 1) || size_ynew != enew.size())
    throw std::runtime_error("rebin: y and error vectors should be of same size & 1 shorter than x");

  // If not adding to existing vectors, make sure ynew & enew contain zeroes
  if (!addition) {
    ynew.assign(size_ynew, 0.0);
    enew.assign(size_ynew, 0.0);
  }

  // Find the starting points to avoid wasting time processing irrelevant bins
  size_t iold = 0, inew = 0; // iold/inew is the bin number under consideration
                             // (counting from 1, so index+1)
  if (xnew.front() > xold.front()) {
    auto it = std::upper_bound(xold.cbegin(), xold.cend(), xnew.front());
    if (it == xold.end())
      return;
    //      throw std::runtime_error("No overlap: max of X-old < min of X-new");
    iold = std::distance(xold.begin(), it) - 1; // Old bin to start at (counting from 0)
  } else {
    auto it = std::upper_bound(xnew.cbegin(), xnew.cend(), xold.front());
    if (it == xnew.cend())
      return;
    //      throw std::runtime_error("No overlap: max of X-new < min of X-old");
    inew = std::distance(xnew.cbegin(), it) - 1; // New bin to start at (counting from 0)
  }

  double frac, fracE;
  double oneOverWidth, overlap;
  double temp;

  // loop over old vector from starting point calculated above
  for (; iold < size_yold; ++iold) {
    double xold_of_iold_p_1 = xold[iold + 1]; // cache for speed
    // If current old bin is fully enclosed by new bin, just unload the counts
    if (xold_of_iold_p_1 <= xnew[inew + 1]) {
      ynew[inew] += yold[iold];
      temp = eold[iold];
      enew[inew] += temp * temp;
      // If the upper bin boundaries were equal, then increment inew
      if (xold_of_iold_p_1 == xnew[inew + 1])
        inew++;
    } else {
      double xold_of_iold = xold[iold]; // cache for speed
      // This is the counts per unit X in current old bin
      oneOverWidth = 1. / (xold_of_iold_p_1 - xold_of_iold); // cache 1/width to speed things up
      frac = yold[iold] * oneOverWidth;
      temp = eold[iold];
      fracE = temp * temp * oneOverWidth;

      // Now loop over bins in new vector overlapping with current 'old' bin
      while (inew < size_ynew && xnew[inew + 1] <= xold_of_iold_p_1) {
        overlap = xnew[inew + 1] - std::max(xnew[inew], xold_of_iold);
        ynew[inew] += frac * overlap;
        enew[inew] += fracE * overlap;
        ++inew;
      }

      // Stop if at end of new X range
      if (inew == size_ynew)
        break;

      // Unload the rest of the current old bin into the current new bin
      overlap = xold_of_iold_p_1 - xnew[inew];
      ynew[inew] += frac * overlap;
      enew[inew] += fracE * overlap;
    }
  } // loop over old bins

  if (!addition) // If this used to add at the same time then not necessary
                 // (should be done externally)
  {
    // Now take the root-square of the errors
    using pf = double (*)(double);
    pf uf = std::sqrt;
    std::transform(enew.begin(), enew.end(), enew.begin(), uf);
  }
}

//-------------------------------------------------------------------------------------------------
/**
 * Convert the given set of bin boundaries into bin centre values
 * @param bin_edges :: A vector of values specifying bin boundaries
 * @param bin_centres :: An output vector of bin centre values.
 */
void convertToBinCentre(const std::vector<double> &bin_edges, std::vector<double> &bin_centres) {
  const std::vector<double>::size_type npoints = bin_edges.size();
  if (bin_centres.size() != npoints) {
    bin_centres.resize(npoints);
  }

  // The custom binary function modifies the behaviour of the algorithm to
  // compute the average of
  // two adjacent bin boundaries
  std::adjacent_difference(bin_edges.begin(), bin_edges.end(), bin_centres.begin(), SimpleAverage<double>());
  // The algorithm copies the first element of the input to the first element of
  // the output so we need to
  // remove the first element of the output
  bin_centres.erase(bin_centres.begin());
}

//-------------------------------------------------------------------------------------------------
/**
 * Convert the given set of bin centers into bin boundary values.
 * NOTE: the first and last bin boundaries are calculated so
 * that the first and last bin centers are in the center of the
 * first and last bins, respectively. For a particular set of
 * bin centers, this may not be correct, but it is the best that
 * can be done, lacking any other information. For an empty input vector, an
 * empty output is returned. For an input vector of size 1, i.e., a single bin,
 * there is no information about a proper bin size, so it is set to 1.0.
 *
 * @param bin_centers :: A vector of values specifying bin centers.
 * @param bin_edges   :: An output vector of values specifying bin
 *                       boundaries
 *
 */
void convertToBinBoundary(const std::vector<double> &bin_centers, std::vector<double> &bin_edges) {
  const auto n = bin_centers.size();

  // Special case empty input: output is also empty
  if (n == 0) {
    bin_edges.resize(0);
    return;
  }

  bin_edges.resize(n + 1);

  // Special case input of size one: we have no means of guessing the bin size,
  // set it to 1.
  if (n == 1) {
    bin_edges[0] = bin_centers[0] - 0.5;
    bin_edges[1] = bin_centers[0] + 0.5;
    return;
  }

  for (size_t i = 0; i < n - 1; ++i) {
    bin_edges[i + 1] = 0.5 * (bin_centers[i] + bin_centers[i + 1]);
  }

  bin_edges[0] = bin_centers[0] - (bin_edges[1] - bin_centers[0]);

  bin_edges[n] = bin_centers[n - 1] + (bin_centers[n - 1] - bin_edges[n - 1]);
}

/** Finds the bin index of a value from the vector of bin centers
 * without converting the whole array to bin edges.
 * Assumes the vector is already sorted ascending.
 * @param bin_centers : vector of bin centers
 * @param value : input value
 * @return : the bin index of the value
 * @throw std::out_of_range : if vector is empty or value is out of it's range
 * (in bin edge representation)
 */

size_t indexOfValueFromCenters(const std::vector<double> &bin_centers, const double value) {
  int index = indexOfValueFromCentersNoThrow(bin_centers, value);
  if (index >= 0)
    return static_cast<size_t>(index);
  else
    throw std::out_of_range("indexOfValue - value out of range");
}

int indexOfValueFromCentersNoThrow(const std::vector<double> &bin_centers, const double value) {
  if (bin_centers.empty()) {
    throw std::out_of_range("indexOfValue - vector is empty");
  }
  if (bin_centers.size() == 1) {
    // no mean to guess bin size, assuming 1
    if (value < bin_centers[0] - 0.5 || value > bin_centers[0] + 0.5) {
      return -1;
    } else {
      return 0;
    }
  } else {
    const size_t n = bin_centers.size();
    const double firstBinLowEdge = bin_centers[0] - 0.5 * (bin_centers[1] - bin_centers[0]);
    const double lastBinHighEdge = bin_centers[n - 1] + 0.5 * (bin_centers[n - 1] - bin_centers[n - 2]);
    if (value < firstBinLowEdge || value > lastBinHighEdge) {
      return -1;
    } else {
      const auto it = std::lower_bound(bin_centers.begin(), bin_centers.end(), value);
      if (it == bin_centers.end()) {
        return static_cast<int>(n - 1);
      }
      size_t binIndex = std::distance(bin_centers.begin(), it);
      if (binIndex > 0 &&
          value < bin_centers[binIndex - 1] + 0.5 * (bin_centers[binIndex] - bin_centers[binIndex - 1])) {
        binIndex--;
      }
      return static_cast<int>(binIndex);
    }
  }
}

/** Finds the bin index of a value from the vector of bin edges.
 * Assumes the vector is already sorted ascending.
 * @param bin_edges : vector of bin centers
 * @param value : input value
 * @return : the bin index of the value
 * @throw std::out_of_range : if vector is empty, contains one element,
 *  or value is out of it's range
 */
size_t indexOfValueFromEdges(const std::vector<double> &bin_edges, const double value) {
  if (bin_edges.empty()) {
    throw std::out_of_range("indexOfValue - vector is empty");
  }
  if (bin_edges.size() == 1) {
    throw std::out_of_range("indexOfValue - requires at least two bin edges");
  }
  if (value < bin_edges.front()) {
    throw std::out_of_range("indexOfValue - value out of range");
  }
  const auto it = std::lower_bound(bin_edges.begin(), bin_edges.end(), value);
  if (it == bin_edges.end()) {
    throw std::out_of_range("indexOfValue - value out of range");
  }
  // index of closest edge above value is distance of iterator from start
  size_t edgeIndex = std::distance(bin_edges.begin(), it);
  // if the element n is the first that is >= value, then the value is in (n-1)
  // th bin
  if (edgeIndex > 0)
    edgeIndex--;
  return edgeIndex;
}

//-------------------------------------------------------------------------------------------------
/** Assess if all the values in the vector are equal or if there are some
 * different values
 *  @param[in] arra the vector to examine
 */
bool isConstantValue(const std::vector<double> &arra) {
  // make comparisons with the first value
  auto i = arra.cbegin();

  if (i == arra.cend()) { // empty array
    return true;
  }

  double val(*i);
  // this loop can be entered! NAN values make comparisons difficult because nan
  // != nan, deal with these first
  for (; val != val;) {
    ++i;
    if (i == arra.cend()) {
      // all values are contant (NAN)
      return true;
    }
    val = *i;
  }

  for (; i != arra.cend(); ++i) {
    if (*i != val) {
      return false;
    }
  }
  // no different value was found and so every must be equal to c
  return true;
}

//-------------------------------------------------------------------------------------------------
/** Take a string of comma or space-separated values, and splits it into
 * a vector of doubles.
 * @param listString :: a string like "0.0 1.2" or "2.4, 5.67, 88"
 * @return a vector of doubles
 * @throw an error if there was a string that could not convert to a double.
 */
template <typename NumT> std::vector<NumT> splitStringIntoVector(std::string listString) {
  // Split the string and turn it into a vector.
  std::vector<NumT> values;

  using split_vector_type = std::vector<std::string>;
  split_vector_type strs;

  boost::split(strs, listString, boost::is_any_of(", "));
  for (auto &str : strs) {
    if (!str.empty()) {
      // String not empty
      std::stringstream oneNumber(str);
      NumT num;
      oneNumber >> num;
      values.emplace_back(num);
    }
  }
  return values;
}

//-------------------------------------------------------------------------------------------------
/** Return the index into a vector of bin boundaries for a particular X value.
 *  The index returned is the one for the left edge of the bin.
 *  If beyond the range of the vector, it will return either 0 or bins.size()-2.
 *  @param bins  A reference to the set of bin boundaries to search. It is
 * assumed that they are
 *               monotonically increasing values and this is NOT checked
 *  @param value The value whose boundaries should be found
 */
int getBinIndex(const std::vector<double> &bins, const double value) {
  assert(bins.size() >= 2);
  // Since we cast to an int below:
  assert(bins.size() < static_cast<size_t>(std::numeric_limits<int>::max()));
  // If X is below the min value
  if (value < bins.front())
    return 0;

  // upper_bound will find the right-hand bin boundary (even if the value is
  // equal to
  // the left-hand one) - hence we subtract 1 from the found point.
  // Since we want to return the LH boundary of the last bin if the value is
  // outside
  // the upper range, we leave the last value out (i.e. bins.end()-1)
  auto it = std::upper_bound(bins.begin(), bins.end() - 1, value) - 1;
  assert(it >= bins.begin());
  // Convert an iterator to an index for the return value
  return static_cast<int>(it - bins.begin());
}

//-------------------------------------------------------------------------------------------------

namespace {
/** internal function converted from Lambda to identify interval around
 * specified  point and  run average around this point
 *
 *@param index      -- index to average around
 *@param startIndex -- index in the array of data (input to start average
 *                     from) should be: index>=startIndex>=0
 *@param endIndex   -- index in the array of data (input to end average at)
 *                     should be: index<=endIndex<=input.size()
 *@param halfWidth  -- half width of the interval to integrate.
 *@param input      -- vector of input signal
 *@param binBndrs   -- pointer to vector of bin boundaries or NULL pointer.
 */
double runAverage(size_t index, size_t startIndex, size_t endIndex, const double halfWidth,
                  const std::vector<double> &input, std::vector<double> const *const binBndrs) {

  size_t iStart, iEnd;
  double weight0(0), weight1(0), start(0.0), end(0.0);
  //
  if (binBndrs) {
    // identify initial and final bins to
    // integrate over. Notice the difference
    // between start and end bin and shift of
    // the interpolating function into the center
    // of each bin
    auto &rBndrs = *binBndrs;
    // bin0 = binBndrs->operator[](index + 1) - binBndrs->operator[](index);

    double binC = 0.5 * (rBndrs[index + 1] + rBndrs[index]);
    start = binC - halfWidth;
    end = binC + halfWidth;
    if (start <= rBndrs[startIndex]) {
      iStart = startIndex;
      start = rBndrs[iStart];
    } else {
      iStart = getBinIndex(*binBndrs, start);
      weight0 = (rBndrs[iStart + 1] - start) / (rBndrs[iStart + 1] - rBndrs[iStart]);
      iStart++;
    }
    if (end >= rBndrs[endIndex]) {
      iEnd = endIndex; // the signal defined up to i<iEnd
      end = rBndrs[endIndex];
    } else {
      iEnd = getBinIndex(*binBndrs, end);
      weight1 = (end - rBndrs[iEnd]) / (rBndrs[iEnd + 1] - rBndrs[iEnd]);
    }
    if (iStart > iEnd) { // start and end get into the same bin
      weight1 = 0;
      weight0 = (end - start) / (rBndrs[iStart] - rBndrs[iStart - 1]);
    }
  } else { // integer indexes and functions defined in the bin centers
    auto iHalfWidth = static_cast<size_t>(halfWidth);
    iStart = index - iHalfWidth;
    if (startIndex + iHalfWidth > index)
      iStart = startIndex;
    iEnd = index + iHalfWidth;
    if (iEnd > endIndex)
      iEnd = endIndex;
  }

  double avrg = 0;
  size_t ic = 0;
  for (size_t j = iStart; j < iEnd; j++) {
    avrg += input[j];
    ic++;
  }
  if (binBndrs) { // add values at edges
    if (iStart != startIndex)
      avrg += input[iStart - 1] * weight0;
    if (iEnd != endIndex)
      avrg += input[iEnd] * weight1;

    double div = end - start;
    if (.0 == div)
      return 0;
    else
      return avrg / (end - start);
  } else {
    if (0 == ic) {
      return 0;
    } else {
      return avrg / double(ic);
    }
  }
}
} // namespace

/** Basic running average of input vector within specified range, considering
 *  variable bin-boundaries if such boundaries are provided.
 * The algorithm performs trapezium integration, so some peak shift
 * related to the first derivative of the integrated function can be observed.
 *
 * @param input::   input vector to smooth
 * @param output::  resulting vector (can not coincide with input)
 * @param avrgInterval:: the interval to average function in.
 *                      the function is averaged within +-0.5*avrgInterval
 * @param binBndrs :: pointer to the vector, containing bin boundaries.
 *                    If provided, its length has to be input.size()+1,
 *                    if not, equal size bins of size 1 are assumed,
 *                    so avrgInterval becomes the number of points
 *                    to average over. Bin boundaries array have to
 *                    increase and can not contain equal boundaries.
 * @param startIndex:: if provided, its start index to run averaging from.
 *                     if not, averaging starts from the index 0
 * @param endIndex ::  final index to run average to, if provided. If
 *                     not, or higher then number of elements in input array,
 *                     averaging is performed to the end point of the input
 *                     array
 * @param outBins ::   if present, pointer to a vector to return
 *                     bin boundaries for output array.
 */
void smoothInRange(const std::vector<double> &input, std::vector<double> &output, const double avrgInterval,
                   std::vector<double> const *const binBndrs, size_t startIndex, size_t endIndex,
                   std::vector<double> *const outBins) {

  if (endIndex == 0)
    endIndex = input.size();
  if (endIndex > input.size())
    endIndex = input.size();

  if (endIndex <= startIndex) {
    output.resize(0);
    return;
  }

  size_t max_size = input.size();
  if (binBndrs) {
    if (binBndrs->size() != max_size + 1) {
      throw std::invalid_argument("Array of bin boundaries, "
                                  "if present, have to be one bigger then the input array");
    }
  }

  size_t length = endIndex - startIndex;
  output.resize(length);

  double halfWidth = avrgInterval / 2;
  if (!binBndrs) {
    if (std::floor(halfWidth) * 2 - avrgInterval > 1.e-6) {
      halfWidth = std::floor(halfWidth) + 1;
    }
  }

  if (outBins)
    outBins->resize(length + 1);

  //  Run averaging
  double binSize = 1;
  for (size_t i = startIndex; i < endIndex; i++) {
    if (binBndrs) {
      binSize = binBndrs->operator[](i + 1) - binBndrs->operator[](i);
    }
    output[i - startIndex] = runAverage(i, startIndex, endIndex, halfWidth, input, binBndrs) * binSize;
    if (outBins && binBndrs) {
      outBins->operator[](i - startIndex) = binBndrs->operator[](i);
    }
  }
  if (outBins && binBndrs) {
    outBins->operator[](endIndex - startIndex) = binBndrs->operator[](endIndex);
  }
}

/// Declare all version of this
template MANTID_KERNEL_DLL std::vector<int32_t> splitStringIntoVector<int32_t>(std::string listString);
template MANTID_KERNEL_DLL std::vector<int64_t> splitStringIntoVector<int64_t>(std::string listString);
template MANTID_KERNEL_DLL std::vector<size_t> splitStringIntoVector<size_t>(std::string listString);
template MANTID_KERNEL_DLL std::vector<float> splitStringIntoVector<float>(std::string listString);
template MANTID_KERNEL_DLL std::vector<double> splitStringIntoVector<double>(std::string listString);
template MANTID_KERNEL_DLL std::vector<std::string> splitStringIntoVector<std::string>(std::string listString);

} // namespace Mantid::Kernel::VectorHelper
