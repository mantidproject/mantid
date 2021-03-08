// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

#include <memory>

#include <tuple>
#include <unordered_map>
#include <vector>
#include <boost/container_hash/hash.hpp>

namespace Mantid {

namespace Geometry {
class PeakShape;
}
namespace DataObjects {
using DataObjects::EventWorkspace_sptr;
class PeakShapeEllipsoid;
}
namespace MDAlgorithms {
using Mantid::Kernel::V3D;
using Mantid::Geometry::PeakShape_const_sptr;

/// Cubic lattice coordinates in Q-space
struct CellCoords {
  int64_t a;
  int64_t b;
  int64_t c;

  CellCoords(const V3D &q, const double cellSize) :
    a(static_cast<int64_t>(q[0]/cellSize)),
    b(static_cast<int64_t>(q[1]/cellSize)),
    c(static_cast<int64_t>(q[2]/cellSize)) {}

  /// Check if all cell coords are zero
  bool isOrigin() {return !(a || b || c);}

  /// cast coordinates to scalar, to be used as key in an unordered map
  int64_t getHash(){
    return 1000000000000 * a + 100000000 * b + 10000 * c;
  }

  /// Hashes for the 26 first neighbor coordinates plus the coordinates themselves
  std::vector<int64_t> nearbyCellHashes(){
    std::vector<int64_t> neighbors;
    for(int64_t ia = a - 1; ia <= a + 1; ia++)
      for(int64_t ib = b - 1; ib <= b + 1; ib++)
        for(int64_t ic = c - 1; ic <= c + 1; ic++){
          int64_t key = 1000000000000 * ia + 100000000 * ib + 10000 * ic;
          neighbors.emplace_back(key);
        }
    return neighbors;
  }
};

// [(weight, error), Q-vector], trimmed-down info for an event
using SlimEvent = std::pair<std::pair<double, double>, V3D>;
using SlimEvents = std::vector<SlimEvent>;

struct OccupiedCell {
  size_t peakIndex;
  V3D peakQ;  // QLab vector of the peak within this cell
  SlimEvents events; // events potentially closer than m_radius to the peak
};

/**
 @class IntegrateQLabEvents

 This is a low-level class to construct a map with lists of events near
 each peak Q-vector in the lab frame, shifted to be centered at (0,0,0).
 A method is also provided to find the principal axes
 of such a list of events, and to find the net integrated counts,
 using ellipsoids with axis lengths determined from the standard
 deviations in the directions of the principal axes.

 @author Dennis Mikkelson
 @date   2012-12-19
 */

class DLLExport IntegrateQLabEvents {
public:

  /**
   * Store events within a certain radius of the specified peak centers,
   * and sum these events to estimate pixel intensities.
   *
   * @param   peak_q_list  List of Q-vectors for peak centers.
   * @param   radius       The maximum distance from a peak's Q-vector, for
   *                       an event to be stored in the list associated with
   *                       that peak.
   * @param   useOnePercentBackgroundCorrection flag if one percent background
   *                       correction should be used.
   */
  IntegrateQLabEvents(const SlimEvents &peak_q_list, double radius,
                      const bool useOnePercentBackgroundCorrection = true);

  /// Determine if an input Q-vector lies in the cell associated to the origin
  static bool isOrigin(const V3D &q, const double &cellSize);

  /**
   * Add the specified event Q's to lists of events near peaks.  An event is
   * added to at most one list.  First the nearest h,k,l for that event Q vector
   * is calculated.  If a peak with that h,k,l was specified when this object
   * was constructed and if the distance from the specified event Q to that
   * peak is less than the radius that was specified at construction time,
   * then the event Q vector is added to the list of event Q vectors for that
   * peak.
   * NOTE: The Q-vectors passed in to this method will be shifted by the center
   *       Q for it's associated peak, so that the list of Q-vectors for a peak
   *       are centered around 0,0,0 and represent offsets in Q from the peak
   *       center.
   *
   * @param event_qs   List of event Q vectors to add to lists of Q's associated
   *                   with peaks.
   * @param hkl_integ
   */
  void addEvents(SlimEvents const &event_qs);

