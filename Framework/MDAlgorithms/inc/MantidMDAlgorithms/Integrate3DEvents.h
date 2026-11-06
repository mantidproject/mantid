// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeakShapeEllipsoid_fwd.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include "MantidMDAlgorithms/DllConfig.h"
#include "MantidMDAlgorithms/EllipsoidTrajectoryNorm.h"

#include <memory>

#include <tuple>
#include <unordered_map>
#include <vector>

/**
 * Fits a Gaussian peak and constant background using the supplied ellipsoidal
 * shape and event data.
 *
 * @param shape Ellipsoidal peak shape whose principal radii define the Gaussian
 *              standard deviations.
 * @param peak_q Peak center in Q-space.
 * @param adjustCenter Whether to apply a bounded center refinement.
 * @param inti Output fitted peak intensity.
 * @param sigi Output uncertainty of the fitted intensity.
 */
namespace Mantid {
namespace Geometry {
class PeakShape;
}
namespace MDAlgorithms {

/** @struct IntegrationParameters
 *  @brief Parameters for controlling peak integration
 *
 *  This structure contains the parameters needed to control the integration
 *  of peaks using ellipsoidal volumes.
 */
struct IntegrationParameters {
  std::vector<Kernel::V3D> E1Vectors; ///< Vectors for calculating detector edges
  double backgroundInnerRadius;       ///< Inner radius of background shell
  double backgroundOuterRadius;       ///< Outer radius of background shell
  double regionRadius;                ///< Radius of region to search for events
  double peakRadius;                  ///< Radius of peak ellipsoid
  bool specifySize;                   ///< If true, use specified sizes; if false, use data-driven sizes
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
    std::unordered_map<int64_t, std::vector<std::pair<std::pair<double, double>, Mantid::Kernel::V3D>>>;
using PeakQMap = std::unordered_map<int64_t, Mantid::Kernel::V3D>;

/// Quantities from a flux/solid-angle normalized profile fit, for diagnostics
struct ProfileFitNormalization {
  /// summed event weight in the fit region
  double counts{0.0};
  /// fitted background counts in the fit region
  double backgroundCounts{0.0};
  /// expected counts per unit integrated intensity of the fitted peak
  double peakNorm{0.0};
  /// normalization of the fit region
  double regionNorm{0.0};
  /// coverage of the fit region
  EllipsoidNormResult coverage;
};

/// Options and results for refining a peak's shape in a profile fit
struct ProfileFitShape {
  /// refine the shape only if the peak's intensity is at least this many standard deviations
  double minSignalToNoise{10.0};
  /// each principal standard deviation may change by at most this factor from the supplied shape's
  double maxChange{2.0};
  /// with the center, it may move at most this many of the supplied shape's standard deviations (in its own frame)
  double maxCenterShift{3.0};
  /// whether the peak was strong enough for its shape to be refined
  bool attempted{false};
  /// whether the shape was refined (and the refinement kept)
  bool adjusted{false};
  /// principal axes and standard deviations of the refined shape, or of the supplied one if not refined
  DataObjects::PeakEllipsoidFrame directions;
  DataObjects::PeakEllipsoidExtent sigmas;
};

/// Weighted event sums inside a peak ellipsoid and its background shell
struct ShapeCounts {
  double peak{0.0};
  double peakErrorSq{0.0};
  double shell{0.0};
  double shellErrorSq{0.0};
};

class MANTID_MDALGORITHMS_DLL Integrate3DEvents {
public:
  /// Construct object to store events around peaks and integrate peaks
  Integrate3DEvents(const std::vector<std::pair<std::pair<double, double>, Mantid::Kernel::V3D>> &peak_q_list,
                    Kernel::DblMatrix UBinv, double radius, const bool useOnePercentBackgroundCorrection = true);

