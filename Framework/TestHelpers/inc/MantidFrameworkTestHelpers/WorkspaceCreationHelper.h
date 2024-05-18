// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This header MAY NOT be included in any test from a package below DataObjects
 *    (e.g. Kernel, Geometry, API).
 *  Conversely, this file (and its cpp) MAY NOT be modified to use anything from
 *a
 *  package higher than DataObjects (e.g. any algorithm), even if via the
 *factory.
 *********************************************************************************/
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidGeometry/Instrument/Detector.h"

#include <gmock/gmock.h>

#include <string>
#include <vector>

namespace Mantid {
namespace DataObjects {
class PeaksWorkspace;
class LeanElasticPeaksWorkspace;
} // namespace DataObjects
namespace Kernel {
class Logger;
class V3D;
} // namespace Kernel
} // namespace Mantid

namespace WorkspaceCreationHelper {
/// Create a Fibonacci series
template <typename T> struct FibSeries {
private:
  T x1; /// Initial value 1;
  T x2; /// Initial value 2;
public:
  inline FibSeries() : x1(1), x2(1) {}
  inline T operator()() {
    const T out(x1 + x2);
    x1 = x2;
    x2 = out;
    return out;
  }
};
/** Stub algorithm for doing logging/progress reporting*/
class StubAlgorithm : public Mantid::API::Algorithm {
public:
  StubAlgorithm(size_t nSteps = 100);
  /// Algorithm's name for identification
  const std::string name() const override { return "MockAlgorithm"; }
  /// Algorithm's version for identification
  int version() const override { return 1; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Test"; }
  /// Algorithm's summary.
  const std::string summary() const override { return "Test summary."; }

  Mantid::Kernel::Logger &getLogger() { return g_log; }

  Mantid::API::Progress *getProgress() { return m_Progress.get(); }
  void resetProgress(size_t nSteps) { m_Progress = std::make_unique<Mantid::API::Progress>(this, 0.0, 1.0, nSteps); }

private:
  void init() override {}
  void exec() override {}

  std::unique_ptr<Mantid::API::Progress> m_Progress;
  /// logger -> to provide logging,
  static Mantid::Kernel::Logger &g_log;
};

/// A struct containing the cells of an EPP table row.
struct EPPTableRow {
  /// FindEPP algorithm fitting success status.
  enum class FitStatus { SUCCESS, FAILURE };

