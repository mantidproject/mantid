// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/WeightingStrategy.h"
#include "MantidDataObjects/EventWorkspace_fwd.h"
#include "MantidGeometry/IDTypes.h"

namespace Mantid {
namespace Algorithms {
using SpectraDistanceMap = std::map<specnum_t, Mantid::Kernel::V3D>;

/*
Filters spectra detector list by radius.
*/
class MANTID_ALGORITHMS_DLL RadiusFilter {
public:
  /**
  Constructor
  @param cutoff : radius cutoff for filtering
  */
  RadiusFilter(double cutoff) : m_cutoff(cutoff) {
    if (cutoff < 0) {
      throw std::invalid_argument("RadiusFilter - Cannot have a negative cutoff.");
    }
  }
  /**
  Apply the filtering based on radius.
  @param unfiltered : unfiltered spectra-distance map.
  @return filtered spectra-distance map.
  */
  SpectraDistanceMap apply(SpectraDistanceMap &unfiltered) const {
    SpectraDistanceMap neighbSpectra;
    double cutoff{m_cutoff};
    std::copy_if(unfiltered.begin(), unfiltered.end(), std::inserter(neighbSpectra, neighbSpectra.end()),
                 [cutoff](const std::pair<specnum_t, Mantid::Kernel::V3D> &spectraDistance) {
                   return spectraDistance.second.norm() <= cutoff;
                 });
    return neighbSpectra;
  }

private:
  /// Radius cutoff.
  double m_cutoff;
};

/** Smooth neighboring pixels.

  @authors Janik Zikovsky, Vickie Lynch, SNS
  @date Oct 2010
*/
class MANTID_ALGORITHMS_DLL SmoothNeighbours final : public API::Algorithm {
public:
  /// Default constructor
  SmoothNeighbours();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SmoothNeighbours"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Perform a moving-average smoothing by summing spectra of nearest "
           "neighbours over the face of detectors.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SmoothData"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Smoothing"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  void execWorkspace2D();
  void execEvent(Mantid::DataObjects::EventWorkspace_sptr &ws);
  void findNeighboursRectangular();
  void findNeighboursUbiquitous();

  /// Sets the weighting stragegy.
  void setWeightingStrategy(const std::string &strategyName, double &cutOff);
  /// Translate the entered radius into meters.
  double translateToMeters(const std::string &radiusUnits, const double &enteredRadius) const;

  /// Build the instrument/detector setup in workspace
  void setupNewInstrument(API::MatrixWorkspace &outWS) const;

  /// Build the instrument/detector setup in workspace
  void spreadPixels(const API::MatrixWorkspace_sptr &outWS);

  /// Number to sum
  int m_adjX = 0;
  /// Number to sum
  int m_adjY = 0;
  /// Edge pixels to ignore
  int m_edge = 0;
  /// Radius to search nearest neighbours
  double m_radius = 0.0;
  /// Number of neighbours
  int m_nNeighbours = 0;
  /// Weight the neighbours during summing
  std::unique_ptr<WeightingStrategy> m_weightedSum;
  /// PreserveEvents
  bool m_preserveEvents = false;
  ///  expand by pixel IDs
  bool m_expandSumAllPixels = false;
  /// number of output workspace pixels
  size_t m_outWI = 0;
  /// Input workspace
  Mantid::API::MatrixWorkspace_sptr m_inWS;
  /// Each neighbours is specified as a pair with workspace index, weight.
  using weightedNeighbour = std::pair<size_t, double>;
  /// Vector of list of neighbours (with weight) for each workspace index.
  std::vector<std::vector<weightedNeighbour>> m_neighbours;
  /// Progress reporter
  std::unique_ptr<Mantid::API::Progress> m_progress = nullptr;
};

} // namespace Algorithms
} // namespace Mantid