  /// Construct object to store events around peaks, each with its nearest peak, without indexing them
  Integrate3DEvents(const std::vector<std::pair<std::pair<double, double>, Mantid::Kernel::V3D>> &peak_q_list,
                    double radius, const bool useOnePercentBackgroundCorrection = true);

  Integrate3DEvents(const std::vector<std::pair<std::pair<double, double>, Mantid::Kernel::V3D>> &peak_q_list,
                    std::vector<Mantid::Kernel::V3D> const &hkl_list, std::vector<Mantid::Kernel::V3D> const &mnp_list,
                    Kernel::DblMatrix UBinv, Kernel::DblMatrix ModHKL, double radius_m, double radius_s, int MaxO,
                    const bool CrossT, const bool useOnePercentBackgroundCorrection = true);

  /// Add event Q's to lists of events near peaks
  void addEvents(std::vector<std::pair<std::pair<double, double>, Mantid::Kernel::V3D>> const &event_qs,
                 bool hkl_integ);

  /// Find the net integrated intensity of a peak, using ellipsoidal volumes
  std::shared_ptr<const Mantid::Geometry::PeakShape>
  ellipseIntegrateEvents(const std::vector<Kernel::V3D> &E1Vec, Mantid::Kernel::V3D const &peak_q, bool specify_size,
                         double peak_radius, double back_inner_radius, double back_outer_radius,
                         DataObjects::PeakEllipsoidExtent &axes_radii, double &inti, double &sigi);

  /// Find the net integrated intensity of a modulated peak, using ellipsoidal
  /// volumes
  std::shared_ptr<const Mantid::Geometry::PeakShape>
  ellipseIntegrateModEvents(const std::vector<Kernel::V3D> &E1Vec, Mantid::Kernel::V3D const &peak_q,
                            Mantid::Kernel::V3D const &hkl, Mantid::Kernel::V3D const &mnp, bool specify_size,
                            double peak_radius, double back_inner_radius, double back_outer_radius,
                            DataObjects::PeakEllipsoidExtent &axes_radii, double &inti, double &sigi);

  /// Find the net integrated intensity of a strong peak, using ellipsoidal volumes
  /// fit from the events, and return peak shape and library peak parameters
  std::pair<std::shared_ptr<const Mantid::Geometry::PeakShape>, std::tuple<double, double, double>>
  integrateStrongPeak(const IntegrationParameters &params, const Kernel::V3D &peak_q, double &inti, double &sigi);

  /// Integrate a weak peak using a shape from a library peak, scaling the
  /// shape by the fractional intensity from the library peak
  std::shared_ptr<const Geometry::PeakShape> integrateWeakPeak(const IntegrationParameters &params,
                                                               Mantid::DataObjects::PeakShapeEllipsoid_const_sptr shape,
                                                               const std::tuple<double, double, double> &libPeak,
                                                               const Mantid::Kernel::V3D &peak_q, double &inti,
                                                               double &sigi);

  /// Integrate a peak using a shape supplied by the caller (e.g. from an
  /// already-integrated PeaksWorkspace) instead of one fit from the events,
  /// used exactly as supplied and centered on peak_q.
  void integrateUsingShape(const Mantid::DataObjects::PeakShapeEllipsoid &shape, const Mantid::Kernel::V3D &peak_q,
                           double &inti, double &sigi);

  /// Weighted event sums (and error-squared sums) inside the peak ellipsoid
  /// and the background shell of a shape supplied by the caller, centered on
  /// peak_q, without the one percent background correction.
  ShapeCounts countUsingShape(const Mantid::DataObjects::PeakShapeEllipsoid &shape, const Mantid::Kernel::V3D &peak_q);

