// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/Integrate3DEvents.h"
#include "MantidDataObjects/NoShape.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"

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
namespace Mantid::MDAlgorithms {

using namespace std;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::V3D;

/**
 * Construct an object to store events that correspond to a peak and are
 * within the specified radius of the specified peak centers, and to
 * integrate the peaks.
 *
 * @param   peak_q_list  List of Q-vectors for peak centers.
 * @param   UBinv        The matrix that maps Q-vectors to h,k,l
 * @param   radius       The maximum distance from a peak's Q-vector, for
 *                       an event to be stored in the list associated with
 *                       that peak.
 * @param   useOnePercentBackgroundCorrection flag if one percent background
 *                       correction should be used.
 */
Integrate3DEvents::Integrate3DEvents(
    const std::vector<std::pair<std::pair<double, double>, Mantid::Kernel::V3D>> &peak_q_list, Kernel::DblMatrix UBinv,
    double radius, const bool useOnePercentBackgroundCorrection)
    : m_UBinv(std::move(UBinv)), m_radius(radius), maxOrder(0), crossterm(false),
      m_useOnePercentBackgroundCorrection(useOnePercentBackgroundCorrection) {
  for (size_t it = 0; it != peak_q_list.size(); ++it) {
    int64_t hkl_key = getHklKey(peak_q_list[it].second);
    if (hkl_key != 0) // only save if hkl != (0,0,0)
      m_peak_qs[hkl_key] = peak_q_list[it].second;
  }
}

/**
 * With modulation vectors, construct an object to store events that correspond
 * to a peak and are within the specified radius of the specified peak centers,
 * and to integrate the peaks.
 *
 * @overload
 * @param   peak_q_list  List of Q-vectors for peak centers.
 * @param   hkl_list     The list of h,k,l
 * @param   mnp_list     The list of satellite m,n,p
 * @param   UBinv        The matrix that maps Q-vectors to h,k,l
 * @param   ModHKL       The Modulation vectors
 * @param   radius_m     The maximum distance from a peak's Q-vector, for
 *                       an event to be stored in the list associated with
 *                       that peak.
 * @param   radius_s     The maximum distance from a peak's Q-vector, for
 *                       an event to be stored in the list associated with
 *                       that satellite peak.
 * @param   MaxO         The maximum order of satellite peaks.
 * @param   CrossT       Switch for cross terms of satellites.
 * @param   useOnePercentBackgroundCorrection flag if one percent background
 *                       correction should be used.
 */
Integrate3DEvents::Integrate3DEvents(
    const std::vector<std::pair<std::pair<double, double>, Mantid::Kernel::V3D>> &peak_q_list,
    std::vector<V3D> const &hkl_list, std::vector<V3D> const &mnp_list, Kernel::DblMatrix UBinv,
    Kernel::DblMatrix ModHKL, double radius_m, double radius_s, int MaxO, const bool CrossT,
    const bool useOnePercentBackgroundCorrection)
    : m_UBinv(std::move(UBinv)), m_ModHKL(std::move(ModHKL)), m_radius(radius_m), s_radius(radius_s), maxOrder(MaxO),
      crossterm(CrossT), m_useOnePercentBackgroundCorrection(useOnePercentBackgroundCorrection) {
  for (size_t it = 0; it != peak_q_list.size(); ++it) {
    int64_t hklmnp_key =
        getHklMnpKey(boost::math::iround<double>(hkl_list[it][0]), boost::math::iround<double>(hkl_list[it][1]),
                     boost::math::iround<double>(hkl_list[it][2]), boost::math::iround<double>(mnp_list[it][0]),
                     boost::math::iround<double>(mnp_list[it][1]), boost::math::iround<double>(mnp_list[it][2]));
    if (hklmnp_key != 0) // only save if hkl != (0,0,0)
      m_peak_qs[hklmnp_key] = peak_q_list[it].second;
  }
}

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
void Integrate3DEvents::addEvents(std::vector<std::pair<std::pair<double, double>, V3D>> const &event_qs,
                                  bool hkl_integ) {
  if (!maxOrder)
    for (const auto &event_q : event_qs)
      addEvent(event_q, hkl_integ);
  else
    for (const auto &event_q : event_qs)
      addModEvent(event_q, hkl_integ);
}

std::pair<std::shared_ptr<const Geometry::PeakShape>, std::tuple<double, double, double>>
Integrate3DEvents::integrateStrongPeak(const IntegrationParameters &params, const V3D &peak_q, double &inti,
                                       double &sigi) {

  inti = 0.0; // default values, in case something
  sigi = 0.0; // is wrong with the peak.
  auto result = getEvents(peak_q);
  if (!result)
    return std::make_pair(std::make_shared<NoShape>(), make_tuple(0., 0., 0.));

  const auto &events = *result;
  if (events.empty())
    return std::make_pair(std::make_shared<NoShape>(), make_tuple(0., 0., 0.));

  DblMatrix cov_matrix(3, 3);
  makeCovarianceMatrix(events, cov_matrix, params.regionRadius);

  std::vector<V3D> eigen_vectors;
  std::vector<double> eigen_values;
  getEigenVectors(cov_matrix, eigen_vectors, eigen_values);

  std::vector<double> sigmas(3);
  for (int i = 0; i < 3; i++) {
    sigmas[i] = sqrt(eigen_values[i]);
  }

  bool invalid_peak =
      std::any_of(sigmas.cbegin(), sigmas.cend(), [](const double sigma) { return std::isnan(sigma) || sigma <= 0; });

  if (invalid_peak)
    return std::make_pair(std::make_shared<NoShape>(), make_tuple(0., 0., 0.));

  const auto max_sigma = *std::max_element(sigmas.begin(), sigmas.end());
  if (max_sigma == 0)
    return std::make_pair(std::make_shared<NoShape>(), make_tuple(0., 0., 0.));

  auto rValues = calculateRadiusFactors(params, max_sigma);
  auto &r1 = std::get<0>(rValues), r2 = std::get<1>(rValues), r3 = std::get<2>(rValues);

  std::vector<double> abcBackgroundOuterRadii, abcBackgroundInnerRadii;
  std::vector<double> peakRadii;
  for (int i = 0; i < 3; i++) {
    abcBackgroundOuterRadii.emplace_back(r3 * sigmas[i]);
    abcBackgroundInnerRadii.emplace_back(r2 * sigmas[i]);
    peakRadii.emplace_back(r1 * sigmas[i]);
  }

  const auto isPeakOnDetector = correctForDetectorEdges(rValues, params.E1Vectors, peak_q, peakRadii,
                                                        abcBackgroundInnerRadii, abcBackgroundOuterRadii);

  if (!isPeakOnDetector)
    return std::make_pair(std::make_shared<NoShape>(), make_tuple(0.0, 0.0, 0.));

  const auto backgrd = numInEllipsoidBkg(events, eigen_vectors, abcBackgroundOuterRadii, abcBackgroundInnerRadii,
                                         m_useOnePercentBackgroundCorrection);
  const auto core = numInEllipsoid(events, eigen_vectors, sigmas);
  const auto peak = numInEllipsoid(events, eigen_vectors, peakRadii);
  const auto ratio = pow(r1, 3) / (pow(r3, 3) - pow(r2, 3));

  inti = peak.first - ratio * backgrd.first;
  sigi = sqrt(peak.second + ratio * ratio * backgrd.second);

  // compute the fraction of peak within the standard core
  const auto total = (core.first + peak.first) - ratio * backgrd.first;
  const auto frac = std::min(1.0, std::abs(inti / total));
  // compute the uncertainty in the fraction
  const auto df_ds_core = (1 - frac) / peak.first;
  const auto df_ds_peak = frac / peak.first;
  const auto fracError = sqrt(peak.first * pow(df_ds_core, 2) + core.first * pow(df_ds_peak, 2));

  // create the peaks shape for the strong peak
  const auto shape = std::make_shared<const PeakShapeEllipsoid>(eigen_vectors, peakRadii, abcBackgroundInnerRadii,
                                                                abcBackgroundOuterRadii, Mantid::Kernel::QLab,
                                                                "IntegrateEllipsoidsTwoStep");

  return std::make_pair(shape, std::make_tuple(frac, fracError, max_sigma));
}

std::shared_ptr<const Geometry::PeakShape>
Integrate3DEvents::integrateWeakPeak(const IntegrationParameters &params, PeakShapeEllipsoid_const_sptr shape,
                                     const std::tuple<double, double, double> &libPeak, const V3D &center, double &inti,
                                     double &sigi) {

  inti = 0.0; // default values, in case something
  sigi = 0.0; // is wrong with the peak.

  auto result = getEvents(center);
  if (!result)
    return std::make_shared<NoShape>();

  const auto &events = *result;

  const auto &directions = shape->directions();
  auto abcBackgroundInnerRadii = shape->abcRadiiBackgroundInner();
  auto abcBackgroundOuterRadii = shape->abcRadiiBackgroundOuter();
  auto abcRadii = shape->abcRadii();

  const auto max_sigma = std::get<2>(libPeak);
  auto rValues = calculateRadiusFactors(params, max_sigma);

  const auto isPeakOnDetector = correctForDetectorEdges(rValues, params.E1Vectors, center, abcRadii,
                                                        abcBackgroundInnerRadii, abcBackgroundOuterRadii);

  if (!isPeakOnDetector)
    return shape;

  const double r1 = std::get<0>(rValues), r2 = std::get<1>(rValues), r3 = std::get<2>(rValues);

  // integrate
  std::pair<double, double> backgrd = numInEllipsoidBkg(events, directions, abcBackgroundOuterRadii,
                                                        abcBackgroundInnerRadii, m_useOnePercentBackgroundCorrection);
  std::pair<double, double> peak_w_back = numInEllipsoid(events, directions, abcRadii);
  double ratio = pow(r1, 3) / (pow(r3, 3) - pow(r2, 3));

  const auto frac = std::get<0>(libPeak);
  const auto fracError = std::get<1>(libPeak);

  inti = peak_w_back.first - ratio * backgrd.first;

  // correct for fractional intensity
  sigi = sigi / pow(inti, 2);
  sigi += pow((fracError / frac), 2);

  inti = inti * frac;
  sigi = sqrt(sigi) * inti;

  // scale integration shape by fractional amount
  for (size_t i = 0; i < abcRadii.size(); ++i) {
    abcRadii[i] *= frac;
    abcBackgroundInnerRadii[i] *= frac;
    abcBackgroundOuterRadii[i] *= frac;
  }

  return std::make_shared<const PeakShapeEllipsoid>(shape->directions(), abcRadii, abcBackgroundInnerRadii,
                                                    abcBackgroundOuterRadii, Mantid::Kernel::QLab,
                                                    "IntegrateEllipsoidsTwoStep");
}

double Integrate3DEvents::estimateSignalToNoiseRatio(const IntegrationParameters &params, const V3D &center,
                                                     bool forceSpherical, double sphericityTol) {

  auto result = getEvents(center);
  if (!result)
    return .0;

  const auto &events = *result;
  if (events.empty())
    return .0;

  DblMatrix cov_matrix(3, 3);
  makeCovarianceMatrix(events, cov_matrix, params.regionRadius);

  std::vector<V3D> eigen_vectors;
  std::vector<double> eigen_values;
  getEigenVectors(cov_matrix, eigen_vectors, eigen_values);

  std::vector<double> sigmas(3);
  for (int i = 0; i < 3; i++) {
    sigmas[i] = sqrt(eigen_values[i]);
  }

  const auto max_sigma = *std::max_element(sigmas.begin(), sigmas.end());
  const auto min_sigma = *std::min_element(sigmas.begin(), sigmas.end());
  if (max_sigma == 0)
    return .0;

  auto rValues = calculateRadiusFactors(params, max_sigma);
  auto &r1 = std::get<0>(rValues), r2 = std::get<1>(rValues), r3 = std::get<2>(rValues);
  std::vector<double> abcBackgroundOuterRadii, abcBackgroundInnerRadii;
  std::vector<double> peakRadii;
  if (forceSpherical) {
    // test for spherically symmeteric peak (within tolerance)
    if ((max_sigma - min_sigma) / max_sigma > sphericityTol)
      return .0;
    for (int i = 0; i < 3; i++) {
      abcBackgroundOuterRadii.emplace_back(r3 * max_sigma);
      abcBackgroundInnerRadii.emplace_back(r2 * max_sigma);
      peakRadii.emplace_back(r1 * max_sigma);
    }
  } else {
    for (int i = 0; i < 3; i++) {
      abcBackgroundOuterRadii.emplace_back(r3 * sigmas[i]);
      abcBackgroundInnerRadii.emplace_back(r2 * sigmas[i]);
      peakRadii.emplace_back(r1 * sigmas[i]);
    }
  }

  // Background / Peak / Background
  std::pair<double, double> backgrd = numInEllipsoidBkg(events, eigen_vectors, abcBackgroundOuterRadii,
                                                        abcBackgroundInnerRadii, m_useOnePercentBackgroundCorrection);

  std::pair<double, double> peak_w_back = numInEllipsoid(events, eigen_vectors, peakRadii);

  double ratio = pow(r1, 3) / (pow(r3, 3) - pow(r2, 3));
  auto inti = peak_w_back.first - ratio * backgrd.first;
  auto sigi = sqrt(peak_w_back.second + ratio * ratio * backgrd.second);

  return inti / sigi;
}

const std::vector<std::pair<std::pair<double, double>, V3D>> *Integrate3DEvents::getEvents(const V3D &peak_q) {
  auto hkl_key = getHklKey(peak_q);
  if (maxOrder)
    hkl_key = getHklMnpKey(peak_q);

  if (hkl_key == 0)
    return nullptr;

  const auto pos = m_event_lists.find(hkl_key);

  if (m_event_lists.end() == pos)
    return nullptr;

  if (pos->second.size() < 3) // if there are not enough events
    return nullptr;

  return &(pos->second);
}

bool Integrate3DEvents::correctForDetectorEdges(std::tuple<double, double, double> &radii,
                                                const std::vector<V3D> &E1Vecs, const V3D &peak_q,
                                                const std::vector<double> &axesRadii,
                                                const std::vector<double> &bkgInnerRadii,
                                                const std::vector<double> &bkgOuterRadii) {

  if (E1Vecs.empty())
    return true;

  const auto &r1 = std::get<0>(radii);
  auto &r2 = std::get<1>(radii);
  auto &r3 = std::get<2>(radii);
  auto h3 = 1.0 - detectorQ(E1Vecs, peak_q, bkgOuterRadii);
  // scaled from area of circle minus segment when r normalized to 1
  auto m3 = std::sqrt(1.0 - (std::acos(1.0 - h3) - (1.0 - h3) * std::sqrt(2.0 * h3 - h3 * h3)) / M_PI);
  auto h1 = 1.0 - detectorQ(E1Vecs, peak_q, axesRadii);
  // Do not use peak if edge of detector is inside integration radius
  if (h1 > 0.0)
    return false;

  r3 *= m3;
  if (r2 != r1) {
    auto h2 = 1.0 - detectorQ(E1Vecs, peak_q, bkgInnerRadii);
    // scaled from area of circle minus segment when r normalized to 1
    auto m2 = std::sqrt(1.0 - (std::acos(1.0 - h2) - (1.0 - h2) * std::sqrt(2.0 * h2 - h2 * h2)) / M_PI);
    r2 *= m2;
  }

  return true;
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
Mantid::Geometry::PeakShape_const_sptr
Integrate3DEvents::ellipseIntegrateEvents(const std::vector<V3D> &E1Vec, V3D const &peak_q, bool specify_size,
                                          double peak_radius, double back_inner_radius, double back_outer_radius,
                                          std::vector<double> &axes_radii, double &inti, double &sigi) {
  inti = 0.0; // default values, in case something
  sigi = 0.0; // is wrong with the peak.

  int64_t hkl_key = getHklKey(peak_q);

  if (hkl_key == 0) {
    return std::make_shared<NoShape>();
  }

  auto pos = m_event_lists.find(hkl_key);
  if (m_event_lists.end() == pos)
    return std::make_shared<NoShape>();
  ;

  const std::vector<std::pair<std::pair<double, double>, V3D>> &some_events = pos->second;

  if (some_events.size() < 3) // if there are not enough events to
  {                           // find covariance matrix, return
    return std::make_shared<NoShape>();
  }

  DblMatrix cov_matrix(3, 3);
  makeCovarianceMatrix(some_events, cov_matrix, m_radius);

  std::vector<V3D> eigen_vectors;
  std::vector<double> eigen_values;
  getEigenVectors(cov_matrix, eigen_vectors, eigen_values);

  std::vector<double> sigmas(3);
  for (int i = 0; i < 3; i++) {
    sigmas[i] = sqrt(eigen_values[i]);
  }

  bool invalid_peak =
      std::any_of(sigmas.cbegin(), sigmas.cend(), [](const double sigma) { return std::isnan(sigma) || sigma <= 0; });

  if (invalid_peak)                     // if data collapses to a line or
  {                                     // to a plane, the volume of the
    return std::make_shared<NoShape>(); // ellipsoids will be zero.
  }

  return ellipseIntegrateEvents(E1Vec, peak_q, some_events, eigen_vectors, sigmas, specify_size, peak_radius,
                                back_inner_radius, back_outer_radius, axes_radii, inti, sigi);
}

Mantid::Geometry::PeakShape_const_sptr
Integrate3DEvents::ellipseIntegrateModEvents(const std::vector<V3D> &E1Vec, V3D const &peak_q, V3D const &hkl,
                                             V3D const &mnp, bool specify_size, double peak_radius,
                                             double back_inner_radius, double back_outer_radius,
                                             std::vector<double> &axes_radii, double &inti, double &sigi) {
  inti = 0.0; // default values, in case something
  sigi = 0.0; // is wrong with the peak.

  int64_t hkl_key = getHklMnpKey(boost::math::iround<double>(hkl[0]), boost::math::iround<double>(hkl[1]),
                                 boost::math::iround<double>(hkl[2]), boost::math::iround<double>(mnp[0]),
                                 boost::math::iround<double>(mnp[1]), boost::math::iround<double>(mnp[2]));

  if (hkl_key == 0) {
    return std::make_shared<NoShape>();
  }

  auto pos = m_event_lists.find(hkl_key);
  if (m_event_lists.end() == pos)
    return std::make_shared<NoShape>();
  ;

  const std::vector<std::pair<std::pair<double, double>, V3D>> &some_events = pos->second;

  if (some_events.size() < 3) // if there are not enough events to
  {                           // find covariance matrix, return
    return std::make_shared<NoShape>();
  }

  DblMatrix cov_matrix(3, 3);
  if (hkl_key % 1000 == 0)
    makeCovarianceMatrix(some_events, cov_matrix, m_radius);
  else
    makeCovarianceMatrix(some_events, cov_matrix, s_radius);

  std::vector<V3D> eigen_vectors;
  std::vector<double> eigen_values;
  getEigenVectors(cov_matrix, eigen_vectors, eigen_values);

  std::vector<double> sigmas(3);
  for (int i = 0; i < 3; i++)
    sigmas[i] = sqrt(eigen_values[i]);

  bool invalid_peak =
      std::any_of(sigmas.cbegin(), sigmas.cend(), [](const double sigma) { return std::isnan(sigma) || sigma <= 0; });

  if (invalid_peak)                     // if data collapses to a line or
  {                                     // to a plane, the volume of the
    return std::make_shared<NoShape>(); // ellipsoids will be zero.
  }

  return ellipseIntegrateEvents(E1Vec, peak_q, some_events, eigen_vectors, sigmas, specify_size, peak_radius,
                                back_inner_radius, back_outer_radius, axes_radii, inti, sigi);
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
std::pair<double, double>
Integrate3DEvents::numInEllipsoid(std::vector<std::pair<std::pair<double, double>, V3D>> const &events,
                                  std::vector<V3D> const &directions, std::vector<double> const &sizes) {

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
std::pair<double, double>
Integrate3DEvents::numInEllipsoidBkg(std::vector<std::pair<std::pair<double, double>, V3D>> const &events,
                                     std::vector<V3D> const &directions, std::vector<double> const &sizes,
                                     std::vector<double> const &sizesIn, const bool useOnePercentBackgroundCorrection) {
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

  auto endIndex = eventVec.size();
  if (useOnePercentBackgroundCorrection) {
    // Remove top 1% of background
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

void Integrate3DEvents::makeCovarianceMatrix(std::vector<std::pair<std::pair<double, double>, V3D>> const &events,
                                             DblMatrix &matrix, double radius) {
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

/**
 *  Calculate the eigen vectors of a 3x3 real symmetric matrix using the GSL.
 *
 *  @param cov_matrix     3x3 real symmetric matrix.
 *  @param eigen_vectors  The eigen vectors for the matrix are returned
 *                        in this list.
 *  @param eigen_values   3 eigenvalues of matrix
 */
void Integrate3DEvents::getEigenVectors(DblMatrix const &cov_matrix, std::vector<V3D> &eigen_vectors,
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
    key = 1000000000000 * h + 100000000 * k + 10000 * l;

  return key;
}
/**
 *  Form a map key as 10^12*h + 10^6*k + l from the integers h,k,l.
 *
 *  @param  h        The first Miller index
 *  @param  k        The second Miller index
 *  @param  l        The third  Miller index
 *  @param  m        The first modulation index
 *  @param  n        The second modulation index
 *  @param  p        The third  modulation index
 */

int64_t Integrate3DEvents::getHklMnpKey(int h, int k, int l, int m, int n, int p) {
  int64_t key(0);

  if (h != 0 || k != 0 || l != 0 || m != 0 || n != 0 || p != 0)
    key = 1000000000000 * h + 100000000 * k + 10000 * l + 100 * m + 10 * n + p;

  return key;
}
/**
 *  Form a map key for the specified q_vector.  The q_vector is mapped to
 *  h,k,l by UBinv and the map key is then formed from those rounded h,k,l
 *  values.
 *
 *  @param hkl  The q_vector to be mapped to h,k,l
 */
int64_t Integrate3DEvents::getHklKey2(V3D const &hkl) {
  int h = boost::math::iround<double>(hkl[0]);
  int k = boost::math::iround<double>(hkl[1]);
  int l = boost::math::iround<double>(hkl[2]);
  return getHklKey(h, k, l);
}
/**
 *  Form a map key for the specified q_vector.  The q_vector is mapped to
 *  h,k,l by UBinv and the map key is then formed from those rounded h,k,l
 *  values.
 *
 *  @param hkl  The q_vector to be mapped to h,k,l
 */
int64_t Integrate3DEvents::getHklMnpKey2(V3D const &hkl) {
  V3D modvec1 = V3D(m_ModHKL[0][0], m_ModHKL[1][0], m_ModHKL[2][0]);
  V3D modvec2 = V3D(m_ModHKL[0][1], m_ModHKL[1][1], m_ModHKL[2][1]);
  V3D modvec3 = V3D(m_ModHKL[0][2], m_ModHKL[1][2], m_ModHKL[2][2]);
  if (Geometry::IndexingUtils::ValidIndex(hkl, m_radius)) {
    int h = boost::math::iround<double>(hkl[0]);
    int k = boost::math::iround<double>(hkl[1]);
    int l = boost::math::iround<double>(hkl[2]);

    return getHklMnpKey(h, k, l, 0, 0, 0);
  } else if (!crossterm) {
    if (modvec1 != V3D(0, 0, 0))
      for (int order = -maxOrder; order <= maxOrder; order++) {
        if (order == 0)
          continue; // exclude order 0
        V3D hkl1(hkl);
        hkl1[0] -= order * modvec1[0];
        hkl1[1] -= order * modvec1[1];
        hkl1[2] -= order * modvec1[2];
        if (Geometry::IndexingUtils::ValidIndex(hkl1, s_radius)) {
          int h = boost::math::iround<double>(hkl1[0]);
          int k = boost::math::iround<double>(hkl1[1]);
          int l = boost::math::iround<double>(hkl1[2]);
          return getHklMnpKey(h, k, l, order, 0, 0);
        }
      }
    if (modvec2 != V3D(0, 0, 0))
      for (int order = -maxOrder; order <= maxOrder; order++) {
        if (order == 0)
          continue; // exclude order 0
        V3D hkl1(hkl);
        hkl1[0] -= order * modvec2[0];
        hkl1[1] -= order * modvec2[1];
        hkl1[2] -= order * modvec2[2];
        if (Geometry::IndexingUtils::ValidIndex(hkl1, s_radius)) {
          int h = boost::math::iround<double>(hkl1[0]);
          int k = boost::math::iround<double>(hkl1[1]);
          int l = boost::math::iround<double>(hkl1[2]);
          return getHklMnpKey(h, k, l, 0, order, 0);
        }
      }
    if (modvec3 != V3D(0, 0, 0))
      for (int order = -maxOrder; order <= maxOrder; order++) {
        if (order == 0)
          continue; // exclude order 0
        V3D hkl1(hkl);
        hkl1[0] -= order * modvec3[0];
        hkl1[1] -= order * modvec3[1];
        hkl1[2] -= order * modvec3[2];
        if (Geometry::IndexingUtils::ValidIndex(hkl1, s_radius)) {
          int h = boost::math::iround<double>(hkl1[0]);
          int k = boost::math::iround<double>(hkl1[1]);
          int l = boost::math::iround<double>(hkl1[2]);
          return getHklMnpKey(h, k, l, 0, 0, order);
        }
      }
  } else {
    int maxOrder1 = maxOrder;
    if (modvec1 == V3D(0, 0, 0))
      maxOrder1 = 0;
    int maxOrder2 = maxOrder;
    if (modvec2 == V3D(0, 0, 0))
      maxOrder2 = 0;
    int maxOrder3 = maxOrder;
    if (modvec3 == V3D(0, 0, 0))
      maxOrder3 = 0;
    for (int m = -maxOrder1; m <= maxOrder1; m++)
      for (int n = -maxOrder2; n <= maxOrder2; n++)
        for (int p = -maxOrder3; p <= maxOrder3; p++) {
          if (m == 0 && n == 0 && p == 0)
            continue; // exclude 0,0,0
          V3D hkl1(hkl);
          V3D mnp = V3D(m, n, p);
          hkl1 -= m_ModHKL * mnp;
          if (Geometry::IndexingUtils::ValidIndex(hkl1, s_radius)) {
            int h = boost::math::iround<double>(hkl1[0]);
            int k = boost::math::iround<double>(hkl1[1]);
            int l = boost::math::iround<double>(hkl1[2]);
            return getHklMnpKey(h, k, l, m, n, p);
          }
        }
  }
  return 0;
}
/**
 *  Form a map key for the specified q_vector.  The q_vector is mapped to
 *  h,k,l by UBinv and the map key is then formed from those rounded h,k,l
 *  values.
 *
 *  @param q_vector  The q_vector to be mapped to h,k,l
 */
int64_t Integrate3DEvents::getHklKey(V3D const &q_vector) {
  V3D hkl = m_UBinv * q_vector;
  int h = boost::math::iround<double>(hkl[0]);
  int k = boost::math::iround<double>(hkl[1]);
  int l = boost::math::iround<double>(hkl[2]);
  return getHklKey(h, k, l);
}

/**
 *  Form a map key for the specified q_vector of satellite peaks.
 *  The q_vector is mapped to h,k,l by UBinv
 *  and the map key is then formed from those rounded h+-0.5,k+-0.5,l+-0.5 or
 * h,k,l depending on the offset of satellite peak from main peak.
 *
 *  @param q_vector  The q_vector to be mapped to h,k,l
 */
int64_t Integrate3DEvents::getHklMnpKey(V3D const &q_vector) {
  V3D hkl = m_UBinv * q_vector;

  V3D modvec1 = V3D(m_ModHKL[0][0], m_ModHKL[1][0], m_ModHKL[2][0]);
  V3D modvec2 = V3D(m_ModHKL[0][1], m_ModHKL[1][1], m_ModHKL[2][1]);
  V3D modvec3 = V3D(m_ModHKL[0][2], m_ModHKL[1][2], m_ModHKL[2][2]);
  if (Geometry::IndexingUtils::ValidIndex(hkl, m_radius)) {
    int h = boost::math::iround<double>(hkl[0]);
    int k = boost::math::iround<double>(hkl[1]);
    int l = boost::math::iround<double>(hkl[2]);

    return getHklMnpKey(h, k, l, 0, 0, 0);
  } else if (!crossterm) {
    if (modvec1 != V3D(0, 0, 0))
      for (int order = -maxOrder; order <= maxOrder; order++) {
        if (order == 0)
          continue; // exclude order 0
        V3D hkl1(hkl);
        hkl1[0] -= order * modvec1[0];
        hkl1[1] -= order * modvec1[1];
        hkl1[2] -= order * modvec1[2];
        if (Geometry::IndexingUtils::ValidIndex(hkl1, s_radius)) {
          int h = boost::math::iround<double>(hkl1[0]);
          int k = boost::math::iround<double>(hkl1[1]);
          int l = boost::math::iround<double>(hkl1[2]);
          return getHklMnpKey(h, k, l, order, 0, 0);
        }
      }
    if (modvec2 != V3D(0, 0, 0))
      for (int order = -maxOrder; order <= maxOrder; order++) {
        if (order == 0)
          continue; // exclude order 0
        V3D hkl1(hkl);
        hkl1[0] -= order * modvec2[0];
        hkl1[1] -= order * modvec2[1];
        hkl1[2] -= order * modvec2[2];
        if (Geometry::IndexingUtils::ValidIndex(hkl1, s_radius)) {
          int h = boost::math::iround<double>(hkl1[0]);
          int k = boost::math::iround<double>(hkl1[1]);
          int l = boost::math::iround<double>(hkl1[2]);
          return getHklMnpKey(h, k, l, 0, order, 0);
        }
      }
    if (modvec3 != V3D(0, 0, 0))
      for (int order = -maxOrder; order <= maxOrder; order++) {
        if (order == 0)
          continue; // exclude order 0
        V3D hkl1(hkl);
        hkl1[0] -= order * modvec3[0];
        hkl1[1] -= order * modvec3[1];
        hkl1[2] -= order * modvec3[2];
        if (Geometry::IndexingUtils::ValidIndex(hkl1, s_radius)) {
          int h = boost::math::iround<double>(hkl1[0]);
          int k = boost::math::iround<double>(hkl1[1]);
          int l = boost::math::iround<double>(hkl1[2]);
          return getHklMnpKey(h, k, l, 0, 0, order);
        }
      }
  } else {
    int maxOrder1 = maxOrder;
    if (modvec1 == V3D(0, 0, 0))
      maxOrder1 = 0;
    int maxOrder2 = maxOrder;
    if (modvec2 == V3D(0, 0, 0))
      maxOrder2 = 0;
    int maxOrder3 = maxOrder;
    if (modvec3 == V3D(0, 0, 0))
      maxOrder3 = 0;
    for (int m = -maxOrder1; m <= maxOrder1; m++)
      for (int n = -maxOrder2; n <= maxOrder2; n++)
        for (int p = -maxOrder3; p <= maxOrder3; p++) {
          if (m == 0 && n == 0 && p == 0)
            continue; // exclude 0,0,0
          V3D hkl1(hkl);
          V3D mnp = V3D(m, n, p);
          hkl1 -= m_ModHKL * mnp;
          if (Geometry::IndexingUtils::ValidIndex(hkl1, s_radius)) {
            int h = boost::math::iround<double>(hkl1[0]);
            int k = boost::math::iround<double>(hkl1[1]);
            int l = boost::math::iround<double>(hkl1[2]);
            return getHklMnpKey(h, k, l, m, n, p);
          }
        }
  }
  return 0;
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
 * @param hkl_integ
 */
void Integrate3DEvents::addEvent(std::pair<std::pair<double, double>, V3D> event_Q, bool hkl_integ) {
  int64_t hkl_key;
  if (hkl_integ)
    hkl_key = getHklKey2(event_Q.second);
  else
    hkl_key = getHklKey(event_Q.second);

  if (hkl_key == 0) // don't keep events associated with 0,0,0
    return;

  auto peak_it = m_peak_qs.find(hkl_key);
  if (peak_it != m_peak_qs.end()) {
    if (!peak_it->second.nullVector()) {
      if (hkl_integ)
        event_Q.second = event_Q.second - m_UBinv * peak_it->second;
      else
        event_Q.second = event_Q.second - peak_it->second;
      if (event_Q.second.norm() < m_radius) {
        m_event_lists[hkl_key].emplace_back(event_Q);
      }
    }
  }
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
 * @param hkl_integ
 */
void Integrate3DEvents::addModEvent(std::pair<std::pair<double, double>, V3D> event_Q, bool hkl_integ) {
  int64_t hklmnp_key;

  if (hkl_integ)
    hklmnp_key = getHklMnpKey2(event_Q.second);
  else
    hklmnp_key = getHklMnpKey(event_Q.second);

  if (hklmnp_key == 0) // don't keep events associated with 0,0,0
    return;

  auto peak_it = m_peak_qs.find(hklmnp_key);
  if (peak_it != m_peak_qs.end()) {
    if (!peak_it->second.nullVector()) {
      if (hkl_integ)
        event_Q.second = event_Q.second - m_UBinv * peak_it->second;
      else
        event_Q.second = event_Q.second - peak_it->second;

      if (hklmnp_key % 10000 == 0) {
        if (event_Q.second.norm() < m_radius)
          m_event_lists[hklmnp_key].emplace_back(event_Q);
      } else if (event_Q.second.norm() < s_radius) {
        m_event_lists[hklmnp_key].emplace_back(event_Q);
      }
    }
  }
}

/**
 * Integrate a list of events, centered about (0,0,0) given the principal
 * axes for the events and the standard deviations in the directions
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
PeakShapeEllipsoid_const_sptr Integrate3DEvents::ellipseIntegrateEvents(
    const std::vector<V3D> &E1Vec, V3D const &peak_q,
    std::vector<std::pair<std::pair<double, double>, Mantid::Kernel::V3D>> const &ev_list,
    std::vector<V3D> const &directions, std::vector<double> const &sigmas, bool specify_size, double peak_radius,
    double back_inner_radius, double back_outer_radius, std::vector<double> &axes_radii, double &inti, double &sigi) {
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
  } // major axis has the specified size
  else {
    r1 = 3;
    r2 = 3;
    r3 = r2 * 1.25992105; // A factor of 2 ^ (1/3) will make the background
    // shell volume equal to the peak region volume.

    // if necessary restrict the background ellipsoid
    // to lie within the specified sphere, and adjust
    // the other sizes, proportionally
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

  inti = peak_w_back.first - ratio * backgrd.first;
  sigi = sqrt(peak_w_back.second + ratio * ratio * backgrd.second);

  // Make the shape and return it.
  return std::make_shared<const PeakShapeEllipsoid>(directions, abcRadii, abcBackgroundInnerRadii,
                                                    abcBackgroundOuterRadii, Mantid::Kernel::QLab,
                                                    "IntegrateEllipsoids");
}
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
double Integrate3DEvents::detectorQ(const std::vector<V3D> &E1Vec, const V3D &QLabFrame, const std::vector<double> &r) {
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

/** Calculate the radius to use for each axis of the ellipsoid from the
 * parameters provided
 *
 * @param params :: the integration parameters
 * @param max_sigma :: the largest sigma of all axes
 * @return tuple of values representing the radius for each axis.
 */
std::tuple<double, double, double> Integrate3DEvents::calculateRadiusFactors(const IntegrationParameters &params,
                                                                             double max_sigma) const {
  double r1 = 0, r2 = 0, r3 = 0;

  if (!params.specifySize) {
    r1 = 3;
    r2 = 3;
    r3 = r2 * 1.25992105; // A factor of 2 ^ (1/3) will make the background
    // shell volume equal to the peak region volume.

    // if necessary restrict the background ellipsoid
    // to lie within the specified sphere, and adjust
    // the other sizes, proportionally
    if (r3 * max_sigma > params.regionRadius) {
      r3 = params.regionRadius / max_sigma;
      r1 = r3 * 0.79370053f; // This value for r1 and r2 makes the background
      r2 = r1;               // shell volume equal to the peak region volume.
    }
  } else {
    // scale specified sizes by 1/max_sigma
    // so when multiplied by the individual
    // sigmas in different directions, the
    r1 = params.peakRadius / max_sigma;
    r2 = params.backgroundInnerRadius / max_sigma;
    r3 = params.backgroundOuterRadius / max_sigma;
  }

  return std::make_tuple(r1, r2, r3);
}

} // namespace Mantid::MDAlgorithms
