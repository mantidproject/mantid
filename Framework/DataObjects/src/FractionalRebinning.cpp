#include "MantidDataObjects/FractionalRebinning.h"

#include "MantidAPI/Progress.h"
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidGeometry/Math/PolygonIntersection.h"
#include "MantidKernel/V2D.h"

#include <boost/math/special_functions/fpclassify.hpp>

namespace Mantid {

using namespace API;
using namespace Geometry;
using namespace Kernel;

namespace DataObjects {

namespace FractionalRebinning {

/**
 * Find the possible region of intersection on the output workspace for the
 * given polygon. The given polygon must have a CLOCKWISE winding.
 * @param outputWS A pointer to the output workspace
 * @param verticalAxis A vector containing the output vertical axis edges
 * @param inputQ The input polygon (Polygon winding must be clockwise)
 * @param qstart An output giving the starting index in the Q direction
 * @param qend An output giving the end index in the Q direction
 * @param x_start An output giving the start index in the dX direction
 * @param x_end An output giving the end index in the dX direction
 * @return True if an intersecition is possible
 */
bool getIntersectionRegion(MatrixWorkspace_const_sptr outputWS,
                           const std::vector<double> &verticalAxis,
                           const Quadrilateral &inputQ, size_t &qstart,
                           size_t &qend, size_t &x_start, size_t &x_end) {
  const MantidVec &xAxis = outputWS->readX(0);
  const double xn_lo(inputQ.minX()), xn_hi(inputQ.maxX());
  const double yn_lo(inputQ.minY()), yn_hi(inputQ.maxY());

  if (xn_hi < xAxis.front() || xn_lo > xAxis.back() ||
      yn_hi < verticalAxis.front() || yn_lo > verticalAxis.back())
    return false;

  MantidVec::const_iterator start_it =
      std::upper_bound(xAxis.begin(), xAxis.end(), xn_lo);
  MantidVec::const_iterator end_it =
      std::upper_bound(xAxis.begin(), xAxis.end(), xn_hi);
  x_start = 0;
  x_end = xAxis.size() - 1;
  if (start_it != xAxis.begin()) {
    x_start = (start_it - xAxis.begin() - 1);
  }
  if (end_it != xAxis.end()) {
    x_end = end_it - xAxis.begin();
  }

  // Q region
  start_it = std::upper_bound(verticalAxis.begin(), verticalAxis.end(), yn_lo);
  end_it = std::upper_bound(verticalAxis.begin(), verticalAxis.end(), yn_hi);
  qstart = 0;
  qend = verticalAxis.size() - 1;
  if (start_it != verticalAxis.begin()) {
    qstart = (start_it - verticalAxis.begin() - 1);
  }
  if (end_it != verticalAxis.end()) {
    qend = end_it - verticalAxis.begin();
  }

  return true;
}

/**
 * Computes the square root of the errors and if the input was a distribution
 * this divides by the new bin-width
 * @param outputWS The workspace containing the output data
 * @param inputWS The input workspace used for testing distribution state
 * @param progress An optional progress object. Reported to once per bin.
 */
void normaliseOutput(MatrixWorkspace_sptr outputWS,
                     MatrixWorkspace_const_sptr inputWS,
                     boost::shared_ptr<Progress> progress) {
  for (int64_t i = 0; i < static_cast<int64_t>(outputWS->getNumberHistograms());
       ++i) {
    MantidVec &outputY = outputWS->dataY(i);
    MantidVec &outputE = outputWS->dataE(i);
    for (size_t j = 0; j < outputWS->blocksize(); ++j) {
      if (progress)
        progress->report("Calculating errors");
      const double binWidth =
          (outputWS->readX(i)[j + 1] - outputWS->readX(i)[j]);
      double eValue = std::sqrt(outputE[j]);
      // Don't do this for a RebinnedOutput workspace. The fractions
      // take care of such things.
      if (inputWS->isDistribution() && inputWS->id() != "RebinnedOutput") {
        outputY[j] /= binWidth;
        eValue /= binWidth;
      }
      outputE[j] = eValue;
    }
  }
  outputWS->isDistribution(inputWS->isDistribution());
}

/**
 * Rebin the input quadrilateral to the output grid.
 * The quadrilateral must have a CLOCKWISE winding.
 * @param inputQ The input polygon (Polygon winding must be Clockwise)
 * @param inputWS The input workspace containing the input intensity values
 * @param i The index in the vertical axis direction that inputQ references
 * @param j The index in the horizontal axis direction that inputQ references
 * @param outputWS A pointer to the output workspace that accumulates the data
 * @param verticalAxis A vector containing the output vertical axis bin
 * boundaries
 */
void rebinToOutput(const Quadrilateral &inputQ,
                   MatrixWorkspace_const_sptr inputWS, const size_t i,
                   const size_t j, MatrixWorkspace_sptr outputWS,
                   const std::vector<double> &verticalAxis) {
  const MantidVec &X = outputWS->readX(0);
  size_t qstart(0), qend(verticalAxis.size() - 1), x_start(0),
      x_end(X.size() - 1);
  if (!getIntersectionRegion(outputWS, verticalAxis, inputQ, qstart, qend,
                             x_start, x_end))
    return;

  const auto &inY = inputWS->readY(i);
  const auto &inE = inputWS->readE(i);
  // It seems to be more efficient to construct this once and clear it before
  // each calculation
  // in the loop
  ConvexPolygon intersectOverlap;
  for (size_t y = qstart; y < qend; ++y) {
    const double vlo = verticalAxis[y];
    const double vhi = verticalAxis[y + 1];
    for (size_t xi = x_start; xi < x_end; ++xi) {
      const V2D ll(X[xi], vlo);
      const V2D lr(X[xi + 1], vlo);
      const V2D ur(X[xi + 1], vhi);
      const V2D ul(X[xi], vhi);
      const Quadrilateral outputQ(ll, lr, ur, ul);

      double yValue = inY[j];
      if (boost::math::isnan(yValue)) {
        continue;
      }
      intersectOverlap.clear();
      if (intersection(outputQ, inputQ, intersectOverlap)) {
        const double weight = intersectOverlap.area() / inputQ.area();
        yValue *= weight;
        double eValue = inE[j] * weight;
        if (inputWS->isDistribution()) {
          const double overlapWidth =
              intersectOverlap.maxX() - intersectOverlap.minX();
          yValue *= overlapWidth;
          eValue *= overlapWidth;
        }
        eValue = eValue * eValue;
        PARALLEL_CRITICAL(overlap_sum) {
          outputWS->dataY(y)[xi] += yValue;
          outputWS->dataE(y)[xi] += eValue;
        }
      }
    }
  }
}

/**
 * Rebin the input quadrilateral to the output grid
 * The quadrilateral must have a CLOCKWISE winding.
 * @param inputQ The input polygon (Polygon winding must be clockwise)
 * @param inputWS The input workspace containing the input intensity values
 * @param i The indexiin the vertical axis direction that inputQ references
 * @param j The index in the horizontal axis direction that inputQ references
 * @param outputWS A pointer to the output workspace that accumulates the data
 * @param verticalAxis A vector containing the output vertical axis bin
 * boundaries
 */
void rebinToFractionalOutput(const Quadrilateral &inputQ,
                             MatrixWorkspace_const_sptr inputWS, const size_t i,
                             const size_t j, RebinnedOutput_sptr outputWS,
                             const std::vector<double> &verticalAxis) {
  const MantidVec &X = outputWS->readX(0);
  size_t qstart(0), qend(verticalAxis.size() - 1), x_start(0),
      x_end(X.size() - 1);
  if (!getIntersectionRegion(outputWS, verticalAxis, inputQ, qstart, qend,
                             x_start, x_end))
    return;

  const auto &inY = inputWS->readY(i);
  const auto &inE = inputWS->readE(i);
  // Don't do the overlap removal if already RebinnedOutput.
  // This wreaks havoc on the data.
  const bool removeOverlap(inputWS->isDistribution() &&
                           inputWS->id() != "RebinnedOutput");
  // It seems to be more efficient to construct this once and clear it before
  // each calculation
  // in the loop
  ConvexPolygon intersectOverlap;
  for (size_t yi = qstart; yi < qend; ++yi) {
    const double vlo = verticalAxis[yi];
    const double vhi = verticalAxis[yi + 1];
    for (size_t xi = x_start; xi < x_end; ++xi) {
      const V2D ll(X[xi], vlo);
      const V2D lr(X[xi + 1], vlo);
      const V2D ur(X[xi + 1], vhi);
      const V2D ul(X[xi], vhi);
      const Quadrilateral outputQ(ll, lr, ur, ul);

      double yValue = inY[j];
      if (boost::math::isnan(yValue)) {
        continue;
      }
      intersectOverlap.clear();
      if (intersection(outputQ, inputQ, intersectOverlap)) {
        const double weight = intersectOverlap.area() / inputQ.area();
        yValue *= weight;
        double eValue = inE[j] * weight;
        if (removeOverlap) {
          const double overlapWidth =
              intersectOverlap.maxX() - intersectOverlap.minX();
          yValue *= overlapWidth;
          eValue *= overlapWidth;
        }
        eValue *= eValue;
        PARALLEL_CRITICAL(overlap) {
          outputWS->dataY(yi)[xi] += yValue;
          outputWS->dataE(yi)[xi] += eValue;
          outputWS->dataF(yi)[xi] += weight;
        }
      }
    }
  }
}

} // namespace FractionalRebinning

} // namespace DataObjects
} // namespace Mantid
