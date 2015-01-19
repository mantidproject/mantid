#include <math.h>
#include <iostream>
#include <fstream>
#include <boost/math/special_functions/round.hpp>

#include "MantidMDEvents/Integrate3DEvents.h"

extern "C" {
#include <stdio.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_eigen.h>
}

namespace Mantid {
namespace MDEvents {

using namespace std;

/**
 * Construct an object to store events that correspond to a peak an are
 * within the specified radius of the specified peak centers, and to
 * integrate the peaks.
 *
 * @param   peak_q_list  List of Q-vectors for peak centers.
 * @param   UBinv        The matrix that maps Q-vectors to h,k,l
 * @param   radius       The maximum distance from a peak's Q-vector, for
 *                       an event to be stored in the list associated with
 *                       that peak.
 */
Integrate3DEvents::Integrate3DEvents(std::vector<V3D> const &peak_q_list,
                                     DblMatrix const &UBinv, double radius) {
  this->UBinv = UBinv;
  this->radius = radius;

  int64_t hkl_key;
  for (size_t i = 0; i < peak_q_list.size(); i++) {
    hkl_key = getHklKey(peak_q_list[i]);

    if (hkl_key != 0) // only save if hkl != (0,0,0)
      peak_qs[hkl_key] = peak_q_list[i];
  }
}

/**
 *  Destructor.
 */
Integrate3DEvents::~Integrate3DEvents() {}

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
 */
void Integrate3DEvents::addEvents(std::vector<V3D> const &event_qs) {
  for (size_t i = 0; i < event_qs.size(); i++) {
    addEvent(event_qs[i]);
  }
}

/**
 * Integrate the events around the specified peak Q-vector.  The principal
 * axes of the events near this Q-vector and the standard deviations in the
 * directions of these principal axes determine ellipsoidal regions
 * for integrating the peak and estimating the background.  Alternatively,
 * if peak and background radii are specified, then those will be used for
 * half the major axis length of the ellipsoids, and the other axes of the
 * ellipsoids will be set proportionally, based on the standard deviations.
 *
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
void Integrate3DEvents::ellipseIntegrateEvents(
    V3D const &peak_q, bool specify_size, double peak_radius,
    double back_inner_radius, double back_outer_radius,
    std::vector<double> &axes_radii, double &inti, double &sigi) {
  inti = 0.0; // default values, in case something
  sigi = 0.0; // is wrong with the peak.

  int64_t hkl_key = getHklKey(peak_q);
  if (hkl_key == 0) {
    return;
  }

  std::vector<V3D> &some_events = event_lists[hkl_key];

  if (some_events.size() < 3) // if there are not enough events to
  {                           // find covariance matrix, return
    return;
  }

  DblMatrix cov_matrix(3, 3);
  makeCovarianceMatrix(some_events, cov_matrix, radius);

  std::vector<V3D> eigen_vectors;
  getEigenVectors(cov_matrix, eigen_vectors);

  std::vector<double> sigmas;
  for (int i = 0; i < 3; i++) {
    sigmas.push_back(stdDev(some_events, eigen_vectors[i], radius));
  }

  bool invalid_peak = false;
  for (int i = 0; i < 3; i++) {
    if ((boost::math::isnan)(sigmas[i])) {
      invalid_peak = true;
    } else if (sigmas[i] <= 0) {
      invalid_peak = true;
    }
  }

  if (invalid_peak) // if data collapses to a line or
  {                 // to a plane, the volume of the
    return;         // ellipsoids will be zero.
  }

  ellipseIntegrateEvents(some_events, eigen_vectors, sigmas, specify_size,
                         peak_radius, back_inner_radius, back_outer_radius,
                         axes_radii, inti, sigi);
}

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
int Integrate3DEvents::numInEllipsoid(std::vector<V3D> const &events,
                                      std::vector<V3D> const &directions,
                                      std::vector<double> const &sizes) {
  int count = 0;
  for (size_t i = 0; i < events.size(); i++) {
    double sum = 0;
    for (size_t k = 0; k < 3; k++) {
      double comp = events[i].scalar_prod(directions[k]) / sizes[k];
      sum += comp * comp;
    }
    if (sum <= 1)
      count++;
  }

  return count;
}

/**
 *  Given a list of events, associated with a particular peak
 *  and SHIFTED to be centered at (0,0,0), calculate the 3x3
 *  covariance matrix for finding the principal axes of that
 *  local event data.  Only events within the specified radius
 *  of (0,0,0) will be used.
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
void Integrate3DEvents::makeCovarianceMatrix(std::vector<V3D> const &events,
                                             DblMatrix &matrix, double radius) {
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      double sum = 0;
      for (size_t i = 0; i < events.size(); i++) {
        if (events[i].norm() <= radius) {
          sum += events[i][row] * events[i][col];
        }
      }
      if (events.size() > 1)
        matrix[row][col] = sum / (double)(events.size() - 1);
      else
        matrix[row][col] = sum;
    }
  }
}

/**
 *  Calculate the eigen vectors of a 3x3 real symmetric matrix using the GSL.
 *
 *  @param cov_matrix     3x3 real symmetric matrix.
 *  @param eigen_vectors  The eigen vectors for the matrix are returned
 *                        in this list.
 */
void Integrate3DEvents::getEigenVectors(DblMatrix const &cov_matrix,
                                        std::vector<V3D> &eigen_vectors) {
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
    eigen_vectors.push_back(V3D(gsl_matrix_get(eigen_vec, 0, col),
                                gsl_matrix_get(eigen_vec, 1, col),
                                gsl_matrix_get(eigen_vec, 2, col)));
  }

