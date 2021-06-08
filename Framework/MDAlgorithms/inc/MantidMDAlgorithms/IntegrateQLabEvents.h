// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace Mantid {

namespace Geometry {
class PeakShape;
}

namespace MDAlgorithms {
using Mantid::DataObjects::PeakShapeEllipsoid_const_sptr;
using Mantid::Geometry::PeakShape_const_sptr;
using Mantid::Kernel::V3D;

/// Partition QLab space into a cubic lattice
struct CellCoords {
  int64_t a;
  int64_t b;
  int64_t c;

  CellCoords(const V3D &q, const double cellSize)
      : a(static_cast<int64_t>(q[0] / cellSize)), b(static_cast<int64_t>(q[1] / cellSize)),
        c(static_cast<int64_t>(q[2] / cellSize)) {}

  /// Check if all cell coords are zero
  bool isOrigin() { return !(a || b || c); }

  /// cast coordinates to scalar, to be used as key in an unordered map
  int64_t getHash() { return 1000000000000 * a + 100000000 * b + 10000 * c; }

  /// Hashes for the 26 first neighbor coordinates plus the coordinates
  /// themselves
  std::vector<int64_t> nearbyCellHashes() {
    std::vector<int64_t> neighbors;
    for (int64_t ia = a - 1; ia <= a + 1; ia++)
      for (int64_t ib = b - 1; ib <= b + 1; ib++)
        for (int64_t ic = c - 1; ic <= c + 1; ic++) {
          int64_t key = 1000000000000 * ia + 100000000 * ib + 10000 * ic;
          neighbors.emplace_back(key);
        }
    return neighbors;
  }
};

// [(weight, error), Q-vector], trimmed-down info for an event
using SlimEvent = std::pair<std::pair<double, double>, V3D>;
using SlimEvents = std::vector<SlimEvent>;

// A cell in partitioned QLab space containing one peak
struct OccupiedCell {
  size_t peakIndex;  // index of the peak within this cell
  V3D peakQ;         // QLab vector of the peak within this cell
  SlimEvents events; // events potentially closer than m_radius to the peak
};

/**
 @class IntegrateQLabEvents

 This is a low-level class to construct a map with lists of events near
 each peak Q-vector in the lab frame. The Q-vector of each event is shifted
 by the Q-vector of the associated peak.
 A method is also provided to find the principal axes of such a list
 of events and to find the net integrated counts using ellipsoids
 with axis lengths determined from the standard deviations in the
 directions of the principal axes.

 @author Dennis Mikkelson
 @date   2012-12-19
 */

class DLLExport IntegrateQLabEvents {
public:
  /**
   * @brief Store events within a certain radius of the specified peak centers,
   * and sum these events to estimate pixel intensities.
   * @param peak_q_list : List of Q-vectors for peak centers.
   * @param radius : The maximum distance from a peak's Q-vector, for an
   * event to be stored in the list associated with that peak.
   * @param useOnePercentBackgroundCorrection : flag if one percent background
   * correction should be used. */
  IntegrateQLabEvents(const SlimEvents &peak_q_list, double radius,
                      const bool useOnePercentBackgroundCorrection = true);

  /// Determine if an input Q-vector lies in the cell associated to the origin
  static bool isOrigin(const V3D &q, const double &cellSize);

  /**
   * @brief distribute the events among the cells of the partitioned QLab
   * space.
   * @details Given QLab partitioned into a cubic lattice with unit cell of
   * certain size, assign each event to one particular cell depending on its
   * QLab vector.
   * @param event_qs : List of SlimEvent objects to be distributed */
  void addEvents(SlimEvents const &event_qs);

