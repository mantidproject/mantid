// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/InstrumentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

namespace MuonWorkspaceCreationHelper {

// Create y-values for a fake muon dataset.
struct yDataAsymmetry {
  yDataAsymmetry();
  yDataAsymmetry(const double amp, const double phi);
  double operator()(const double t, size_t spec);

private:
  double m_amp;                                                     // Amplitude of the oscillations
  double m_phi;                                                     // Phase of the sinusoid
  const double m_omega = 5.0;                                       // Frequency of the oscillations
  const double tau = Mantid::PhysicalConstants::MuonLifetime * 1e6; // Muon life time in microseconds
};

// Generate y-values which increment by 1 each time the function is called
struct yDataCounts {
  yDataCounts();
  double operator()(const double t, size_t spec);

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
 * @param nspec :: The number of spectra
 * @param maxt ::  The number of histogram bin edges (between 0.0 and 1.0).
 * Number of bins = maxt - 1 .
 * @param dataGenerator :: A function object which takes a double (t) and
 * integer (spectrum number) and gives back a double (the y-value for the data).
 * @return Pointer to the workspace.
 */
template <typename Function = yDataAsymmetry>
Mantid::API::MatrixWorkspace_sptr createAsymmetryWorkspace(std::size_t nspec, std::size_t maxt,
                                                           Function dataGenerator = yDataAsymmetry()) {
  Mantid::API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
      dataGenerator, static_cast<int>(nspec), 0.0, 1.0, (1.0 / static_cast<double>(maxt)), true, eData());

  for (auto g = 0u; g < nspec; ++g) {
    auto &spec = ws->getSpectrum(g);
    spec.addDetectorID(g + 1);
    spec.setSpectrumNo(g + 1);
  }
  // Add number of good frames (required for Asymmetry calculation)
  ws->mutableRun().addProperty("goodfrm", 10);
  // Add instrument and run number
  std::shared_ptr<Mantid::Geometry::Instrument> inst1 = std::make_shared<Mantid::Geometry::Instrument>();
  inst1->setName("EMU");
  ws->setInstrument(inst1);
  ws->mutableRun().addProperty("run_number", 12345);

  return ws;
}

/**
 * Create a matrix workspace appropriate for Group Counts. One detector per
 * spectra, numbers starting from 1. The detector ID and spectrum number are
 * equal. Y values increase from 0 in integer steps.
 */
Mantid::API::MatrixWorkspace_sptr createCountsWorkspace(size_t nspec, size_t maxt, double seed,
                                                        size_t detectorIDseed = 1);

Mantid::API::MatrixWorkspace_sptr createCountsWorkspace(size_t nspec, size_t maxt, double seed, size_t detectorIDseed,
                                                        bool hist, double xStart, double xEnd);

/**
 * Create a WorkspaceGroup and add to the ADS, populate with
 * MatrixWorkspaces simulating periods as used in muon analysis. Workspace
 * for period i has a name ending _i.
 */
Mantid::API::WorkspaceGroup_sptr createMultiPeriodWorkspaceGroup(const int &nPeriods, size_t nspec, size_t maxt,
                                                                 const std::string &wsGroupName);

Mantid::API::WorkspaceGroup_sptr createMultiPeriodAsymmetryData(const int &nPeriods, size_t nspec, size_t maxt,
                                                                const std::string &wsGroupName);

/**
 * Create a simple dead time TableWorkspace with two columns (spectrum
 * number and dead time).
 */
Mantid::API::ITableWorkspace_sptr createDeadTimeTable(const size_t &nspec, const std::vector<double> &deadTimes);

/**
 * Create a simple time zero TableWorkspace with one column (time zero)
 */
Mantid::API::ITableWorkspace_sptr createTimeZeroTable(const size_t &numSpec, const std::vector<double> &timeZeros);

// Creates a single - point workspace with instrument and runNumber set.
Mantid::API::MatrixWorkspace_sptr createWorkspaceWithInstrumentandRun(const std::string &instrName, int runNumber,
                                                                      size_t nSpectra = 1);

// Creates a grouped workspace containing multiple workspaces with multiple
// spectra, the detector IDs are consequtive across the spectra, starting from 1
Mantid::API::WorkspaceGroup_sptr createWorkspaceGroupConsecutiveDetectorIDs(const int &nWorkspaces, size_t nspec,
                                                                            size_t maxt,
                                                                            const std::string &wsGroupName);

} // namespace MuonWorkspaceCreationHelper