  /// Construct a row with the default values.
  EPPTableRow() = default;
  /// Construct a row with default workspace index and errors set to zero.
  EPPTableRow(const double peakCentre, const double sigma, const double height, const FitStatus fitStatus);
  /// Construct a row with errors set to zero.
  EPPTableRow(const int index, const double peakCentre, const double sigma, const double height,
              const FitStatus fitStatus);
  int workspaceIndex = -1;
  double peakCentre = 0;
  double peakCentreError = 0;
  double sigma = 0;
  double sigmaError = 0;
  double height = 0;
  double heightError = 0;
  double chiSq = 0;
  FitStatus fitStatus = FitStatus::SUCCESS;
};

/**
 * Adds a workspace to the ADS
 * @param name :: The name of the workspace
 * @param ws :: The workspace object
 */
template <typename WSType> void storeWS(const std::string &name, WSType &ws) {
  Mantid::API::AnalysisDataService::Instance().add(name, ws);
}
/// Deletes a workspace
void removeWS(const std::string &name);
/// Returns a workspace of a given type
template <typename T> std::shared_ptr<T> getWS(const std::string &name) {
  return Mantid::API::AnalysisDataService::Instance().retrieveWS<T>(name);
}

/// Creates and returns point or bin based histograms with the data specified in
/// parameters
template <typename YType, typename EType>
Mantid::HistogramData::Histogram createHisto(bool isHistogram, YType &&yAxis, EType &&eAxis);
Mantid::DataObjects::Workspace2D_sptr create1DWorkspaceRand(int size, bool isHisto);
Mantid::DataObjects::Workspace2D_sptr create1DWorkspaceConstant(int size, double value, double error, bool isHisto);
Mantid::DataObjects::Workspace2D_sptr create1DWorkspaceFib(int size, bool isHisto);
Mantid::DataObjects::Workspace2D_sptr create1DWorkspaceConstantWithXerror(int size, double value, double error,
                                                                          double xError, bool isHisto = true);
Mantid::DataObjects::Workspace2D_sptr create2DWorkspace(size_t nhist, size_t numBoundaries);
Mantid::DataObjects::Workspace2D_sptr create2DWorkspaceWhereYIsWorkspaceIndex(int nhist, int numBoundaries);
Mantid::DataObjects::Workspace2D_sptr
create2DWorkspace123(int64_t nHist, int64_t nBins, bool isHist = false,
                     const std::set<int64_t> &maskedWorkspaceIndices = std::set<int64_t>(), bool hasDx = false);
Mantid::DataObjects::Workspace2D_sptr
create2DWorkspace154(int64_t nHist, int64_t nBins, bool isHist = false,
                     const std::set<int64_t> &maskedWorkspaceIndices = std::set<int64_t>(), bool hasDx = false);
Mantid::DataObjects::Workspace2D_sptr
create2DWorkspaceWithValuesAndXerror(int64_t nHist, int64_t nBins, bool isHist, double xVal, double yVal, double eVal,
                                     double dxVal,
                                     const std::set<int64_t> &maskedWorkspaceIndices = std::set<int64_t>());
Mantid::DataObjects::Workspace2D_sptr maskSpectra(Mantid::DataObjects::Workspace2D_sptr workspace,
                                                  const std::set<int64_t> &maskedWorkspaceIndices);
/**
 * Create a WorkspaceGroup with N workspaces and the specified parameters
 */
Mantid::API::WorkspaceGroup_sptr createWorkspaceGroup(int nEntries, int nHist, int nBins, const std::string &stem);
/** Create a 2D workspace with this many histograms and bins.
 * Filled with Y = 2.0 and E = sqrt(2.0)w
 */
Mantid::DataObjects::Workspace2D_sptr create2DWorkspaceBinned(size_t nhist, size_t numVals, double x0 = 0.0,
                                                              double deltax = 1.0);

/** Create a 2D workspace with this many point-histograms and bins.
 * Filled with Y = 2.0 and E = M_SQRT2
 */
Mantid::DataObjects::Workspace2D_sptr create2DWorkspacePoints(size_t nhist, size_t numVals, double x0 = 0.0,
                                                              double deltax = 1.0);

/** Create a 2D workspace with this many histograms and bins. The bins are
 * assumed to be non-uniform and given by the input array
 * Filled with Y = 2.0 and E = sqrt(2.0)w
 */
Mantid::DataObjects::Workspace2D_sptr
create2DWorkspaceNonUniformlyBinned(int nhist, const int numBoundaries, const double xBoundaries[], bool hasDx = false);

struct ReturnOne {
  double operator()(const double, std::size_t) { return 1.0; };
};

/**
 * Creates a 2D workspace from taking the function values from the input
 * function. The function type must define operator()(double, int)
 * @param yFunc :: A function to use for the y values
 * @param nSpec :: The number of spectra
 * @param x0 :: The start of the x range
 * @param x1 :: The end of the x range
 * @param dx :: The steps in x
 * @param isHist :: True if it should be a histogram
 * @param eFunc :: A function to use for the y error values
 * @return The new workspace. The errors are set to 1.0
 */
template <typename fT, typename gT = ReturnOne>
Mantid::DataObjects::Workspace2D_sptr create2DWorkspaceFromFunction(fT yFunc, int nSpec, double x0, double x1,
                                                                    double dx, bool isHist = false,
                                                                    gT eFunc = ReturnOne()) {
  int nX = int((x1 - x0) / dx) + 1;
  int nY = nX - (isHist ? 1 : 0);
  if (nY <= 0)
    throw std::invalid_argument("Number of bins <=0. Cannot create an empty workspace");

  auto ws = std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
      Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", nSpec, nX, nY));

  for (int iSpec = 0; iSpec < nSpec; iSpec++) {
    auto &X = ws->mutableX(iSpec);
    auto &Y = ws->mutableY(iSpec);
    auto &E = ws->mutableE(iSpec);
    for (int i = 0; i < nY; i++) {
      double x = x0 + dx * i;
      X[i] = x;
      Y[i] = yFunc(x, iSpec);
      E[i] = eFunc(x, iSpec);
    }
    if (isHist)
      X.back() = X[nY - 1] + dx;
  }
  return ws;
}

/// Add random noise to a 2D workspace.
void addNoise(const Mantid::API::MatrixWorkspace_sptr &ws, double noise, const double lower = -0.5,
              const double upper = 0.5);

/// Create a test workspace with a fully defined instrument.
Mantid::DataObjects::Workspace2D_sptr
create2DWorkspaceWithFullInstrument(int nhist, int nbins, bool includeMonitors = false, bool startYNegative = false,
                                    bool isHistogram = true,
                                    const std::string &instrumentName = std::string("testInst"), bool hasDx = false);

/**
 * Create a workspace as for create2DWorkspaceWithFullInstrument, but including
 *time indexing, i.e. detector scans. Note that no positions or rotations are
 *currently changed for the detector scan workspaces.
 *
 * Data filled with: Y: 2.0, E: sqrt(2.0), X: nbins of width 1 starting at 0
 */
Mantid::API::MatrixWorkspace_sptr
create2DDetectorScanWorkspaceWithFullInstrument(int nhist, int nbins, size_t nTimeIndexes, size_t startTime = 0,
                                                size_t firstInterval = 1, bool includeMonitors = false,
                                                bool startYNegative = false, bool isHistogram = true,
                                                const std::string &instrumentName = std::string("testInst"));

Mantid::DataObjects::Workspace2D_sptr create2DWorkspaceWithGeographicalDetectors(
    const int nlat, const int nlong, const double anginc, int nbins, const double x0 = 0.5, const double deltax = 1.0,
    const std::string &instrumentName = std::string("testInst"), const std::string &xunit = std::string("Momentum"));

/**
 * Create a test workspace with a Theta numeric axis instead of a spectrum axis
 * the values run from 1 to nhist
 * Data filled with: Y: 2.0, E: sqrt(2.0), X: nbins of width 1 starting at 0
 */
Mantid::DataObjects::Workspace2D_sptr create2DWorkspaceThetaVsTOF(int nHist, int nBins);

Mantid::DataObjects::Workspace2D_sptr create2DWorkspaceWithRectangularInstrument(int numBanks, int numPixels,
                                                                                 int numBins);
Mantid::DataObjects::Workspace2D_sptr create2DWorkspace123WithMaskedBin(int numHist, int numBins,
                                                                        int maskedWorkspaceIndex, int maskedBinIndex);

/** Create an Eventworkspace with an instrument that contains
 * RectangularDetector's */
Mantid::DataObjects::EventWorkspace_sptr createEventWorkspaceWithFullInstrument(int numBanks, int numPixels,
                                                                                bool clearEvents = true);

/** Create an Eventworkspace with instrument 2.0 that contains
 * RectangularDetector's */
Mantid::DataObjects::EventWorkspace_sptr createEventWorkspaceWithFullInstrument2(int numBanks, int numPixels,
                                                                                 bool clearEvents = true);

/**
 * Creates an event workspace with instrument which consists of cylindrical
 *detectors.
 *
 * X data: 100 histogrammed bins, starting from 0.0 in steps of 1.0.
 * Y data: 2 ToF events for every bin
 *
 * @param numBanks :: How many detector groups there should be
 * @param clearEvents :: Whether workspace should not contain any events
 * @return Workspace with described type of events (empty if clearEvents) and
 *instrument set
 */
Mantid::DataObjects::EventWorkspace_sptr createEventWorkspaceWithNonUniformInstrument(int numBanks, bool clearEvents);

Mantid::DataObjects::WorkspaceSingleValue_sptr createWorkspaceSingleValue(double value);
Mantid::DataObjects::WorkspaceSingleValue_sptr createWorkspaceSingleValueWithError(double value, double error);
/** Perform some finalization on event workspace stuff */
void eventWorkspace_Finalize(const Mantid::DataObjects::EventWorkspace_sptr &ew);
/** Create event workspace with:
 * 500 pixels
 * 1000 histogrammed bins.
 */
Mantid::DataObjects::EventWorkspace_sptr createEventWorkspace();

/** Create event workspace with:
 * 50 pixels
 * 100 histogrammed bins from 0.0 in steps of 1.0
 * 200 events; two in each bin, at time 0.5, 1.5, etc.
 * PulseTime = 1 second, 2 seconds, etc.
 */
Mantid::DataObjects::EventWorkspace_sptr createEventWorkspace2(int numPixels = 50, int numBins = 100);

Mantid::DataObjects::EventWorkspace_sptr createEventWorkspace(int numPixels, int numBins, int numEvents = 100,
                                                              double x0 = 0.0, double binDelta = 1.0,
                                                              int eventPattern = 1, int start_at_pixelID = 0);

Mantid::DataObjects::EventWorkspace_sptr createEventWorkspaceWithStartTime(
    int numPixels, int numBins, int numEvents = 100, double x0 = 0.0, double binDelta = 1.0, int eventPattern = 1,
    int start_at_pixelID = 0,
    Mantid::Types::Core::DateAndTime run_start = Mantid::Types::Core::DateAndTime("2010-01-01T00:00:00"));

Mantid::DataObjects::EventWorkspace_sptr createGroupedEventWorkspace(std::vector<std::vector<int>> const &groups,
                                                                     int numBins, double binDelta = 1.,
                                                                     double xOffset = 0.);

Mantid::DataObjects::EventWorkspace_sptr createRandomEventWorkspace(size_t numbins, size_t numpixels,
                                                                    double bin_delta = 1.0);

Mantid::API::MatrixWorkspace_sptr createGroupedWorkspace2D(size_t numHist, int numBins, double binDelta);
// grouped workspace with detectors arranges in rings in center and into boxes
// outside
Mantid::API::MatrixWorkspace_sptr createGroupedWorkspace2DWithRingsAndBoxes(size_t RootOfNumHist = 10, int numBins = 10,
                                                                            double binDelta = 1.0);
// not strictly creating a workspace, but really helpful to see what one
// contains
void displayDataY(const Mantid::API::MatrixWorkspace_const_sptr &ws);
// not strictly creating a workspace, but really helpful to see what one
// contains
void displayData(const Mantid::API::MatrixWorkspace_const_sptr &ws);
// not strictly creating a workspace, but really helpful to see what one
// contains
void displayDataX(const Mantid::API::MatrixWorkspace_const_sptr &ws);
// not strictly creating a workspace, but really helpful to see what one
// contains
void displayDataE(const Mantid::API::MatrixWorkspace_const_sptr &ws);

void addTSPEntry(Mantid::API::Run &runInfo, const std::string &name, double val);
void setOrientedLattice(const Mantid::API::MatrixWorkspace_sptr &ws, double a, double b, double c);
void setGoniometer(const Mantid::API::MatrixWorkspace_sptr &ws, double phi, double chi, double omega);

// create workspace which should be result of homering (transform to energy in
// inelastic)
Mantid::API::MatrixWorkspace_sptr createProcessedWorkspaceWithCylComplexInstrument(size_t numPixels = 100,
                                                                                   size_t numBins = 20,
                                                                                   bool has_oriented_lattice = true);

// Create a workspace with all components needed for inelastic analysis;
Mantid::API::MatrixWorkspace_sptr createProcessedInelasticWS(const std::vector<double> &L2,
                                                             const std::vector<double> &polar,
                                                             const std::vector<double> &azimutal, size_t numBins = 4,
                                                             double Emin = -10, double Emax = 10, double Ei = 11);

Mantid::DataObjects::EventWorkspace_sptr
createEventWorkspace3(const Mantid::DataObjects::EventWorkspace_const_sptr &sourceWS, const std::string &wsname,
                      Mantid::API::Algorithm *alg);

/// Function to create a fixed RebinnedOutput workspace
Mantid::DataObjects::RebinnedOutput_sptr createRebinnedOutputWorkspace();

/// Populates a mutable reference from initializer list starting at user
/// specified index
template <typename T>
void populateWsWithInitList(T &destination, size_t startingIndex, const std::initializer_list<double> &values);

/// Create a simple peaks workspace containing the given number of peaks
std::shared_ptr<Mantid::DataObjects::PeaksWorkspace> createPeaksWorkspace(const int numPeaks,
                                                                          const bool createOrientedLattice = false);
/// Create a simple peaks workspace containing the given number of peaks and UB
/// matrix
std::shared_ptr<Mantid::DataObjects::PeaksWorkspace> createPeaksWorkspace(const int numPeaks,
                                                                          const Mantid::Kernel::DblMatrix &ubMat);

/// Create a simple lean peaks workspace containing the given number of peaks
std::shared_ptr<Mantid::DataObjects::LeanElasticPeaksWorkspace>
createLeanPeaksWorkspace(const int numPeaks, const bool createOrientedLattice = false);
/// Create a simple lean peaks workspace containing the given number of peaks and UB
/// matrix
std::shared_ptr<Mantid::DataObjects::LeanElasticPeaksWorkspace>
createLeanPeaksWorkspace(const int numPeaks, const Mantid::Kernel::DblMatrix &ubMat);

/**Build table workspace with preprocessed detectors for existing workspace with
 * instrument */
std::shared_ptr<Mantid::DataObjects::TableWorkspace>
buildPreprocessedDetectorsWorkspace(const Mantid::API::MatrixWorkspace_sptr &ws);
// create range of angular detectors positions
void create2DAngles(std::vector<double> &L2, std::vector<double> &polar, std::vector<double> &azim, size_t nPolar = 10,
                    size_t nAzim = 10, double polStart = 0, double polEnd = 90, double azimStart = -30,
                    double azimEnd = 30);

/// Create a 2D workspace with one detector and one monitor based around a
/// virtual reflectometry instrument.
Mantid::API::MatrixWorkspace_sptr create2DWorkspaceWithReflectometryInstrument(
    const double startX = 0.0, const Mantid::Kernel::V3D &slit1Pos = Mantid::Kernel::V3D(0, 0, 0),
    const Mantid::Kernel::V3D &slit2Pos = Mantid::Kernel::V3D(0, 0, 1), const double vg1 = 0.5, const double vg2 = 1.0,
    const Mantid::Kernel::V3D &sourcePos = Mantid::Kernel::V3D(0, 0, 0),
    const Mantid::Kernel::V3D &monitorPos = Mantid::Kernel::V3D(14, 0, 0),
    const Mantid::Kernel::V3D &samplePos = Mantid::Kernel::V3D(15, 0, 0),
    const Mantid::Kernel::V3D &detectorPos = Mantid::Kernel::V3D(20, (20 - 15), 0), const int nBins = 100,
    const double deltaX = 2000.0);

/// Create a 2D workspace with one monitor and three detectors based around
/// a virtual reflectometry instrument.
Mantid::API::MatrixWorkspace_sptr create2DWorkspaceWithReflectometryInstrumentMultiDetector(
    const double startX = 0.0, const double detSize = 0.0,
    const Mantid::Kernel::V3D &slit1Pos = Mantid::Kernel::V3D(0, 0, 0),
    const Mantid::Kernel::V3D &slit2Pos = Mantid::Kernel::V3D(0, 0, 1), const double vg1 = 0.5, const double vg2 = 1.0,
    const Mantid::Kernel::V3D &sourcePos = Mantid::Kernel::V3D(0, 0, 0),
    const Mantid::Kernel::V3D &monitorPos = Mantid::Kernel::V3D(14, 0, 0),
    const Mantid::Kernel::V3D &samplePos = Mantid::Kernel::V3D(15, 0, 0),
    const Mantid::Kernel::V3D &detectorCenterPos = Mantid::Kernel::V3D(20, (20 - 15), 0), const int nSpectra = 4,
    const int nBins = 20, const double deltaX = 5000.0);

void createInstrumentForWorkspaceWithDistances(const Mantid::API::MatrixWorkspace_sptr &workspace,
                                               const Mantid::Kernel::V3D &samplePosition,
                                               const Mantid::Kernel::V3D &sourcePosition,
                                               const std::vector<Mantid::Kernel::V3D> &detectorPositions);

/// Create a table workspace corresponding to what the FindEPP algorithm gives.
Mantid::API::ITableWorkspace_sptr createEPPTableWorkspace(const std::vector<EPPTableRow> &rows);

/**
 * Create a copy of the SNAP "lite" instrument. This is the normal instrument with the 8x8 pixels added together. The
 * default values for the detector positions are taken from SNAP_57514.
 */
Mantid::API::MatrixWorkspace_sptr createSNAPLiteInstrument(const std::string &wkspName, const double ang1 = -65.3,
                                                           const double ang2 = 104.95);
Mantid::API::MatrixWorkspace_sptr createFocusedSNAPLiteInstrument(const std::string &wkspName,
                                                                  const std::string &groupingAlg,
                                                                  const std::string &groupingDescr,
                                                                  const double ang1 = -65.3,
                                                                  const double ang2 = 104.95);
} // namespace WorkspaceCreationHelper