  /**
   * @brief Integrate the events around the specified peak QLab vector.
   * @details The principal axes of the events near this Q-vector
   * and the standard deviations in the directions of these principal
   * axes determine ellipsoidal regions for integrating the peak and
   * estimating the background.  Alternatively, if peak and background
   * radii are specified, then those will be used for half the major
   * axis length of the ellipsoids, and the other axes of the ellipsoids
   * will be set proportionally, based on the standard deviations.
   * @param E1Vec : Vector of values for calculating edge of detectors
   * @param peak_q : The QLab-vector for the peak center.
   * @param specify_size : If true the integration will be done using the
   * ellipsoids with major axes determined by the peak, back_inner
   * and back_outer radii parameters. If false, the integration will be
   * done using a peak region with major axis chosen so that it
   * covers +- three standard deviations of the data in each direction. In
   * this case, the background ellipsoidal shell is chosen to have the
   * same VOLUME as the peak ellipsoid, and to use the peak ellipsoid
   * for the inner radius.
   * @param peak_radius : Size of half the major axis of the ellipsoidal
   * peak region.
   * @param back_inner_radius : Size of half the major axis of the INNER
   * ellipsoidal boundary of the background region
   * @param back_outer_radius : Size of half the major axis of the OUTER
   * ellipsoidal boundary of the background region
   * @param axes_radii : The radii used for integration in the directions
   * of the three principal axes.
   * @param inti : (output) collects the net integrated intensity
   * @param sigi : (output) collects an estimate of the standard deviation
   * of the net integrated intensity */
  PeakShape_const_sptr ellipseIntegrateEvents(const std::vector<V3D> &E1Vec, V3D const &peak_q, bool specify_size,
                                              double peak_radius, double back_inner_radius, double back_outer_radius,
                                              std::vector<double> &axes_radii, double &inti, double &sigi);

  /**
   * @brief Assign events to each of the cells occupied by events.
   * @details Iterate over each QLab cell containing a peak and accumulate the
   * list of events for the cell and for the first-neighbor cells into a
   * single list of events. The QLab vectors for this events are shifted
   * by the QLab vector of the peak. */
  void populateCellsWithPeaks();

private:
  /**
   * @brief Number of events in an ellipsoid.
   * @details The ellipsoid is centered at 0,0,0 with the three specified
   * axes and the three specified sizes in the direction of those axes.
   * NOTE: The three axes must be mutually orthogonal unit vectors.
   * @param events : List of SlimEvents centered at 0,0,0
   * @param directions : List of 3 orthonormal directions for the axes of
   * the ellipsoid.
   * @param sizes : List of three values a,b,c giving half the length
   * of the three axes of the ellisoid.
   * @return number of events and estimated error */
  static std::pair<double, double> numInEllipsoid(SlimEvents const &events, std::vector<V3D> const &directions,
                                                  std::vector<double> const &sizes);

  /**
   * @brief Number of events in an ellipsoid with background correction.
   * @details The ellipsoid is centered at 0,0,0 with the three specified
   * axes and the three specified sizes in the direction of those axes.
   * NOTE: The three axes must be mutually orthogonal unit vectors.
   * @param events : List of 3D events centered at 0,0,0
   * @param directions : List of 3 orthonormal directions for the axes of
   * the ellipsoid.
   * @param sizes : List of three values a,b,c giving half the length
   * of the three axes of the ellisoid.
   * @param sizesIn : List of three values a,b,c giving half the length
   * of the three inner axes of the ellisoid.
   * @param useOnePercentBackgroundCorrection : flag if one percent background
   * correction should be used.
   * @return number of events and estimated error */
  static std::pair<double, double> numInEllipsoidBkg(SlimEvents const &events, std::vector<V3D> const &directions,
                                                     std::vector<double> const &sizes,
                                                     std::vector<double> const &sizesIn,
                                                     const bool useOnePercentBackgroundCorrection);

  /**
   * @brief 3x3 covariance matrix of a list of SlimEvent objects
   * @details the purpose of the covariance matrix is to find the principal axes
   * of the SlimeEvents, associated with a particular peak. Their QLab vectors
   * are already shifted by the QLab vector of the peak. Only events within
   * the specified distance from the peak (here at Q=[0,0,0]) will be used.
   * The covariance matrix can be easily constructed. X, Y, Z of each peak
   * position are the variables we wish to determine the covariance. The mean
   * position in each dimension has already been calculated on subtracted,
   * since this corresponds to the QLab vector peak. The expected values
   * of each correlation test X,X X,Y X,Z e.t.c form the elements of this
   * 3x3 matrix, but since the  probabilities are equal, we can remove them
   * from the sums of the expected values, and simply divide by the number
   * of events for each matrix element. Note that the diagonal elements
   * form the variance X,X, Y,Y, Z,Z
   * @param events : SlimEvents associated to one peak
   * @param matrix : (output) 3x3 covariance matrix
   * @param radius : Only events within this distance radius of the
   * peak (here at Q=[0,0,0]) are used for calculating the covariance matrix.*/
  static void makeCovarianceMatrix(SlimEvents const &events, Kernel::DblMatrix &matrix, double radius);