  /// Integrate a peak using a shape supplied by the caller, by maximizing
  /// the Poisson log-likelihood of a Gaussian peak (amplitude fit) plus a
  /// flat background rate against the raw events, instead of counting
  /// events inside/outside ellipsoidal boundaries. The shape's peak radii
  /// are interpreted as the Gaussian's standard deviations (1-sigma) along
  /// its principal axes; the shape's background radii are not used, since
  /// the background rate is fit directly instead. shape.translation() seeds
  /// the fit center (an offset from peak_q), so a previously found
  /// correction is honored rather than discarded. If adjustCenter is true,
  /// also refines the center by a bounded, coordinate-ascent Gauss-Newton
  /// step (capped at one standard deviation of incremental shift from
  /// shape.translation()) -- a slight correction, not a free centroid
  /// search. center returns the offset from peak_q actually used for the
  /// fit (unchanged from shape.translation() if adjustCenter is false), for
  /// the caller to persist as the output shape's translation if it wants to.
  /// If norm is given, the expected counts are weighted by the flux and
  /// solid angle along each detector pixel's trajectory through the fit
  /// region (a sphere of the search radius about peak_q), so the intensity
  /// is in normalized units, and normalization returns diagnostics. If
  /// shapeFit is given and the peak is strong enough, the shape (and with
  /// adjustCenter the center) is refined too, within shapeFit's bounds; a
  /// weaker peak then keeps both its shape and its center.
  void integrateUsingShapeProfileFit(const Mantid::DataObjects::PeakShapeEllipsoid &shape,
                                     const Mantid::Kernel::V3D &peak_q, bool adjustCenter, Mantid::Kernel::V3D &center,
                                     double &inti, double &sigi, const EllipsoidTrajectoryNorm *norm = nullptr,
                                     ProfileFitNormalization *normalization = nullptr,
                                     ProfileFitShape *shapeFit = nullptr);

  /// Estimate the signal-to-noise ratio for a peak at the given position
  /// by fitting an ellipsoid to the events and comparing peak to background
  double estimateSignalToNoiseRatio(const IntegrationParameters &params, const Mantid::Kernel::V3D &center,
                                    bool forceSpherical = false, double sphericityTol = 0.02);

private:
  /// Refine a peak's shape, and optionally its center, by maximizing the profile likelihood
  static void refineProfileShape(const std::vector<std::pair<std::pair<double, double>, Mantid::Kernel::V3D>> &events,
                                 const Mantid::Kernel::V3D &peak_q, const EllipsoidTrajectoryNorm *norm,
                                 const EllipsoidChords &region, double volume, bool adjustCenter,
                                 const DataObjects::PeakEllipsoidFrame &priorDirections,
                                 const DataObjects::PeakEllipsoidExtent &priorSigmas, double maxChange,
                                 const Mantid::Kernel::V3D &origin, double maxCenterShift, Mantid::Kernel::V3D &center,
                                 double &intensity, double &background, double &sigma, double &peakNorm,
                                 DataObjects::PeakEllipsoidFrame &directions, DataObjects::PeakEllipsoidExtent &sigmas);

  /// Get a list of events for a given Q
  const std::vector<std::pair<std::pair<double, double>, Mantid::Kernel::V3D>> *
  getEvents(const Mantid::Kernel::V3D &peak_q);

  /// Correct integration radii if peak extends to detector edges
  /// and return false if peak center is off detector
  bool correctForDetectorEdges(std::tuple<double, double, double> &radii,
                               const std::vector<Mantid::Kernel::V3D> &E1Vecs, const Mantid::Kernel::V3D &peak_q,
                               const DataObjects::PeakEllipsoidExtent &axesRadii,
                               const DataObjects::PeakEllipsoidExtent &bkgInnerRadii,
                               const DataObjects::PeakEllipsoidExtent &bkgOuterRadii);

  /// Calculate the number of events in an ellipsoid centered at 0,0,0
  static std::pair<double, double>
  numInEllipsoid(std::vector<std::pair<std::pair<double, double>, Mantid::Kernel::V3D>> const &events,
                 DataObjects::PeakEllipsoidFrame const &directions, DataObjects::PeakEllipsoidExtent const &sizes);

