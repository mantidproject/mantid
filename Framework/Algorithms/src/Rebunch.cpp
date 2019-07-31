// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Rebunch.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidKernel/BoundedValidator.h"

#include <cmath>
#include <numeric>
#include <sstream>

namespace Mantid {
using namespace HistogramData;
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Rebunch)

using namespace Kernel;
using API::MatrixWorkspace;
using API::MatrixWorkspace_const_sptr;
using API::WorkspaceProperty;

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void Rebunch::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The result of rebinning");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(1);
  declareProperty("NBunch", 1, mustBePositive,
                  "The number of bins that will be summed in each bunch");
}

/** Executes the rebin algorithm
 *
 *  @throw runtime_error Thrown if
 */
void Rebunch::exec() {
  // retrieve the properties
  int n_bunch = getProperty("NBunch");

  // Get the input workspace
  MatrixWorkspace_const_sptr inputW = getProperty("InputWorkspace");

  bool dist = inputW->isDistribution();

  // workspace independent determination of length
  auto histnumber = static_cast<int>(inputW->size() / inputW->blocksize());

  auto size_x = static_cast<int>(inputW->x(0).size());
  auto size_y = static_cast<int>(inputW->y(0).size());

  // signal is the same length for histogram and point data
  int ny = (size_y / n_bunch);
  if (size_y % n_bunch > 0)
    ny += 1;
  // default is for hist
  int nx = ny + 1;
  bool point = false;
  if (size_x == size_y) {
    point = true;
    nx = ny;
  }

  // make output Workspace the same type is the input, but with new length of
  // signal array
  API::MatrixWorkspace_sptr outputW =
      API::WorkspaceFactory::Instance().create(inputW, histnumber, nx, ny);

  int progress_step = histnumber / 100;
  if (progress_step == 0)
    progress_step = 1;
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputW, *outputW))
  for (int hist = 0; hist < histnumber; hist++) {
    PARALLEL_START_INTERUPT_REGION
    // output data arrays are implicitly filled by function
    if (point) {
      rebunch_point(inputW->x(hist), inputW->y(hist), inputW->e(hist),
                    outputW->mutableX(hist), outputW->mutableY(hist),
                    outputW->mutableE(hist), n_bunch);
    } else if (dist) {
      rebunch_hist_frequencies(inputW->x(hist), inputW->y(hist),
                               inputW->e(hist), outputW->mutableX(hist),
                               outputW->mutableY(hist), outputW->mutableE(hist),
                               n_bunch);
    } else {
      rebunch_hist_counts(inputW->x(hist), inputW->y(hist), inputW->e(hist),
                          outputW->mutableX(hist), outputW->mutableY(hist),
                          outputW->mutableE(hist), n_bunch);
    }

    if (hist % progress_step == 0) {
      progress(double(hist) / histnumber);
      interruption_point();
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  outputW->setDistribution(dist);

  // Copy units
  if (outputW->getAxis(0)->unit().get())
    outputW->getAxis(0)->unit() = inputW->getAxis(0)->unit();
  try {
    if (inputW->getAxis(1)->unit().get())
      outputW->getAxis(1)->unit() = inputW->getAxis(1)->unit();
  } catch (Exception::IndexError &) {
    // OK, so this isn't a Workspace2D
  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputW);
}

/** Rebunches histogram data data according to n_bunch input
 *
 * @param xold :: old x data
 * @param yold :: old y data
 * @param eold :: old e data
 * @param xnew :: new x data
 * @param ynew :: new y data
 * @param enew :: new e data
 * @param n_bunch :: number of data points to bunch together for each new point
 * @throw runtime_error Thrown if algorithm cannot execute
 * @throw invalid_argument Thrown if input to function is incorrect
 **/
void Rebunch::rebunch_hist_counts(const HistogramX &xold,
                                  const HistogramY &yold,
                                  const HistogramE &eold, HistogramX &xnew,
                                  HistogramY &ynew, HistogramE &enew,
                                  const size_t n_bunch) {
  size_t size_x = xold.size();
  size_t size_y = yold.size();

  double ysum, esum;
  size_t hi_index = size_x - 1;
  size_t wbins = size_y / n_bunch;
  size_t rem = size_y % n_bunch;

  size_t i, j;
  int i_in = 0;
  j = 0;
  while (j < wbins) {
    ysum = 0.0;
    esum = 0.0;
    for (i = 1; i <= n_bunch; i++) {
      ysum += yold[i_in];
      esum += eold[i_in] * eold[i_in];
      i_in++;
    }
    // average contributing x values
    ynew[j] = ysum;
    enew[j] = sqrt(esum);
    j++;
  }
  if (rem != 0) {
    ysum = 0.0;
    esum = 0.0;
    for (i = 1; i <= rem; i++) {
      ysum += yold[i_in];
      esum += eold[i_in] * eold[i_in];
      i_in++;
    }
    ynew[j] = ysum;
    enew[j] = sqrt(esum);
  }

  j = 0;
  xnew[j] = xold[0];
  j++;
  for (i = n_bunch; i < hi_index; i += n_bunch) {
    xnew[j] = xold[i];
    j++;
  }
  xnew[j] = xold[hi_index];
}

/** Rebunches histogram data data according to n_bunch input
 *
 * @param xold :: old x data
 * @param yold :: old y data
 * @param eold :: old e data
 * @param xnew :: new x data
 * @param ynew :: new y data
 * @param enew :: new e data
 * @param n_bunch :: number of data points to bunch together for each new point
 * @throw runtime_error Thrown if algorithm cannot execute
 * @throw invalid_argument Thrown if input to function is incorrect
 **/
void Rebunch::rebunch_hist_frequencies(const HistogramX &xold,
                                       const HistogramY &yold,
                                       const HistogramE &eold, HistogramX &xnew,
                                       HistogramY &ynew, HistogramE &enew,
                                       const size_t n_bunch) {
  double width;
  size_t size_x = xold.size();
  size_t size_y = yold.size();

  double ysum, esum;
  size_t hi_index = size_x - 1;
  size_t wbins = size_y / n_bunch;
  size_t rem = size_y % n_bunch;

  size_t i, j;
  int i_in = 0;
  j = 0;
  while (j < wbins) {
    ysum = 0.0;
    esum = 0.0;
    for (i = 1; i <= n_bunch; i++) {
      width = xold[i_in + 1] - xold[i_in];
      ysum += yold[i_in] * width;
      esum += eold[i_in] * eold[i_in] * width * width;
      i_in++;
    }
    // average contributing x values
    ynew[j] = ysum;
    enew[j] = sqrt(esum);
    j++;
  }
  if (rem != 0) {
    ysum = 0.0;
    esum = 0.0;
    for (i = 1; i <= rem; i++) {
      width = xold[i_in + 1] - xold[i_in];
      ysum += yold[i_in] * width;
      esum += eold[i_in] * eold[i_in] * width * width;
      i_in++;
    }
    ynew[j] = ysum;
    enew[j] = sqrt(esum);
  }

  j = 0;
  xnew[j] = xold[0];
  j++;
  for (i = n_bunch; i < hi_index; i += n_bunch) {
    xnew[j] = xold[i];
    j++;
  }
  xnew[j] = xold[hi_index];

  for (i = 0; i < ynew.size(); i++) {
    width = xnew[i + 1] - xnew[i];
    ynew[i] = ynew[i] / width;
    enew[i] = enew[i] / width;
  }
}

/** Rebunches point data data according to n_bunch input
 *
 * @param xold :: old x data
 * @param yold :: old y data
 * @param eold :: old e data
 * @param xnew :: new x data
 * @param ynew :: new y data
 * @param enew :: new e data
 * @param n_bunch :: number of data points to bunch together for each new point
 * @throw runtime_error Thrown if algorithm cannot execute
 * @throw invalid_argument Thrown if input to function is incorrect
 **/
void Rebunch::rebunch_point(const HistogramX &xold, const HistogramY &yold,
                            const HistogramE &eold, HistogramX &xnew,
                            HistogramY &ynew, HistogramE &enew,
                            const size_t n_bunch) {

  size_t size_y = yold.size();
  double xsum, ysum, esum;
  size_t wbins = size_y / n_bunch;
  size_t rem = size_y % n_bunch;

  size_t i, j;
  int i_in = 0;
  j = 0;
  while (j < wbins) {
    xsum = 0.0;
    ysum = 0.0;
    esum = 0.0;
    for (i = 1; i <= n_bunch; i++) {
      xsum += xold[i_in];
      ysum += yold[i_in];
      esum += eold[i_in] * eold[i_in];
      i_in++;
    }
    // average contributing x values
    xnew[j] = xsum / static_cast<double>(n_bunch);
    ynew[j] = ysum / static_cast<double>(n_bunch);
    enew[j] = sqrt(esum) / static_cast<double>(n_bunch);
    j++;
  }
  if (rem != 0) {
    xsum = 0.0;
    ysum = 0.0;
    esum = 0.0;
    for (i = 1; i <= rem; i++) {
      xsum += xold[i_in];
      ysum += yold[i_in];
      esum += eold[i_in] * eold[i_in];
      i_in++;
    }
    xnew[j] = xsum / static_cast<double>(rem);
    ynew[j] = ysum / static_cast<double>(rem);
    enew[j] = sqrt(esum) / static_cast<double>(rem);
  }
}

} // namespace Algorithms
} // namespace Mantid