  /**
   * Integrate the events around the specified peak Q-vector.  The principal
   * axes of the events near this Q-vector and the standard deviations in the
   * directions of these principal axes determine ellipsoidal regions
   * for integrating the peak and estimating the background.  Alternatively,
   * if peak and background radii are specified, then those will be used for
   * half the major axis length of the ellipsoids, and the other axes of the
   * ellipsoids will be set proportionally, based on the standard deviations.
   *
   * @param E1Vec               Vector of values for calculating edge of detectors
   * @param peak_q              The Q-vector for the peak center.
   * @param specify_size        If true the integration will be done using the
   *                            ellipsoids with major axes determined by the
   *                            peak, back_inner and back_outer radii
   *                            parameters.  If false, the integration will be
   *                            done using a peak region with major axis chosen
   *                            so that it covers +- three standard deviations
   *                            of the data in each direction.  In this case,
   *                            the background ellipsoidal shell is chosen to
   *                            have the same VOLUME as the peak ellipsoid, and
   *                            to use the peak ellipsoid for the inner
   *                            radius.
   * @param peak_radius         Size of half the major axis of the ellipsoidal
   *                            peak region.
   * @param back_inner_radius   Size of half the major axis of the INNER
   *                            ellipsoidal boundary of the background region
   * @param back_outer_radius   Size of half the major axis of the OUTER
   *                            ellipsoidal boundary of the background region
   *
   * @param axes_radii          The radii used for integration in the
   *                            directions of the three principal axes.
   * @param inti                Returns the net integrated intensity
   * @param sigi                Returns an estimate of the standard deviation
   *                            of the net integrated intensity
   *
   */
  PeakShape_const_sptr ellipseIntegrateEvents(
      const std::vector<V3D> &E1Vec,
      V3D const &peak_q,
      bool specify_size,
      double peak_radius,
      double back_inner_radius,
      double back_outer_radius,
      std::vector<double> &axes_radii,
      double &inti,
      double &sigi);

  void populateCellsWithPeaks();

private:

  /**
   * Calculate the number of events in an ellipsoid centered at 0,0,0 with
   * the three specified axes and the three specified sizes in the direction
   * of those axes.  NOTE: The three axes must be mutually orthogonal unit
   *                       vectors.
   *
   * @param  events      List of 3D events centered at 0,0,0
   * @param  directions  List of 3 orthonormal directions for the axes of
   *                     the ellipsoid.
   * @param  sizes       List of three values a,b,c giving half the length
   *                     of the three axes of the ellisoid.
   * @return Then number of events that are in or on the specified ellipsoid.
   */
  static std::pair<double, double>
  numInEllipsoid(SlimEvents const &events,
                 std::vector<V3D> const &directions,
                 std::vector<double> const &sizes);

  /**
   * Calculate the number of events in an ellipsoid centered at 0,0,0 with
   * the three specified axes and the three specified sizes in the direction
   * of those axes.  NOTE: The three axes must be mutually orthogonal unit
   *                       vectors.
   *
   * @param  events      List of 3D events centered at 0,0,0
   * @param  directions  List of 3 orthonormal directions for the axes of
   *                     the ellipsoid.
   * @param  sizes       List of three values a,b,c giving half the length
   *                     of the three axes of the ellisoid.
   * @param  sizesIn       List of three values a,b,c giving half the length
   *                     of the three inner axes of the ellisoid.
   * @param  useOnePercentBackgroundCorrection  flag if one percent background
   correction should be used.
   * @return Then number of events that are in or on the specified ellipsoid.
   */
  static std::pair<double, double>
  numInEllipsoidBkg(SlimEvents const &events,
                    std::vector<V3D> const &directions,
                    std::vector<double> const &sizes,
                    std::vector<double> const &sizesIn,
                    const bool useOnePercentBackgroundCorrection);

  /**
   *  Given a list of events, associated with a particular peak
   *  and already SHIFTED to be centered at (0,0,0), calculate the 3x3
   *  covariance matrix for finding the principal axes of that
   *  local event data.  Only events within the specified radius
   *  of (0,0,0) will be used.
   *
   *  The covariance matrix can be easily constructed. X, Y, Z of each peak
   *position are the variables we wish to determine
   *  the covariance. The mean position in each dimension has already been
   *calculated on subtracted, since this corresponds to the centre position of
   *each
   *  peak, which we knew aprori. The expected values of each correlation test X,X
   *X,Y X,Z e.t.c form the elements of this 3 by 3 matrix, but since the
   *  probabilities are equal, we can remove them from the sums of the expected
   *values, and simply divide by the number of events for each matrix element.
   *  Note that the diagonal elements form the variance X,X, Y,Y, Z,Z
   *
   *  @param events    Vector of V3D objects containing the
   *                   Q vectors for a peak, with mean at (0,0,0).
   *  @param matrix    A 3x3 matrix that will be filled out with
   *                   the covariance matrix for the list of
   *                   events.
   *  @param radius    Only events within this radius of the
   *                   peak center (0,0,0) will be used for
   *                   calculating the covariance matrix.
   */
  static void makeCovarianceMatrix(SlimEvents const &events,
      Kernel::DblMatrix &matrix, double radius);