  /// Calculate the number of events in an ellipsoid centered at 0,0,0
  static std::pair<double, double>
  numInEllipsoidBkg(std::vector<std::pair<std::pair<double, double>, Mantid::Kernel::V3D>> const &events,
                    DataObjects::PeakEllipsoidFrame const &directions, DataObjects::PeakEllipsoidExtent const &sizes,
                    DataObjects::PeakEllipsoidExtent const &sizesIn, const bool useOnePercentBackgroundCorrection);

  /// Calculate the 3x3 covariance matrix of a list of Q-vectors at 0,0,0
  static void makeCovarianceMatrix(std::vector<std::pair<std::pair<double, double>, Mantid::Kernel::V3D>> const &events,
                                   Kernel::DblMatrix &matrix, double radius);

  /// Calculate the eigen vectors of a 3x3 real symmetric matrix
  static void getEigenVectors(Kernel::DblMatrix const &cov_matrix, std::array<Mantid::Kernel::V3D, 3> &eigen_vectors,
                              DataObjects::PeakEllipsoidExtent &eigen_values);

  /// Form a map key as 10^12*h + 10^6*k + l from the integers h, k, l
  static int64_t getHklKey(int h, int k, int l);

  static int64_t getHklMnpKey(int h, int k, int l, int m, int n, int p);

  /// Form a map key for the specified q_vector.
  int64_t getHklKey(Mantid::Kernel::V3D const &q_vector);
  int64_t getHklMnpKey(Mantid::Kernel::V3D const &q_vector);
  int64_t getHklKey2(Mantid::Kernel::V3D const &hkl);
  int64_t getHklMnpKey2(Mantid::Kernel::V3D const &hkl);

  /// Form a map key for the peak of a q_vector: its h,k,l, or the nearest peak's index without indexing
  int64_t getPeakKey(Mantid::Kernel::V3D const &q_vector);
  /// One more than the index of the nearest peak within the radius of q_vector, or 0 if none
  int64_t getNearestPeakKey(Mantid::Kernel::V3D const &q_vector) const;
  /// Key of the cell of the grid of peak centers with integer coordinates i, j, k
  static int64_t getCellKey(int64_t i, int64_t j, int64_t k);

  /// Add an event to the vector of events for the closest h,k,l
  void addEvent(std::pair<std::pair<double, double>, Mantid::Kernel::V3D> event_Q, bool hkl_integ);
  void addModEvent(std::pair<std::pair<double, double>, Mantid::Kernel::V3D> event_Q, bool hkl_integ);

  /// Find the net integrated intensity of a list of Q's using ellipsoids
  DataObjects::PeakShapeEllipsoid_const_sptr
  ellipseIntegrateEvents(const std::vector<Kernel::V3D> &E1Vec, Kernel::V3D const &peak_q,
                         std::vector<std::pair<std::pair<double, double>, Kernel::V3D>> const &ev_list,
                         DataObjects::PeakEllipsoidFrame const &directions, std::array<double, 3> const &sigmas,
                         bool specify_size, double peak_radius, double back_inner_radius, double back_outer_radius,
                         DataObjects::PeakEllipsoidExtent &axes_radii, double &inti, double &sigi);

  /// Compute if a particular Q falls on the edge of a detector
  double detectorQ(const std::vector<Kernel::V3D> &E1Vec, const Mantid::Kernel::V3D &QLabFrame,
                   const DataObjects::PeakEllipsoidExtent &r);

  std::tuple<double, double, double> calculateRadiusFactors(const IntegrationParameters &params,
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
  // assign events to the nearest peak (within m_radius) instead of the nearest h,k,l
  bool m_nearestPeak{false};
  std::vector<Kernel::V3D> m_peakCenters;                       // peak Q-vectors, by index
  std::unordered_map<int64_t, std::vector<size_t>> m_peakCells; // peak indices in cells of side m_radius
};

} // namespace MDAlgorithms

} // namespace Mantid