  /**
   * @brief Eigen vectors of a 3x3 real symmetric matrix using the GSL.
   * @param cov_matrix : 3x3 real symmetric matrix.
   * @param eigen_vectors : (output) returned eigen vectors
   * @param eigen_values : (output) three eigenvalues
   */
  static void getEigenVectors(Kernel::DblMatrix const &cov_matrix, std::vector<V3D> &eigen_vectors,
                              std::vector<double> &eigen_values);

  /**
   * @brief assign an event to one cell of the partitioned QLab space.
   * @param event : SlimEvent to be assigned */
  void addEvent(const SlimEvent event);

  /**
   * @brief Integrate a list of events associated to one peak.
   * @details The QLab vector of the events are shifted by the QLab vector
   * of the peak. Spatial distribution of the events in QLab space is
   * described with principal axes of the ellipsoid, as well as the
   * standard deviations in the the directions of the principal axes.
   * @param E1Vec : Vector of values for calculating edge of detectors
   * @param peak_q : The Q-vector for the peak center.
   * @param ev_list : List of events centered around the peak (here with
   * Q=[0,0,0]).
   * @param directions : The three principal axes of the list of events
   * @param sigmas : The standard deviations of the events in the
   * directions of the three principal axes.
   * @param specify_size : If true the integration will be done using the
   * ellipsoids with major axes determined by the peak, back_inner and
   * back_outer radii parameters. If false, the integration will be done
   * using a peak region with major axis chosen so that it covers +- three
   * standard deviations of the data in each direction. In this case, the
   * background ellipsoidal shell is chosen to have the same VOLUME as the
   * peak ellipsoid, and to use the peak ellipsoid for the inner radius.
   * @param peak_radius : Size of half the major axis of the ellipsoid
   * @param back_inner_radius : Size of half the major axis of the INNER
   * ellipsoidal boundary of the background region
   * @param back_outer_radius : Size of half the major axis of the OUTER
   * ellipsoidal boundary of the background region
   * @param axes_radii : The radii used for integration in the directions
   * of the three principal axes.
   * @param inti : (output) net integrated intensity
   * @param sigi : (output) estimate of the standard deviation the intensity */
  PeakShapeEllipsoid_const_sptr ellipseIntegrateEvents(const std::vector<V3D> &E1Vec, V3D const &peak_q,
                                                       SlimEvents const &ev_list, std::vector<V3D> const &directions,
                                                       std::vector<double> const &sigmas, bool specify_size,
                                                       double peak_radius, double back_inner_radius,
                                                       double back_outer_radius, std::vector<double> &axes_radii,
                                                       double &inti, double &sigi);

  /**
   * @brief Calculate if this Q is on a detector
   * @details The distance from C to OE is given by dv=C-E*(C.scalar_prod(E))
   * If dv.norm<integration_radius, one of the detector trajectories on the
   * edge is too close to the peak. This method is applied to all masked
   * pixels. If there are masked pixels trajectories inside an integration
   * volume, the peak must be rejected.
   * @param E1Vec : Vector of values for calculating edge of detectors
   * @param QLabFrame: The Peak center.
   * @param r: Peak radius.
   */
  double detectorQ(const std::vector<V3D> &E1Vec, const V3D QLabFrame, const std::vector<double> &r);

  // Private data members
  double m_radius; // size of sphere to use for events around a peak
  /// if one percent culling of the background should be performed.
  const bool m_useOnePercentBackgroundCorrection;
  /// size of the square cell unit, holding at most one single peak
  double m_cellSize;
  /// list the occupied cells in an unordered map for fast searching
  std::unordered_map<size_t, OccupiedCell> m_cellsWithPeaks;
  /// list of cells occupied with events
  std::unordered_map<size_t, SlimEvents> m_cellsWithEvents;
};

} // namespace MDAlgorithms

} // namespace Mantid
