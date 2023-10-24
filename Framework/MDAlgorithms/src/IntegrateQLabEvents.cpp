// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/IntegrateQLabEvents.h"
#include "MantidDataObjects/NoShape.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiThreaded.h"

#include <boost/math/special_functions/round.hpp>
#include <cmath>
#include <fstream>
#include <memory>
#include <numeric>
#include <tuple>

extern "C" {
#include <cstdio>
#include <utility>

#include <gsl/gsl_eigen.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
}

using namespace Mantid::DataObjects;
namespace Mantid {
using Mantid::API::Progress;

namespace MDAlgorithms {

using namespace std;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::V3D;

namespace {
/// static logger
Kernel::Logger g_log("CostFuncUnweightedLeastSquares");
} // namespace

IntegrateQLabEvents::IntegrateQLabEvents(const SlimEvents &peak_q_list, double radius,
                                         const bool useOnePercentBackgroundCorrection)
    : m_radius(radius), m_useOnePercentBackgroundCorrection(useOnePercentBackgroundCorrection) {
  m_cellSize = m_radius;
  for (size_t peakIndex = 0; peakIndex != peak_q_list.size(); ++peakIndex) {
    const V3D q = peak_q_list[peakIndex].second;
    CellCoords abc(q, m_cellSize);
    // abc = [0, 0, 0] is no scattering
    if (!abc.isOrigin()) {
      SlimEvents events; // empty list
      OccupiedCell c = {peakIndex, q, events};
      std::pair<size_t, OccupiedCell> newCell(abc.getHash(), c);
      m_cellsWithPeaks.insert(newCell);
    }
  }
}

/**
 * @brief Set peak integration radius
 * @param radius :: double integration radius. radius must be larger than 0.
 */
void IntegrateQLabEvents::setRadius(const double &radius) { m_radius = radius; }

bool IntegrateQLabEvents::isOrigin(const V3D &q, const double &cellSize) {
  int64_t a(static_cast<int64_t>(q[0] / cellSize));
  int64_t b(static_cast<int64_t>(q[1] / cellSize));
  int64_t c(static_cast<int64_t>(q[2] / cellSize));
  return !(a || b || c);
}

void IntegrateQLabEvents::addEvents(SlimEvents const &event_qs) {
  for (const auto &event_q : event_qs)
    addEvent(event_q);
}

// Entry function that perform the integration
Mantid::Geometry::PeakShape_const_sptr
IntegrateQLabEvents::ellipseIntegrateEvents(const std::vector<V3D> &E1Vec, V3D const &peak_q, bool specify_size,
                                            double peak_radius, double back_inner_radius, double back_outer_radius,
                                            std::vector<double> &axes_radii, double &inti, double &sigi,
                                            std::pair<double, double> &backi) {
  inti = 0.0; // default values, in case something
  sigi = 0.0; // is wrong with the peak.
  backi = std::make_pair<double, double>(0.0, 0.0);

  int64_t hash = CellCoords(peak_q, m_cellSize).getHash();
  auto cell_it = m_cellsWithPeaks.find(hash);
  if (cell_it == m_cellsWithPeaks.end())
    return std::make_shared<NoShape>(); // peak_q is [0, 0, 0]
  OccupiedCell cell = cell_it->second;
  const SlimEvents &some_events = cell.events;
  if (some_events.size() < 3)
    return std::make_shared<NoShape>();

  DblMatrix cov_matrix(3, 3);
  makeCovarianceMatrix(some_events, cov_matrix, m_radius);

  std::vector<V3D> eigen_vectors;
  std::vector<double> eigen_values;
  getEigenVectors(cov_matrix, eigen_vectors, eigen_values);

  std::vector<double> sigmas(3);
  for (int i = 0; i < 3; i++)
    sigmas[i] = sqrt(eigen_values[i]);

  bool invalid_peak =
      std::any_of(sigmas.cbegin(), sigmas.cend(), [](const double sigma) { return std::isnan(sigma) || sigma <= 0; });

  // if data collapses to a line or plane, the ellipsoid volume is zero
  if (invalid_peak)
    return std::make_shared<NoShape>();

  return ellipseIntegrateEvents(E1Vec, peak_q, some_events, eigen_vectors, sigmas, specify_size, peak_radius,
                                back_inner_radius, back_outer_radius, axes_radii, inti, sigi, backi);
}

std::pair<double, double> IntegrateQLabEvents::numInEllipsoid(SlimEvents const &events,
                                                              std::vector<V3D> const &directions,
                                                              std::vector<double> const &sizes) {
  std::pair<double, double> count(0, 0);
  for (const auto &event : events) {
    double sum = 0;
    for (size_t k = 0; k < 3; k++) {
      double comp = event.second.scalar_prod(directions[k]) / sizes[k];
      sum += comp * comp;
    }
    if (sum <= 1) {
      count.first += event.first.first;   // count
      count.second += event.first.second; // error squared (add in quadrature)
    }
  }
  return count;
}

std::pair<double, double> IntegrateQLabEvents::numInEllipsoidBkg(SlimEvents const &events,
                                                                 std::vector<V3D> const &directions,
                                                                 std::vector<double> const &sizes,
                                                                 std::vector<double> const &sizesIn,
                                                                 const bool useOnePercentBackgroundCorrection) {
  std::pair<double, double> count(0, 0);
  std::vector<std::pair<double, double>> eventVec;
  for (const auto &event : events) {
    double sum = 0;
    double sumIn = 0;
    for (size_t k = 0; k < 3; k++) {
      double comp = event.second.scalar_prod(directions[k]) / sizes[k];
      sum += comp * comp;
      comp = event.second.scalar_prod(directions[k]) / sizesIn[k];
      sumIn += comp * comp;
    }
    if (sum <= 1 && sumIn >= 1)
      eventVec.emplace_back(event.first);
  }

  // NOTE:
  //  [for SNS only]
  //  Some event has weight great than 1, which is corrected using the following prunning
  //  by removing the top 1% events with higher weights.
  //  It is worth pointing out that this pruning is (to the best) an rough estimate as it
  //  will most likely either over-prunning (remove some events with weight of 1) or under-
  //  pruning (did not remove all events with weights greater than 1).
  auto endIndex = eventVec.size();
  if (useOnePercentBackgroundCorrection) {
    // Remove top 1%
    std::sort(eventVec.begin(), eventVec.end(),
              [](const std::pair<double, double> &a, const std::pair<double, double> &b) { return a.first < b.first; });
    endIndex = static_cast<size_t>(0.99 * static_cast<double>(endIndex));
  }

  for (size_t k = 0; k < endIndex; ++k) {
    count.first += eventVec[k].first;
    count.second += eventVec[k].second;
  }

  return count;
}

void IntegrateQLabEvents::makeCovarianceMatrix(SlimEvents const &events, DblMatrix &matrix, double radius) {
  double totalCounts;
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      totalCounts = 0;
      double sum = 0;
      for (const auto &event : events) {
        if (event.second.norm() <= radius) {
          totalCounts += event.first.first;
          sum += event.first.first * event.second[row] * event.second[col];
        }
      }
      if (totalCounts > 1)
        matrix[row][col] = sum / (totalCounts - 1);
      else
        matrix[row][col] = sum;
    }
  }
}

