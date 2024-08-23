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
#include "MantidAPI/SpectrumInfo.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidCrystal/FindSXPeaksHelper.h"
#include "MantidDataObjects/PeaksWorkspace.h"

#include <vector>

namespace Mantid {
namespace Crystal {

using peakvector = std::vector<FindSXPeaksHelper::SXPeak>;

/** Search detector space for single crystal peaks.

  This algorithm takes a 2D workspace in TOF or d-spacing as input and uses
  one of a selection of peak finding & background strategies to search for
  peaks in each 1D spectrum. Peaks found in multiple spectra within a given
  resolution of one another will be merged into a single peak.

  The algorithm creates a peaks workspace containing all of the information
  about the found peaks. The returned peaks workspace will be unindexed.

  @author L C Chapon, ISIS, Rutherford Appleton Laboratory
  @date 11/08/2009
*/
class MANTID_CRYSTAL_DLL FindSXPeaks final : public API::Algorithm {
public:
  /// Default constructor
  FindSXPeaks();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FindSXPeaks"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Takes a 2D workspace as input and finds the highest intensity "
           "point in each 1D spectrum. This is used in particular for single "
           "crystal as a quick way to find strong peaks.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"IndexSXPeaks"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Crystal\\Peaks;Optimization\\PeakFinding"; }

  static const std::string strongestPeakStrategy;
  static const std::string allPeaksStrategy;
  static const std::string allPeaksNSigmaStrategy;
  static const std::string relativeResolutionStrategy;
  static const std::string absoluteResolutionPeaksStrategy;

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;

  /// Selects a background strategy
  std::unique_ptr<FindSXPeaksHelper::BackgroundStrategy> getBackgroundStrategy() const;

  /// Selects a peak finding strategy
  std::unique_ptr<FindSXPeaksHelper::PeakFindingStrategy>
  getPeakFindingStrategy(const FindSXPeaksHelper::BackgroundStrategy *backgroundStrategy,
                         const API::SpectrumInfo &spectrumInfo, const double minValue, const double maxValue,
                         const FindSXPeaksHelper::XAxisUnit tofUnits = FindSXPeaksHelper::XAxisUnit::TOF) const;

  /// Selects a peak finding strategy
  std::unique_ptr<FindSXPeaksHelper::ReducePeakListStrategy>
  getReducePeakListStrategy(const FindSXPeaksHelper::CompareStrategy *compareStrategy) const;

  /// Selects a comparison strategy
  std::unique_ptr<FindSXPeaksHelper::CompareStrategy> getCompareStrategy() const;

  //
  void reducePeakList(const peakvector &, Mantid::API::Progress &progress);

  /// Check what x units this workspace has
  FindSXPeaksHelper::XAxisUnit getWorkspaceXAxisUnit(const Mantid::API::MatrixWorkspace_const_sptr &workspace) const;

  /// The value in X to start the search from
  double m_MinRange;
  /// The value in X to finish the search at
  double m_MaxRange;
  /// The spectrum to start the integration from
  size_t m_MinWsIndex;
  /// The spectrum to finish the integration at
  size_t m_MaxWsIndex;
  // The peaks workspace that contains the peaks information.
  Mantid::DataObjects::PeaksWorkspace_sptr m_peaks;
};

} // namespace Crystal
} // namespace Mantid
