// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INTEGRATE_3D_EVENTS_H
#define INTEGRATE_3D_EVENTS_H

#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

#include <boost/shared_ptr.hpp>

#include <tuple>
#include <unordered_map>
#include <vector>

namespace Mantid {
namespace Geometry {
class PeakShape;
}
namespace DataObjects {
class PeakShapeEllipsoid;
}
namespace MDAlgorithms {

struct IntegrationParameters {
  std::vector<Kernel::V3D> E1Vectors;
  double backgroundInnerRadius;
  double backgroundOuterRadius;
  double regionRadius;
  double peakRadius;
  bool specifySize;
};

/**
 @class Integrate3DEvents

 This is a low-level class to construct a map with lists of events near
 each peak Q-vector, shifted to be centered at (0,0,0).  A method is also
 provided to find the principal axes of such a list of events, and to
 find the net integrated counts, using ellipsoids with axis lengths
 determined from the standard deviations in the directions of the
 principal axes.

 @author Dennis Mikkelson
 @date   2012-12-19

 */

using EventListMap =
    std::unordered_map<int64_t,
                       std::vector<std::pair<double, Mantid::Kernel::V3D>>>;
using PeakQMap = std::unordered_map<int64_t, Mantid::Kernel::V3D>;

class DLLExport Integrate3DEvents {
public:
  /// Construct object to store events around peaks and integrate peaks
  Integrate3DEvents(
      const std::vector<std::pair<double, Mantid::Kernel::V3D>> &peak_q_list,
      Kernel::DblMatrix const &UBinv, double radius,
      const bool useOnePercentBackgroundCorrection = true);

  Integrate3DEvents(
      const std::vector<std::pair<double, Mantid::Kernel::V3D>> &peak_q_list,
      std::vector<Mantid::Kernel::V3D> const &hkl_list,
      std::vector<Mantid::Kernel::V3D> const &mnp_list,
      Kernel::DblMatrix const &UBinv, Kernel::DblMatrix const &ModHKL,
      double radius_m, double radius_s, int MaxO, const bool CrossT,
      const bool useOnePercentBackgroundCorrection = true);

  /// Add event Q's to lists of events near peaks
  void
  addEvents(std::vector<std::pair<double, Mantid::Kernel::V3D>> const &event_qs,
            bool hkl_integ);

  /// Find the net integrated intensity of a peak, using ellipsoidal volumes
  boost::shared_ptr<const Mantid::Geometry::PeakShape> ellipseIntegrateEvents(
      std::vector<Kernel::V3D> E1Vec, Mantid::Kernel::V3D const &peak_q,
      bool specify_size, double peak_radius, double back_inner_radius,
      double back_outer_radius, std::vector<double> &axes_radii, double &inti,
      double &sigi);

  /// Find the net integrated intensity of a modulated peak, using ellipsoidal
  /// volumes
  boost::shared_ptr<const Mantid::Geometry::PeakShape>
  ellipseIntegrateModEvents(std::vector<Kernel::V3D> E1Vec,
                            Mantid::Kernel::V3D const &peak_q,
                            Mantid::Kernel::V3D const &hkl,
                            Mantid::Kernel::V3D const &mnp, bool specify_size,
                            double peak_radius, double back_inner_radius,
                            double back_outer_radius,
                            std::vector<double> &axes_radii, double &inti,
                            double &sigi);

  /// Find the net integrated intensity of a peak, using ellipsoidal volumes
  std::pair<boost::shared_ptr<const Mantid::Geometry::PeakShape>,
            std::tuple<double, double, double>>
  integrateStrongPeak(const IntegrationParameters &params,
                      const Kernel::V3D &peak_q, double &inti, double &sigi);

  boost::shared_ptr<const Geometry::PeakShape>
  integrateWeakPeak(const IntegrationParameters &params,
                    Mantid::DataObjects::PeakShapeEllipsoid_const_sptr shape,
                    const std::tuple<double, double, double> &libPeak,
                    const Mantid::Kernel::V3D &peak_q, double &inti,
                    double &sigi);

  double estimateSignalToNoiseRatio(const IntegrationParameters &params,
                                    const Mantid::Kernel::V3D &center);

private:
  /// Get a list of events for a given Q
  const std::vector<std::pair<double, Mantid::Kernel::V3D>> *
  getEvents(const Mantid::Kernel::V3D &peak_q);