void IntegrateQLabEvents::getEigenVectors(DblMatrix const &cov_matrix, std::vector<V3D> &eigen_vectors,
                                          std::vector<double> &eigen_values) {
  unsigned int size = 3;

  gsl_matrix *matrix = gsl_matrix_alloc(size, size);
  gsl_vector *eigen_val = gsl_vector_alloc(size);
  gsl_matrix *eigen_vec = gsl_matrix_alloc(size, size);
  gsl_eigen_symmv_workspace *wkspace = gsl_eigen_symmv_alloc(size);

  // copy the matrix data into the gsl matrix
  for (size_t row = 0; row < size; row++)
    for (size_t col = 0; col < size; col++) {
      gsl_matrix_set(matrix, row, col, cov_matrix[row][col]);
    }

  gsl_eigen_symmv(matrix, eigen_val, eigen_vec, wkspace);

  // copy the resulting eigen vectors to output vector
  for (size_t col = 0; col < size; col++) {
    eigen_vectors.emplace_back(gsl_matrix_get(eigen_vec, 0, col), gsl_matrix_get(eigen_vec, 1, col),
                               gsl_matrix_get(eigen_vec, 2, col));
    eigen_values.emplace_back(gsl_vector_get(eigen_val, col));
  }

  gsl_matrix_free(matrix);
  gsl_vector_free(eigen_val);
  gsl_matrix_free(eigen_vec);
  gsl_eigen_symmv_free(wkspace);
}

