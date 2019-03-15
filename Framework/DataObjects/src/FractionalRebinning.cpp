// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/FractionalRebinning.h"

#include "MantidAPI/Progress.h"
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/PolygonIntersection.h"
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidKernel/V2D.h"

#include <cmath>
#include <limits>

namespace {
struct AreaInfo {
  size_t wsIndex;
  size_t binIndex;
  double weight;
  AreaInfo(const size_t xi, const size_t yi, const double w)
      : wsIndex(yi), binIndex(xi), weight(w) {}
};
/**
 * Private function to calculate polygon area directly to avoid the overhead
 * of initializing a ConvexPolygon instance or a V2D vector. This recursive
 * implementation uses the shoelace formula but requires the last element to
 * be the same as the first element. Also note that it returns 2x the area!
 */
template <class T> double polyArea(const T &) { return 0.; }
template <class T, class... Ts>
double polyArea(const T &v1, const T &v2, Ts &&... vertices) {
  return v2.X() * v1.Y() - v2.Y() * v1.X() +
         polyArea(v2, std::forward<Ts>(vertices)...);
}
} // namespace

namespace Mantid {

using namespace API;
using namespace Geometry;
using namespace Kernel;

namespace DataObjects {

namespace FractionalRebinning {

const double POS_TOLERANCE = 1.e-10;

enum class QuadrilateralType { Rectangle, TrapezoidX, TrapezoidY, General };

/**
 * Determine the (axis-aligned) quadrilateral type of the input polygon
 * @param inputQ Input polygon (assumes vertices are in order: ll, ul, ur, lr)
 */
QuadrilateralType getQuadrilateralType(const Quadrilateral &inputQ) {
  const bool inputQHasXParallel =
      fabs(inputQ[0].Y() - inputQ[3].Y()) < POS_TOLERANCE &&
      fabs(inputQ[1].Y() - inputQ[2].Y()) < POS_TOLERANCE;
  const bool inputQHasYParallel =
      fabs(inputQ[0].X() - inputQ[1].X()) < POS_TOLERANCE &&
      fabs(inputQ[2].X() - inputQ[3].X()) < POS_TOLERANCE;
  if (inputQHasXParallel && inputQHasYParallel) {
    return QuadrilateralType::Rectangle;
  } else if (inputQHasXParallel) {
    return QuadrilateralType::TrapezoidX;
  } else if (inputQHasYParallel) {
    return QuadrilateralType::TrapezoidY;
  } else {
    return QuadrilateralType::General;
  }
}

/**
 * Find the possible region of intersection on the output workspace for the
 * given polygon. The given polygon must have a CLOCKWISE winding and the
 * first vertex must be the "lowest left" point.
 * @param xAxis A vector containing the output horizontal axis edges
 * @param verticalAxis A vector containing the output vertical axis edges
 * @param inputQ The input polygon (Polygon winding must be clockwise)
 * @param qstart An output giving the starting index in the Q direction
 * @param qend An output giving the end index in the Q direction
 * @param x_start An output giving the start index in the dX direction
 * @param x_end An output giving the end index in the dX direction
 * @return True if an intersection is possible
 */
bool getIntersectionRegion(const std::vector<double> &xAxis,
                           const std::vector<double> &verticalAxis,
                           const Quadrilateral &inputQ, size_t &qstart,
                           size_t &qend, size_t &x_start, size_t &x_end) {
  const double xn_lo(inputQ.minX()), xn_hi(inputQ.maxX());
  const double yn_lo(inputQ.minY()), yn_hi(inputQ.maxY());

  if (xn_hi < xAxis.front() || xn_lo > xAxis.back() ||
      yn_hi < verticalAxis.front() || yn_lo > verticalAxis.back())
    return false;

  auto start_it = std::upper_bound(xAxis.cbegin(), xAxis.cend(), xn_lo);
  auto end_it = std::upper_bound(start_it, xAxis.cend(), xn_hi);
  x_start = 0;
  if (start_it != xAxis.cbegin()) {
    x_start = (start_it - xAxis.cbegin() - 1);
  }
  x_end = xAxis.size() - 1;
  if (end_it != xAxis.cend()) {
    x_end = end_it - xAxis.cbegin();
  }

  // Q region
  start_it =
      std::upper_bound(verticalAxis.cbegin(), verticalAxis.cend(), yn_lo);
  end_it = std::upper_bound(start_it, verticalAxis.cend(), yn_hi);
  qstart = 0;
  if (start_it != verticalAxis.begin()) {
    qstart = (start_it - verticalAxis.begin() - 1);
  }
  qend = verticalAxis.size() - 1;
  if (end_it != verticalAxis.end()) {
    qend = end_it - verticalAxis.begin();
  }

  return true;
}

/**
 * Computes the output grid bins which intersect the input quad and their
 * overlapping areas assuming both input and output grids are rectangular
 * @param xAxis A vector containing the output horizontal axis edges
 * @param yAxis The output data vertical axis
 * @param inputQ The input quadrilateral
 * @param y_start The starting y-axis index
 * @param y_end The starting y-axis index
 * @param x_start The starting x-axis index
 * @param x_end The starting x-axis index
 * @param areaInfos Output vector of indices and areas of overlapping bins
 */
void calcRectangleIntersections(const std::vector<double> &xAxis,
                                const std::vector<double> &yAxis,
                                const Quadrilateral &inputQ,
                                const size_t y_start, const size_t y_end,
                                const size_t x_start, const size_t x_end,
                                std::vector<AreaInfo> &areaInfos) {
  std::vector<double> width;
  width.reserve(x_end - x_start);
  for (size_t xi = x_start; xi < x_end; ++xi) {
    const double x0 = (xi == x_start) ? inputQ.minX() : xAxis[xi];
    const double x1 = (xi == x_end - 1) ? inputQ.maxX() : xAxis[xi + 1];
    width.emplace_back(x1 - x0);
  }
  areaInfos.reserve((y_end - y_start) * (x_end - x_start));
  for (size_t yi = y_start; yi < y_end; ++yi) {
    const double y0 = (yi == y_start) ? inputQ.minY() : yAxis[yi];
    const double y1 = (yi == y_end - 1) ? inputQ.maxY() : yAxis[yi + 1];
    const double height = y1 - y0;
    auto width_it = width.begin();
    for (size_t xi = x_start; xi < x_end; ++xi) {
      areaInfos.emplace_back(xi, yi, height * (*width_it++));
    }
  }
}

/**
 * Computes the output grid bins which intersect the input quad and their
 * overlapping areas assuming input quad is a y-axis aligned trapezoid.
 * @param xAxis A vector containing the output horizontal axis edges
 * @param yAxis The output data vertical axis
 * @param inputQ The input quadrilateral
 * @param y_start The starting y-axis index
 * @param y_end The ending y-axis index
 * @param x_start The starting x-axis index
 * @param x_end The ending x-axis index
 * @param areaInfos Output vector of indices and areas of overlapping bins
 */
void calcTrapezoidYIntersections(const std::vector<double> &xAxis,
                                 const std::vector<double> &yAxis,
                                 const Quadrilateral &inputQ,
                                 const size_t y_start, const size_t y_end,
                                 const size_t x_start, const size_t x_end,
                                 std::vector<AreaInfo> &areaInfos) {
  // The algorithm proceeds as follows:
  // 1. Determine the left/right bin boundaries on the x- (horizontal)-grid.
  // 2. Loop along x, for each 1-output-bin wide strip construct a new input Q.
  // 3. Loop along y, in each bin determine if any vertex of the new input Q
  //    lies within the bin. Construct an overlap polygon depending on which
  //    vertices are in. The polygon will include these vertices of inputQ
  //    and left/right points previously calc.
  V2D ll = inputQ[0], ul = inputQ[1], ur = inputQ[2], lr = inputQ[3];
  const double mBot = (lr.Y() - ll.Y()) / (lr.X() - ll.X());
  const double cBot = ll.Y() - mBot * ll.X();
  const double mTop = (ur.Y() - ul.Y()) / (ur.X() - ul.X());
  const double cTop = ul.Y() - mTop * ul.X();
  // Checks that the x-edges of the input quadrilateral is in the grid
  // If not, put it on the grid line. Otherwise, get buffer overflow error
  if (ll.X() < xAxis[x_start]) {
    ll = V2D(xAxis[x_start], mBot * xAxis[x_start] + cBot);
    ul = V2D(xAxis[x_start], mTop * xAxis[x_start] + cTop);
  }
  if (lr.X() > xAxis[x_end]) {
    lr = V2D(xAxis[x_end], mBot * xAxis[x_end] + cBot);
    ur = V2D(xAxis[x_end], mTop * xAxis[x_end] + cTop);
  }

  const size_t nx = x_end - x_start;
  const size_t ny = y_end - y_start;
  const double ll_x(ll.X()), ll_y(ll.Y()), ul_y(ul.Y());
  const double lr_x(lr.X()), lr_y(lr.Y()), ur_y(ur.Y());
  // Check if there is only a output single bin and inputQ is fully enclosed
  if (nx == 1 && ny == 1 &&
      (ll_y >= yAxis[y_start] && ll_y <= yAxis[y_start + 1]) &&
      (ul_y >= yAxis[y_start] && ul_y <= yAxis[y_start + 1]) &&
      (ur_y >= yAxis[y_start] && ur_y <= yAxis[y_start + 1]) &&
      (lr_y >= yAxis[y_start] && lr_y <= yAxis[y_start + 1])) {
    areaInfos.emplace_back(x_start, y_start,
                           0.5 * polyArea(ll, ul, ur, lr, ll));
    return;
  }

  // Step 1 - construct the left/right bin lims on the lines of the y-grid.
  const double NaN = std::numeric_limits<double>::quiet_NaN();
  const double DBL_EPS = std::numeric_limits<double>::epsilon();
  std::vector<double> leftLim((nx + 1) * (ny + 1), NaN);
  std::vector<double> rightLim((nx + 1) * (ny + 1), NaN);
  auto x0_it = xAxis.begin() + x_start;
  auto x1_it = xAxis.begin() + x_end + 1;
  auto y0_it = yAxis.begin() + y_start;
  auto y1_it = yAxis.begin() + y_end + 1;
  const size_t ymax = (y_end == yAxis.size()) ? ny : (ny + 1);
  if ((mTop >= 0 && mBot >= 0) || (mTop < 0 && mBot < 0)) {
    // Diagonals in same direction: For a given x-parallel line,
    // Left limit given by one diagonal, right limit given by other
    double left_val, right_val;
    for (size_t yj = 0; yj < ymax; ++yj) {
      const size_t yjx = yj * nx;
      // First, find the far left/right limits, given by the inputQ
      if (mTop >= 0) {
        left_val = (yAxis[yj + y_start] - cTop) / mTop;
        right_val = (yAxis[yj + y_start] - cBot) / mBot;
      } else {
        left_val = (yAxis[yj + y_start] - cBot) / mBot;
        right_val = (yAxis[yj + y_start] - cTop) / mTop;
      }
      if (left_val < ll_x || left_val > lr_x)
        left_val = NaN;
      if (right_val < ll_x || right_val > lr_x)
        right_val = NaN;
      auto left_it = std::upper_bound(x0_it, x1_it, left_val);
      size_t li = 0;
      if (left_it != x1_it) {
        li = left_it - x0_it - 1;
        leftLim[li + yjx] = left_val;
      } else if (yAxis[yj + y_start] < ul_y) {
        left_it = x0_it + 1;
        leftLim[li + yjx] = ll_x;
      }
      auto right_it = std::upper_bound(x0_it, x1_it, right_val);
      if (right_it != x1_it) {
        rightLim[right_it - x0_it - 1 + yjx] = right_val;
      } else if (yAxis[yj + y_start] < ur_y && nx > 0) {
        right_it = x1_it - 1;
        rightLim[nx - 1 + yjx] = lr_x;
      }
      // Now populate the bin boundaries in between
      if (left_it < right_it && right_it != x1_it) {
        for (auto x_it = left_it; x_it != right_it; ++x_it) {
          leftLim[li + 1 + yjx] = *x_it;
          rightLim[li++ + yjx] = *x_it;
        }
      }
    }
  } else {
    // In this case, the diagonals are all on one side or the other.
    const size_t y2 =
        std::upper_bound(y0_it, y1_it, (mTop >= 0) ? ll_y : lr_y) - y0_it;
    const size_t y3 =
        std::upper_bound(y0_it, y1_it, (mTop >= 0) ? ul_y : ur_y) - y0_it;
    double val;
    for (size_t yj = 0; yj < ymax; ++yj) {
      const size_t yjx = yj * nx;
      if (yj < y2) {
        val = (yAxis[yj + y_start] - cBot) / mBot;
      } else if (yj < y3) {
        val = (mTop >= 0) ? ll_x : lr_x;
      } else {
        val = (yAxis[yj + y_start] - cTop) / mTop;
      }
      if (val < ll_x || val > lr_x)
        val = NaN;
      auto left_it =
          (mTop >= 0) ? std::upper_bound(x0_it, x1_it, val) : (x0_it + 1);
      auto right_it =
          (mTop >= 0) ? (x1_it - 1) : std::upper_bound(x0_it, x1_it, val);
      if (left_it == x1_it)
        left_it--;
      if (right_it == x1_it)
        right_it--;
      size_t li = (left_it > x0_it) ? (left_it - x0_it - 1) : 0;
      size_t ri = (right_it > x0_it) ? (right_it - x0_it - 1) : 0;
      leftLim[li + yjx] = (mTop >= 0) ? val : ll_x;
      rightLim[ri + yjx] = (mTop >= 0) ? lr_x : val;
      if (left_it < right_it && right_it != x1_it) {
        for (auto x_it = left_it; x_it != right_it; x_it++) {
          leftLim[li + 1 + yjx] = *x_it;
          rightLim[li++ + yjx] = *x_it;
        }
      }
    }
  }

  // Define constants for bitmask to indicate which vertices are in.
  const size_t LL_IN = 1 << 1;
  const size_t UL_IN = 1 << 2;
  const size_t UR_IN = 1 << 3;
  const size_t LR_IN = 1 << 4;

  // Step 2 - loop over x, creating one-bin wide strips
  V2D nll(ll), nul(ul), nur, nlr, l0, r0, l1, r1;
  double area(0.);
  ConvexPolygon poly;
  areaInfos.reserve(nx * ny);
  size_t yj0, yj1;
  for (size_t xi = x_start; xi < x_end; ++xi) {
    const size_t xj = xi - x_start;
    // Define new 1-bin wide input quadrilateral
    if (xi > x_start) {
      nll = nlr;
      nul = nur;
    }
    if (xi == (x_end - 1)) {
      nlr = lr;
      nur = ur;
    } else {
      nlr = V2D(xAxis[xi + 1], mBot * xAxis[xi + 1] + cBot);
      nur = V2D(xAxis[xi + 1], mTop * xAxis[xi + 1] + cTop);
    }
    // Step 3 - loop over y, find poly. area depending on which vertices are in
    for (size_t yi = y_start; yi < y_end; ++yi) {
      yj0 = (yi - y_start) * nx;
      yj1 = (yi - y_start + 1) * nx;
      // Checks if this bin is completely inside new quadrilateral
      if (yAxis[yi] > std::max(nll.Y(), nlr.Y()) &&
          yAxis[yi + 1] < std::min(nul.Y(), nur.Y())) {
        areaInfos.emplace_back(
            xi, yi, (nlr.X() - nll.X()) * (yAxis[yi + 1] - yAxis[yi]));
        // Checks if this bin is not completely outside new quadrilateral
      } else if (yAxis[yi + 1] >= std::min(nll.Y(), nlr.Y()) &&
                 yAxis[yi] <= std::max(nul.Y(), nur.Y())) {
        size_t vertBits = 0;
        if (nll.Y() >= yAxis[yi] && nll.Y() <= yAxis[yi + 1])
          vertBits |= LL_IN;
        if (nul.Y() >= yAxis[yi] && nul.Y() <= yAxis[yi + 1])
          vertBits |= UL_IN;
        if (nur.Y() >= yAxis[yi] && nur.Y() <= yAxis[yi + 1])
          vertBits |= UR_IN;
        if (nlr.Y() >= yAxis[yi] && nlr.Y() <= yAxis[yi + 1])
          vertBits |= LR_IN;
        l0 = V2D(leftLim[xj + yj0], yAxis[yi]);
        r0 = V2D(rightLim[xj + yj0], yAxis[yi]);
        l1 = V2D(leftLim[xj + yj1], yAxis[yi + 1]);
        r1 = V2D(rightLim[xj + yj1], yAxis[yi + 1]);
        // Now calculate the area based on which vertices are in this bin.
        // Note that a recursive function is used so it can be unrolled and
        // inlined but it means that the first element has to also be put
        // into the final position, because otherwise the recursion cannot
        // implement the shoelace formula (which is circular).
        switch (vertBits) {
        // First check cases where no vertices are in this bin
        case 0:
          area = polyArea(l1, r1, r0, l0, l1);
          break;
        // Now check cases where only one vertex is in. We either have
        // a triangle, or a pentagon depending on the diagonal slope
        case LL_IN:
          if (mBot < 0)
            area = polyArea(nll, l1, r1, r0, l0, nll);
          else
            area = polyArea(nll, l1, r1, nll);
          break;
        case UL_IN:
          if (mTop >= 0)
            area = polyArea(l1, r1, r0, l0, nul, l1);
          else
            area = polyArea(r0, l0, nul, r0);
          break;
        case UR_IN:
          if (mTop < 0)
            area = polyArea(nur, r0, l0, l1, r1, nur);
          else
            area = polyArea(nur, r0, l0, nur);
          break;
        case LR_IN:
          if (mBot >= 0)
            area = polyArea(r0, l0, l1, r1, nlr, r0);
          else
            area = polyArea(l1, r1, nlr, l1);
          break;
        // Now check cases where two vertices are in.
        case (LL_IN | UL_IN):
          if (mTop >= 0) {
            if (mBot < 0)
              area = polyArea(nll, nul, l1, r1, r0, l0, nll);
            else
              area = polyArea(nll, nul, l1, r1, nll);
          } else if (mBot < 0) {
            area = polyArea(nll, nul, r0, l0, nll);
          }
          break;
        case (UR_IN | LR_IN):
          if (mBot >= 0) {
            if (mTop < 0)
              area = polyArea(nur, nlr, r0, l0, l1, r1, nur);
            else
              area = polyArea(nur, nlr, r0, l0, nur);
          } else if (mTop < 0) {
            area = polyArea(nur, nlr, l1, r1, nur);
          }
          break;
        case (UL_IN | UR_IN):
          area = polyArea(nul, nur, r0, l0, nul);
          break;
        case (LL_IN | LR_IN):
          area = polyArea(nlr, nll, l1, r1, nlr);
          break;
        case (LL_IN | UR_IN):
          area = polyArea(nll, l1, r1, nur, r0, l0, nll);
          break;
        case (UL_IN | LR_IN):
          area = polyArea(nul, l1, r1, nlr, r0, l0, nul);
          break;
        // Now check cases where three vertices are in.
        case (UL_IN | UR_IN | LR_IN):
          area = polyArea(nul, nur, nlr, r0, l0, nul);
          break;
        case (LL_IN | UR_IN | LR_IN):
          area = polyArea(nll, l1, r1, nur, nlr, nll);
          break;
        case (LL_IN | UL_IN | LR_IN):
          area = polyArea(nlr, nll, nul, l1, r1, nlr);
          break;
        case (LL_IN | UL_IN | UR_IN):
          area = polyArea(nul, nur, r0, l0, nll, nul);
          break;
        // Finally, the case where all vertices are in.
        case (LL_IN | UL_IN | UR_IN | LR_IN):
          area = polyArea(nll, nul, nur, nlr, nll);
          break;
        }
        if (area > DBL_EPS)
          areaInfos.emplace_back(xi, yi, 0.5 * area);
      }
    }
  }
}

/**
 * Computes the output grid bins which intersect the input quad and their
 * overlapping areas for arbitrary shaped input grids
 * @param xAxis A vector containing the output horizontal axis edges
 * @param yAxis The output data vertical axis
 * @param inputQ The input quadrilateral
 * @param qstart The starting y-axis index
 * @param qend The starting y-axis index
 * @param x_start The starting x-axis index
 * @param x_end The starting x-axis index
 * @param areaInfos Output vector of indices and areas of overlapping bins
 */
void calcGeneralIntersections(const std::vector<double> &xAxis,
                              const std::vector<double> &yAxis,
                              const Quadrilateral &inputQ, const size_t qstart,
                              const size_t qend, const size_t x_start,
                              const size_t x_end,
                              std::vector<AreaInfo> &areaInfos) {
  ConvexPolygon intersectOverlap;
  areaInfos.reserve((qend - qstart) * (x_end - x_start));
  for (size_t yi = qstart; yi < qend; ++yi) {
    const double vlo = yAxis[yi];
    const double vhi = yAxis[yi + 1];
    for (size_t xi = x_start; xi < x_end; ++xi) {
      intersectOverlap.clear();
      if (intersection(
              Quadrilateral(V2D(xAxis[xi], vlo), V2D(xAxis[xi + 1], vlo),
                            V2D(xAxis[xi + 1], vhi), V2D(xAxis[xi], vhi)),
              inputQ, intersectOverlap)) {
        areaInfos.emplace_back(xi, yi, intersectOverlap.area());
      }
    }
  }
}

/**
 * Computes the square root of the errors and if the input was a distribution
 * this divides by the new bin-width
 * @param outputWS The workspace containing the output data
 * @param inputWS The input workspace used for testing distribution state
 * @param progress An optional progress object. Reported to once per bin.
 */
void normaliseOutput(MatrixWorkspace_sptr outputWS,
                     MatrixWorkspace_const_sptr inputWS, Progress *progress) {
  const bool removeBinWidth(inputWS->isDistribution() &&
                            outputWS->id() != "RebinnedOutput");
  for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
    const auto &outputX = outputWS->x(i);
    auto &outputY = outputWS->mutableY(i);
    auto &outputE = outputWS->mutableE(i);
    if (progress)
      progress->report("Calculating errors");
    for (size_t j = 0; j < outputY.size(); ++j) {
      double eValue = std::sqrt(outputE[j]);
      if (removeBinWidth) {
        const double binWidth = outputX[j + 1] - outputX[j];
        outputY[j] /= binWidth;
        eValue /= binWidth;
      }
      outputE[j] = eValue;
    }
  }
  outputWS->setDistribution(inputWS->isDistribution());
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
                   const MatrixWorkspace_const_sptr &inputWS, const size_t i,
                   const size_t j, MatrixWorkspace &outputWS,
                   const std::vector<double> &verticalAxis) {
  const auto &inY = inputWS->y(i);
  // Check once whether the signal
  if (std::isnan(inY[j])) {
    return;
  }

  const auto &X = outputWS.x(0).rawData();
  size_t qstart(0), qend(verticalAxis.size() - 1), x_start(0),
      x_end(X.size() - 1);
  if (!getIntersectionRegion(X, verticalAxis, inputQ, qstart, qend, x_start,
                             x_end))
    return;

  const auto &inE = inputWS->e(i);
  // It seems to be more efficient to construct this once and clear it before
  // each calculation in the loop
  ConvexPolygon intersectOverlap;
  for (size_t y = qstart; y < qend; ++y) {
    const double vlo = verticalAxis[y];
    const double vhi = verticalAxis[y + 1];
    for (size_t xi = x_start; xi < x_end; ++xi) {
      intersectOverlap.clear();
      if (intersection(Quadrilateral(V2D(X[xi], vlo), V2D(X[xi + 1], vlo),
                                     V2D(X[xi + 1], vhi), V2D(X[xi], vhi)),
                       inputQ, intersectOverlap)) {
        const double overlapArea = intersectOverlap.area();
        if (overlapArea == 0.) {
          continue;
        }
        const double weight = overlapArea / inputQ.area();
        double yValue = inY[j];
        yValue *= weight;
        double eValue = inE[j];
        if (inputWS->isDistribution()) {
          const double overlapWidth =
              intersectOverlap.maxX() - intersectOverlap.minX();
          yValue *= overlapWidth;
          eValue *= overlapWidth;
        }
        eValue = eValue * eValue * weight;
        PARALLEL_CRITICAL(overlap_sum) {
          // The mutable calls must be in the critical section
          // so that any calls from omp sections can write to the
          // output workspace safely
          outputWS.mutableY(y)[xi] += yValue;
          outputWS.mutableE(y)[xi] += eValue;
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
 *        Note that the error array of the output workspace contains the
 *        **variance** and not the errors (standard deviations).
 * @param verticalAxis A vector containing the output vertical axis bin
 * boundaries
 * @param inputRB A pointer, of RebinnedOutput type, to the input workspace.
 * It is used to take into account the input area fractions when calcuting
 * the final output fractions.
 * This can be null to indicate that the input was a standard 2D workspace.
 */
void rebinToFractionalOutput(const Quadrilateral &inputQ,
                             const MatrixWorkspace_const_sptr &inputWS,
                             const size_t i, const size_t j,
                             RebinnedOutput &outputWS,
                             const std::vector<double> &verticalAxis,
                             const RebinnedOutput_const_sptr &inputRB) {
  const auto &inX = inputWS->x(i);
  const auto &inY = inputWS->y(i);
  const auto &inE = inputWS->e(i);
  double signal = inY[j];
  if (std::isnan(signal))
    return;

  const auto &X = outputWS.x(0).rawData();
  size_t qstart(0), qend(verticalAxis.size() - 1), x_start(0),
      x_end(X.size() - 1);
  if (!getIntersectionRegion(X, verticalAxis, inputQ, qstart, qend, x_start,
                             x_end))
    return;

  // If the input workspace was normalized by the bin width, we need to
  // recover the original Y value, we do it by 'removing' the bin width
  // Don't do the overlap removal if already RebinnedOutput.
  // This wreaks havoc on the data.
  double error = inE[j];
  double inputWeight = 1.;
  if (inputWS->isDistribution() && !inputRB) {
    const double overlapWidth = inX[j + 1] - inX[j];
    signal *= overlapWidth;
    error *= overlapWidth;
    inputWeight = overlapWidth;
  }

  // The intersection overlap algorithm is relatively costly. The outputQ is
  // defined as rectangular. If the inputQ is is also rectangular or
  // trapezoidal, a simpler/faster way of calculating the intersection area
  // of all or some bins can be used.
  std::vector<AreaInfo> areaInfos;
  const double inputQArea = inputQ.area();
  const QuadrilateralType inputQType = getQuadrilateralType(inputQ);
  if (inputQType == QuadrilateralType::Rectangle) {
    calcRectangleIntersections(X, verticalAxis, inputQ, qstart, qend, x_start,
                               x_end, areaInfos);
  } else if (inputQType == QuadrilateralType::TrapezoidY) {
    calcTrapezoidYIntersections(X, verticalAxis, inputQ, qstart, qend, x_start,
                                x_end, areaInfos);
  } else {
    calcGeneralIntersections(X, verticalAxis, inputQ, qstart, qend, x_start,
                             x_end, areaInfos);
  }

  // If the input is a RebinnedOutput workspace with frac. area we need
  // to account for the weight of the input bin in the output bin weights
  if (inputRB) {
    const auto &inF = inputRB->dataF(i);
    inputWeight = inF[j];
    // If the signal/error has been "finalized" (scaled by 1/inF) then
    // we need to undo this before carrying on.
    if (inputRB->isFinalized()) {
      signal *= inF[j];
      error *= inF[j];
    }
  }

  const double variance = error * error;
  for (const auto &ai : areaInfos) {
    if (ai.weight == 0.) {
      continue;
    }
    const double weight = ai.weight / inputQArea;
    PARALLEL_CRITICAL(overlap) {
      // The mutable calls must be in the critical section
      // so that any calls from omp sections can write to the
      // output workspace safely
      outputWS.mutableY(ai.wsIndex)[ai.binIndex] += signal * weight;
      outputWS.mutableE(ai.wsIndex)[ai.binIndex] += variance * weight;
      outputWS.dataF(ai.wsIndex)[ai.binIndex] += weight * inputWeight;
    }
  }
}

} // namespace FractionalRebinning

} // namespace DataObjects
} // namespace Mantid