  gsl_matrix_free(matrix);
  gsl_vector_free(eigen_val);
  gsl_matrix_free(eigen_vec);
  gsl_eigen_symmv_free(wkspace);
}

/**
 *  Calculate the standard deviation of the given list of 3D events in
 *  the direction of the specified vector.  Only events that are within
 *  the specified radius of 0,0,0 will be considered.
 *
 *  @param  events      List of 3D events centered at 0,0,0
 *  @param  direction   Unit vector giving the direction vector on which
 *                      the 3D events will be projected.
 *  @param  radius      Maximun size of event vectors that will be used
 *                      in calculating the standard deviation.
 */
double Integrate3DEvents::stdDev(std::vector<V3D> const &events,
                                 V3D const &direction, double radius) {
  double sum = 0;
  double sum_sq = 0;
  double stdev = 0;
  int count = 0;

  for (size_t i = 0; i < events.size(); i++) {
    if (events[i].norm() <= radius) {
      double dot_prod = events[i].scalar_prod(direction);
      sum += dot_prod;
      sum_sq += dot_prod * dot_prod;
      count++;
    }
  }

  if (count > 1) {
    double ave = sum / count;
    stdev = sqrt((sum_sq / count - ave * ave) * (double)count / (count - 1.0));
  }

  return stdev;
}

/**
 *  Form a map key as 10^12*h + 10^6*k + l from the integers h,k,l.
 *
 *  @param  h        The first Miller index
 *  @param  k        The second Miller index
 *  @param  l        The third  Miller index
 */
int64_t Integrate3DEvents::getHklKey(int h, int k, int l) {
  int64_t key(0);

  if (h != 0 || k != 0 || l != 0)
    key = 1000000000000 * h + 1000000 * k + l;

  return key;
}

/**
 *  Form a map key for the specified q_vector.  The q_vector is mapped to
 *  h,k,l by UBinv and the map key is then formed from those rounded h,k,l
 *  values.
 *
 *  @param q_vector  The q_vector to be mapped to h,k,l
 */
int64_t Integrate3DEvents::getHklKey(V3D const &q_vector) {
  V3D hkl = UBinv * q_vector;
  int h = boost::math::iround<double>(hkl[0]);
  int k = boost::math::iround<double>(hkl[1]);
  int l = boost::math::iround<double>(hkl[2]);
  return getHklKey(h, k, l);
}

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
 */
void Integrate3DEvents::addEvent(V3D event_Q) {
  int64_t hkl_key = getHklKey(event_Q);

  if (hkl_key == 0) // don't keep events associated with 0,0,0
    return;

  V3D peak_q = peak_qs[hkl_key];
  if (!peak_q.nullVector()) {
    event_Q = event_Q - peak_q;
    if (event_Q.norm() < radius) {
      event_lists[hkl_key].push_back(event_Q);
    }
  }
}

/**
 * Integrate a list of events, centered about (0,0,0) given the principal
 * axes for the events and the standard deviations in the the directions
 * of the principal axes.
 *
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
void Integrate3DEvents::ellipseIntegrateEvents(
    std::vector<V3D> const &ev_list, std::vector<V3D> const &directions,
    std::vector<double> const &sigmas, bool specify_size, double peak_radius,
    double back_inner_radius, double back_outer_radius,
    std::vector<double> &axes_radii, double &inti, double &sigi) {
  // r1, r2 and r3 will give the sizes of the major axis of
  // the peak ellipsoid, and of the inner and outer surface
  // of the background ellipsoidal shell, respectively.
  // They will specify the size as the number of standard
  // deviations in the direction of each of the pricipal
  // axes that the ellipsoid will extend from the center.
  double r1, r2, r3;

  double max_sigma = sigmas[0];
  for (int i = 1; i < 3; i++) {
    if (sigmas[i] > max_sigma) {
      max_sigma = sigmas[i];
    }
  }

  if (specify_size) {
    r1 = peak_radius / max_sigma;       // scale specified sizes by 1/max_sigma
    r2 = back_inner_radius / max_sigma; // so when multiplied by the individual
    r3 = back_outer_radius / max_sigma; // sigmas in different directions, the
  }                                     // major axis has the specified size
  else {
    r1 = 3;
    r2 = 3;
    r3 = r2 * 1.25992105; // A factor of 2 ^ (1/3) will make the background
                          // shell volume equal to the peak region volume.

    // if necessary restrict the background ellipsoid
    // to lie within the specified sphere, and adjust
    // the other sizes, proportionally
    if (r3 * max_sigma > radius) {
      r3 = radius / max_sigma;
      r1 = r3 * 0.79370053f; // This value for r1 and r2 makes the background
      r2 = r1;               // shell volume equal to the peak region volume.
    }
  }

  axes_radii.clear();
  for (int i = 0; i < 3; i++) {
    axes_radii.push_back(r3 * sigmas[i]);
  }
  double back2 = numInEllipsoid(ev_list, directions, axes_radii);

  for (int i = 0; i < 3; i++) {
    axes_radii[i] = r2 * sigmas[i];
  }
  double back1 = numInEllipsoid(ev_list, directions, axes_radii);

  for (int i = 0; i < 3; i++) {
    axes_radii[i] = r1 * sigmas[i];
  }
  double peak_w_back = numInEllipsoid(ev_list, directions, axes_radii);

  double backgrd = back2 - back1;

  double ratio = pow(r1, 3) / (pow(r3, 3) - pow(r2, 3));

  inti = peak_w_back - ratio * backgrd;
  sigi = sqrt(peak_w_back + ratio * ratio * backgrd);
}

} // namespace MDEvents

} // namespace Mantid