  bool correctForDetectorEdges(std::tuple<double, double, double> &radii,
                               const std::vector<Mantid::Kernel::V3D> &E1Vecs,
                               const Mantid::Kernel::V3D &peak_q,
                               const std::vector<double> &axesRadii,
                               const std::vector<double> &bkgInnerRadii,
                               const std::vector<double> &bkgOuterRadii);

  /// Calculate the number of events in an ellipsoid centered at 0,0,0
  static double numInEllipsoid(
      std::vector<std::pair<double, Mantid::Kernel::V3D>> const &events,
      std::vector<Mantid::Kernel::V3D> const &directions,
      std::vector<double> const &sizes);

  /// Calculate the number of events in an ellipsoid centered at 0,0,0
  static double numInEllipsoidBkg(
      std::vector<std::pair<double, Mantid::Kernel::V3D>> const &events,
      std::vector<Mantid::Kernel::V3D> const &directions,
      std::vector<double> const &sizes, std::vector<double> const &sizesIn,
      const bool useOnePercentBackgroundCorrection);

  /// Calculate the 3x3 covariance matrix of a list of Q-vectors at 0,0,0
  static void makeCovarianceMatrix(
      std::vector<std::pair<double, Mantid::Kernel::V3D>> const &events,
      Kernel::DblMatrix &matrix, double radius);

  /// Calculate the eigen vectors of a 3x3 real symmetric matrix
  static void getEigenVectors(Kernel::DblMatrix const &cov_matrix,
                              std::vector<Mantid::Kernel::V3D> &eigen_vectors);

  /// Calculate the standard deviation of 3D events in a specified direction
  static double
  stdDev(std::vector<std::pair<double, Mantid::Kernel::V3D>> const &events,
         Mantid::Kernel::V3D const &direction, double radius);

  /// Form a map key as 10^12*h + 10^6*k + l from the integers h, k, l
  static int64_t getHklKey(int h, int k, int l);

  static int64_t getHklMnpKey(int h, int k, int l, int m, int n, int p);

  /// Form a map key for the specified q_vector.
  int64_t getHklKey(Mantid::Kernel::V3D const &q_vector);
  int64_t getHklMnpKey(Mantid::Kernel::V3D const &q_vector);
  int64_t getHklKey2(Mantid::Kernel::V3D const &hkl);
  int64_t getHklMnpKey2(Mantid::Kernel::V3D const &hkl);

  /// Add an event to the vector of events for the closest h,k,l
  void addEvent(std::pair<double, Mantid::Kernel::V3D> event_Q, bool hkl_integ);
  void addModEvent(std::pair<double, Mantid::Kernel::V3D> event_Q,
                   bool hkl_integ);

  /// Find the net integrated intensity of a list of Q's using ellipsoids
  boost::shared_ptr<const Mantid::DataObjects::PeakShapeEllipsoid>
  ellipseIntegrateEvents(
      std::vector<Kernel::V3D> E1Vec, Kernel::V3D const &peak_q,
      std::vector<std::pair<double, Mantid::Kernel::V3D>> const &ev_list,
      std::vector<Mantid::Kernel::V3D> const &directions,
      std::vector<double> const &sigmas, bool specify_size, double peak_radius,
      double back_inner_radius, double back_outer_radius,
      std::vector<double> &axes_radii, double &inti, double &sigi);

  /// Compute if a particular Q falls on the edge of a detector
  double detectorQ(std::vector<Kernel::V3D> E1Vec,
                   const Mantid::Kernel::V3D QLabFrame,
                   const std::vector<double> &r);

  std::tuple<double, double, double>
  calculateRadiusFactors(const IntegrationParameters &params,
                         double max_sigma) const;

  // Private data members

  PeakQMap m_peak_qs;         // hashtable with peak Q-vectors
  EventListMap m_event_lists; // hashtable with lists of events for each peak
  Kernel::DblMatrix m_UBinv;  // matrix mapping from Q to h,k,l
  Kernel::DblMatrix m_ModHKL; // matrix mapping from Q to m,n,p
  double m_radius;            // size of sphere to use for events around a peak
  double s_radius;            // size of sphere to use for events around a peak
  int maxOrder;
  const bool crossterm;
  const bool m_useOnePercentBackgroundCorrection =
      true; // if one perecent culling of the background should be performed.
};

} // namespace MDAlgorithms

} // namespace Mantid

#endif // INTEGRATE_3D_EVENTS_H