  /**
   *  Calculate the eigen vectors of a 3x3 real symmetric matrix using the GSL.
   *
   *  @param cov_matrix     3x3 real symmetric matrix.
   *  @param eigen_vectors  The eigen vectors for the matrix are returned
   *                        in this list.
   *  @param eigen_values   3 eigenvalues of matrix
   */
  static void getEigenVectors(Kernel::DblMatrix const &cov_matrix,
                              std::vector<V3D> &eigen_vectors,
                              std::vector<double> &eigen_values);

  /**
   * Add an event to the appropriate vector of events for the closest h,k,l,
   * if it is within the required radius of the corresponding peak in the
   * PeakQMap.
   *
   * NOTE: The event passed in may be modified by this method.  In particular,
   * if it corresponds to one of the specified peak_qs, the corresponding peak q
   * will be subtracted from the event and the event will be added to that
   * peak's vector in the event_lists map.
   *
   * @param event_Q      The Q-vector for the event that may be added to the
   *                     event_lists map, if it is close enough to some peak
   * @param hkl_integ
   */
  void addEvent(const SlimEvent event);

  /**
   * Integrate a list of events, centered about (0,0,0) given the principal
   * axes for the events and the standard deviations in the the directions
   * of the principal axes.
   *
   * @param E1Vec             Vector of values for calculating edge of detectors
   * @param peak_q            The Q-vector for the peak center.
   * @param ev_list             List of events centered at (0,0,0) for a
   *                            particular peak.
   * @param directions          The three principal axes of the list of events
   * @param sigmas              The standard deviations of the events in the
   *                            directions of the three principal axes.
   * @param specify_size        If true the integration will be done using the
   *                            ellipsoids with major axes determined by the
   *                            peak, back_inner and back_outer radii
   *                            parameters.  If false, the integration will be
   *                            done using a peak region with major axis chosen
   *                            so that it covers +- three standard deviations
   *                            of the data in each direction.  In this case,
   *                            the background ellipsoidal shell is chosen to
   *                            have the same VOLUME as the peak ellipsoid, and
   *                            to use the peak ellipsoid for the inner
   *                            radius.
   * @param peak_radius         Size of half the major axis of the ellipsoidal
   *                            peak region.
   * @param back_inner_radius   Size of half the major axis of the INNER
   *                            ellipsoidal boundary of the background region
   * @param back_outer_radius   Size of half the major axis of the OUTER
   *                            ellipsoidal boundary of the background region
   *
   * @param axes_radii          The radii used for integration in the
   *                            directions of the three principal axes.
   * @param inti                Returns the net integrated intensity
   * @param sigi                Returns an estimate of the standard deviation
   *                            of the net integrated intensity
   *
   */
  std::shared_ptr<const Mantid::DataObjects::PeakShapeEllipsoid>
  ellipseIntegrateEvents(
      const std::vector<V3D> &E1Vec, V3D const &peak_q,
      SlimEvents const &ev_list,
      std::vector<V3D> const &directions,
      std::vector<double> const &sigmas, bool specify_size, double peak_radius,
      double back_inner_radius, double back_outer_radius,
      std::vector<double> &axes_radii, double &inti, double &sigi);

  /** Calculate if this Q is on a detector
   * The distance from C to OE is given by dv=C-E*(C.scalar_prod(E))
   * If dv.norm<integration_radius, one of the detector trajectories on the edge
   *is too close to the peak
   * This method is applied to all masked pixels. If there are masked pixels
   *trajectories inside an integration volume, the peak must be rejected.
   *
   * @param E1Vec          Vector of values for calculating edge of detectors
   * @param QLabFrame: The Peak center.
   * @param r: Peak radius.
   */
  double detectorQ(const std::vector<V3D> &E1Vec,
                   const V3D QLabFrame,
                   const std::vector<double> &r);

  // Private data members
  double m_radius;            // size of sphere to use for events around a peak
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
