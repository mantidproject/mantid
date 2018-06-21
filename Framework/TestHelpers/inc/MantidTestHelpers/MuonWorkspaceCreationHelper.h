#ifndef MUONWORKSPACECREATIONHELPER_H_
#define MUONWORKSPACECREATIONHELPER_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/ScopedFileHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

namespace MuonWorkspaceCreationHelper {

// Create y-values for a fake muon dataset.
struct yDataAsymmetry {
  yDataAsymmetry();
  yDataAsymmetry(const double amp, const double phi);
  double operator()(const double t, size_t spec);

private:
  double m_amp;               // Amplitude of the oscillations
  double m_phi;               // Phase of the sinusoid
  const double m_omega = 5.0; // Frequency of the oscillations
  const double tau = Mantid::PhysicalConstants::MuonLifetime *
                     1e6; // Muon life time in microseconds
};

// Generate y-values which increment by 1 each time the function is called
struct yDataCounts2 {
  yDataCounts2();
  double operator()(const double, size_t);

private:
  int m_count;
};

// Errors
struct eData {
  double operator()(const double, size_t);
};

/**
 * Create a matrix workspace appropriate for Group Asymmetry. One detector per
 * spectra, numbers starting from 1. The detector ID and spectrum number are
 * equal.
 */
template <typename Function>
Mantid::API::MatrixWorkspace_sptr
createAsymmetryWorkspace(size_t nspec, size_t maxt, Function dataGenerator);

Mantid::API::MatrixWorkspace_sptr createAsymmetryWorkspace(size_t nspec,
                                                           size_t maxt);

/**
 * Create a matrix workspace appropriate for Group Counts. One detector per
 * spectra, numbers starting from 1. The detector ID and spectrum number are
 * equal. Y values increase from 0 in integer steps.
 */
Mantid::API::MatrixWorkspace_sptr
createCountsWorkspace(size_t nspec, size_t maxt, double seed);

/**
 * Create a WorkspaceGroup and add to the ADS, populate with MatrixWorkspaces
 * simulating periods as used in muon analysis. Workspace for period i has a
 * name ending _i.
 */
Mantid::API::WorkspaceGroup_sptr
createMultiPeriodWorkspaceGroup(const int &nPeriods, size_t nspec, size_t maxt,
                                const std::string &wsGroupName);

/**
 * Create a simple dead time TableWorkspace with two columns (spectrum number
 * and dead time).
 */
Mantid::API::ITableWorkspace_sptr
createDeadTimeTable(const size_t &nspec, std::vector<double> &deadTimes);

} // namespace MuonWorkspaceCreationHelper

#endif /*MUONWORKSPACECREATIONHELPER_H_*/