void IntegrateQLabEvents::addEvent(const SlimEvent event) {
  V3D q(event.second);
  CellCoords abc(q, m_cellSize);
  if (abc.isOrigin())
    return;
  int64_t hash = abc.getHash();
  auto cell_it = m_cellsWithEvents.find(hash);
  if (cell_it == m_cellsWithEvents.end()) { // create a new cell
    SlimEvents events = {event};
    std::pair<size_t, SlimEvents> newCell(hash, events);
    m_cellsWithEvents.insert(newCell);
  } else
    cell_it->second.emplace_back(event);
}

PeakShapeEllipsoid_const_sptr
IntegrateQLabEvents::ellipseIntegrateEvents(const std::vector<V3D> &E1Vec, V3D const &peak_q, SlimEvents const &ev_list,
                                            std::vector<V3D> const &directions, std::vector<double> const &sigmas,
                                            bool specify_size, double peak_radius, double back_inner_radius,
                                            double back_outer_radius, std::vector<double> &axes_radii, double &inti,
                                            double &sigi, std::pair<double, double> &backi) {
  /* r1, r2 and r3 will give the sizes of the major axis of the peak
   * ellipsoid, and of the inner and outer surface of the background
   * ellipsoidal shell, respectively.
   * They will specify the size as the number of standard deviations
   * in the direction of each of the principal axes that the ellipsoid
   * will extend from the center. */
  double r1, r2, r3;

  double max_sigma = sigmas[0];
  for (int i = 1; i < 3; i++) {
    if (sigmas[i] > max_sigma) {
      max_sigma = sigmas[i];
    }
  }

  if (specify_size) {
    /* scale specified sizes by 1/max_sigma so when multiplied by the
     * individual sigmas in different directions, the major axis has
     * the specified size */
    r1 = peak_radius / max_sigma;
    r2 = back_inner_radius / max_sigma;
    r3 = back_outer_radius / max_sigma;
  } else {
    r1 = 3;
    r2 = 3;
    r3 = r2 * 1.25992105; // A factor of 2 ^ (1/3) will make the background
    /* shell volume equal to the peak region volume. If necessary,
     * restrict the background ellipsoid to lie within the specified
     * sphere, and adjust the other sizes, proportionally */
    if (r3 * max_sigma > m_radius) {
      r3 = m_radius / max_sigma;
      r1 = r3 * 0.79370053f; // This value for r1 and r2 makes the background
      r2 = r1;               // shell volume equal to the peak region volume.
    }
  }

  axes_radii.clear();
  std::vector<double> abcBackgroundOuterRadii;
  std::vector<double> abcBackgroundInnerRadii;
  std::vector<double> abcRadii;
  for (int i = 0; i < 3; i++) {
    abcBackgroundOuterRadii.emplace_back(r3 * sigmas[i]);
    abcBackgroundInnerRadii.emplace_back(r2 * sigmas[i]);
    abcRadii.emplace_back(r1 * sigmas[i]);
    axes_radii.emplace_back(r1 * sigmas[i]);
  }

  if (!E1Vec.empty()) {
    double h3 = 1.0 - detectorQ(E1Vec, peak_q, abcBackgroundOuterRadii);
    // scaled from area of circle minus segment when r normalized to 1
    double m3 = std::sqrt(1.0 - (std::acos(1.0 - h3) - (1.0 - h3) * std::sqrt(2.0 * h3 - h3 * h3)) / M_PI);
    double h1 = 1.0 - detectorQ(E1Vec, peak_q, axes_radii);
    // Do not use peak if edge of detector is inside integration radius
    if (h1 > 0.0)
      return std::make_shared<const PeakShapeEllipsoid>(directions, abcRadii, abcBackgroundInnerRadii,
                                                        abcBackgroundOuterRadii, Mantid::Kernel::QLab,
                                                        "IntegrateEllipsoids");
    r3 *= m3;
    if (r2 != r1) {
      double h2 = 1.0 - detectorQ(E1Vec, peak_q, abcBackgroundInnerRadii);
      // scaled from area of circle minus segment when r normalized to 1
      double m2 = std::sqrt(1.0 - (std::acos(1.0 - h2) - (1.0 - h2) * std::sqrt(2.0 * h2 - h2 * h2)) / M_PI);
      r2 *= m2;
    }
  }

  std::pair<double, double> backgrd = numInEllipsoidBkg(ev_list, directions, abcBackgroundOuterRadii,
                                                        abcBackgroundInnerRadii, m_useOnePercentBackgroundCorrection);

  std::pair<double, double> peak_w_back = numInEllipsoid(ev_list, directions, axes_radii);

  double ratio = pow(r1, 3) / (pow(r3, 3) - pow(r2, 3));
  if (r3 == r2) {
    // special case if background radius = peak radius, force background to zero
    ratio = 1.0;
    backgrd.first = 0.0;
    backgrd.second = 0.0;
  }

  inti = peak_w_back.first - ratio * backgrd.first;
  sigi = sqrt(peak_w_back.second + ratio * ratio * backgrd.second);
  backi = std::make_pair<double, double>(backgrd.first * ratio, backgrd.second * ratio * ratio);

  if (inti < 0) {
    std::ostringstream msg;
    msg << "Negative intensity found: " << inti << "\n"
        << "Please use slice viewer to check the peak with negative intensity to decide:\n"
        << "-- adjust peak and background raidus\n"
        << "-- prune false positive indexation results\n";
    g_log.notice() << msg.str();
    // debug message
    std::ostringstream debugmsg;
    debugmsg << "peak_radius = " << peak_radius << "\n"
             << "back_inner_radius = " << back_inner_radius << "\n"
             << "back_outer_radius = " << back_outer_radius << "\n"
             << "sigmas = (" << sigmas[0] << "," << sigmas[1] << "," << sigmas[2] << ")\n"
             << "r1, r2, r3 = " << r1 << "," << r2 << "," << r3 << "\n"
             << "peak_w_back.first = " << peak_w_back.first << "\n"
             << "backgrd.first = " << backgrd.first << "\n"
             << "ratio = " << ratio << "\n"
             << "inti = peak_w_back.first - ratio * backgrd.first = " << inti << "\n";
    g_log.debug() << debugmsg.str();
  }

  // Make the shape and return it.
  return std::make_shared<const PeakShapeEllipsoid>(directions, abcRadii, abcBackgroundInnerRadii,
                                                    abcBackgroundOuterRadii, Mantid::Kernel::QLab,
                                                    "IntegrateEllipsoids");
}
double IntegrateQLabEvents::detectorQ(const std::vector<V3D> &E1Vec, const Kernel::V3D &QLabFrame,
                                      const std::vector<double> &r) {
  double quot = 1.0;
  for (const auto &E1 : E1Vec) {
    V3D distv = QLabFrame - E1 * (QLabFrame.scalar_prod(E1)); // distance to the trajectory as a vector
    double quot0 = distv.norm() / *(std::min_element(r.begin(), r.end()));
    if (quot0 < quot) {
      quot = quot0;
    }
  }
  return quot;
}

void IntegrateQLabEvents::populateCellsWithPeaks() {
  for (auto &cell_it : m_cellsWithPeaks) {
    OccupiedCell &cell = cell_it.second;
    CellCoords abc(cell.peakQ, m_cellSize);
    for (const int64_t &hash : abc.nearbyCellHashes()) {
      auto cellE_it = m_cellsWithEvents.find(hash);
      if (cellE_it != m_cellsWithEvents.end()) {
        for (const SlimEvent &event : cellE_it->second) {
          SlimEvent neighborEvent = event; // copy
          neighborEvent.second = event.second - cell.peakQ;
          cell.events.emplace_back(neighborEvent);
        }
      }
    }
  }
}

} // namespace MDAlgorithms
} // namespace Mantid
