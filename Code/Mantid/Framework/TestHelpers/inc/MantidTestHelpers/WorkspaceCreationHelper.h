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
#ifndef WORKSPACECREATIONHELPER_H_
#define WORKSPACECREATIONHELPER_H_
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument/Detector.h"

namespace Mantid {
namespace DataObjects {
class PeaksWorkspace;
}
}

namespace WorkspaceCreationHelper {
/// Create a fibonacci series
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
/** mock algorithn for doing logging/progress reporting*/
class MockAlgorithm : public Mantid::API::Algorithm {
public:
  MockAlgorithm(size_t nSteps = 100);
  ~MockAlgorithm(){};

  /// Algorithm's name for identification
  virtual const std::string name() const { return "MockAlgorithm"; };
  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Test"; }
  /// Algorithm's summary.
  virtual const std::string summary() const { return "Test summary."; }

  Mantid::Kernel::Logger &getLogger() { return g_log; }

  Mantid::API::Progress *getProgress() { return m_Progress.get(); }
  void resetProgress(size_t nSteps) {
    m_Progress = std::auto_ptr<Mantid::API::Progress>(
        new Mantid::API::Progress(this, 0, 1, nSteps));
  }

private:
  void init(){};
  void exec(){};

  std::auto_ptr<Mantid::API::Progress> m_Progress;
  /// logger -> to provide logging,
  static Mantid::Kernel::Logger &g_log;
};

/// Adds a workspace to the ADS
void storeWS(const std::string &name, Mantid::API::Workspace_sptr ws);
/// Deletes a workspce
void removeWS(const std::string &name);
/// Returns a workspace of a given type
template <typename T> boost::shared_ptr<T> getWS(const std::string &name) {
  return Mantid::API::AnalysisDataService::Instance().retrieveWS<T>(name);
}

Mantid::DataObjects::Workspace2D_sptr Create1DWorkspaceRand(int size);
Mantid::DataObjects::Workspace2D_sptr
Create1DWorkspaceConstant(int size, double value, double error);
Mantid::DataObjects::Workspace2D_sptr Create1DWorkspaceFib(int size);
Mantid::DataObjects::Workspace2D_sptr Create2DWorkspace(int nHist, int nBins);
Mantid::DataObjects::Workspace2D_sptr
Create2DWorkspaceWhereYIsWorkspaceIndex(int nhist, int numBoundaries);
Mantid::DataObjects::Workspace2D_sptr Create2DWorkspace123(
    int64_t nHist, int64_t nBin, bool isHist = false,
    const std::set<int64_t> &maskedWorkspaceIndices = std::set<int64_t>());
Mantid::DataObjects::Workspace2D_sptr Create2DWorkspace154(
    int64_t nHist, int64_t nBins, bool isHist = false,
    const std::set<int64_t> &maskedWorkspaceIndices = std::set<int64_t>());
Mantid::DataObjects::Workspace2D_sptr
maskSpectra(Mantid::DataObjects::Workspace2D_sptr workspace,
            const std::set<int64_t> &maskedWorkspaceIndices);
/**
 * Create a WorkspaceGroup with N workspaces and the specified parameters
 */
Mantid::API::WorkspaceGroup_sptr CreateWorkspaceGroup(int nEntries, int nHist,
                                                      int nBins,
                                                      const std::string &stem);
/** Create a 2D workspace with this many histograms and bins.
 * Filled with Y = 2.0 and E = sqrt(2.0)w
 */
Mantid::DataObjects::Workspace2D_sptr
Create2DWorkspaceBinned(int nHist, int nBins, double x0 = 0.0,
                        double deltax = 1.0);

/** Create a 2D workspace with this many histograms and bins. The bins are
 * assumed to be non-uniform and given by the input array
 * Filled with Y = 2.0 and E = sqrt(2.0)w
 */
Mantid::DataObjects::Workspace2D_sptr
Create2DWorkspaceBinned(int nHist, const int nBins, const double xBoundaries[]);

/**
 * Creates a 2D workspace from taking the function values from the input
 * function. The type must define operator()()
 * @param f :: A function to use for the signal values
 * @param nSpec :: The number of spectra
 * @param x0 :: The start of the x range
 * @param x1 :: The end of the x range
 * @param dx :: The steps in x
 * @param isHist :: True if it should be a histogram
 * @return The new workspace. The errors are set to 1.0
 */
template <typename Func>
Mantid::DataObjects::Workspace2D_sptr
Create2DWorkspaceFromFunction(Func f, int nSpec, double x0, double x1,
                              double dx, bool isHist = false) {
  int nX = int((x1 - x0) / dx) + 1;
  int nY = nX - (isHist ? 1 : 0);
  if (nY <= 0)
    throw std::invalid_argument(
        "Number of bins <=0. Cannot create an empty workspace");

  auto ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
      Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", nSpec, nX,
                                                       nY));

  for (int iSpec = 0; iSpec < nSpec; iSpec++) {
    Mantid::MantidVec &X = ws->dataX(iSpec);
    Mantid::MantidVec &Y = ws->dataY(iSpec);
    Mantid::MantidVec &E = ws->dataE(iSpec);
    for (int i = 0; i < nY; i++) {
      double x = x0 + dx * i;
      X[i] = x;
      Y[i] = f(x, iSpec);
      E[i] = 1;
    }
    if (isHist)
      X.back() = X[nY - 1] + dx;
  }
  return ws;
}

/// Add random noise to the signalcreate2DWorkspaceWithFullInstrument
void addNoise(Mantid::API::MatrixWorkspace_sptr ws, double noise,
              const double lower = -0.5, const double upper = 0.5);

/**
 * Create a test workspace with a fully defined instrument
 * Each spectra will have a cylindrical detector defined 2*cylinder_radius away
 * from the centre of the
 * pervious.
 * Data filled with: Y: 2.0, E: sqrt(2.0), X: nbins of width 1 starting at 0
 */
Mantid::DataObjects::Workspace2D_sptr create2DWorkspaceWithFullInstrument(
    int nHist, int nBins, bool includeMonitors = false,
    bool startYNegative = false, bool isHistogram = true,
    const std::string &instrumentName = std::string("testInst"));

/**
 * Create a test workspace with a Theta numeric axis instead of a spectrum axis
 * the values run from 1 to nhist
 * Data filled with: Y: 2.0, E: sqrt(2.0), X: nbins of width 1 starting at 0
 */
Mantid::DataObjects::Workspace2D_sptr create2DWorkspaceThetaVsTOF(int nHist,
                                                                  int nBins);

Mantid::DataObjects::Workspace2D_sptr
create2DWorkspaceWithRectangularInstrument(int numBanks, int numPixels,
                                           int numBins);

/** Create an Eventworkspace with an instrument that contains
 * RectangularDetector's */
Mantid::DataObjects::EventWorkspace_sptr
createEventWorkspaceWithFullInstrument(int numBanks, int numPixels,
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
Mantid::DataObjects::EventWorkspace_sptr
createEventWorkspaceWithNonUniformInstrument(int numBanks, bool clearEvents);

Mantid::DataObjects::WorkspaceSingleValue_sptr
CreateWorkspaceSingleValue(double value);
Mantid::DataObjects::WorkspaceSingleValue_sptr
CreateWorkspaceSingleValueWithError(double value, double error);
/** Perform some finalization on event workspace stuff */
void EventWorkspace_Finalize(Mantid::DataObjects::EventWorkspace_sptr ew);
/** Create event workspace with:
 * 500 pixels
 * 1000 histogrammed bins.
 */
Mantid::DataObjects::EventWorkspace_sptr CreateEventWorkspace();

/** Create event workspace with:
 * 50 pixels
 * 100 histogrammed bins from 0.0 in steps of 1.0
 * 200 events; two in each bin, at time 0.5, 1.5, etc.
 * PulseTime = 1 second, 2 seconds, etc.
 */
Mantid::DataObjects::EventWorkspace_sptr
CreateEventWorkspace2(int numPixels = 50, int numBins = 100);

Mantid::DataObjects::EventWorkspace_sptr
CreateEventWorkspace(int numPixels, int numBins, int numEvents = 100,
                     double x0 = 0.0, double binDelta = 1.0,
                     int eventPattern = 1, int start_at_pixelID = 0);

Mantid::DataObjects::EventWorkspace_sptr
CreateGroupedEventWorkspace(std::vector<std::vector<int>> groups, int numBins,
                            double binDelta = 1.0);

Mantid::DataObjects::EventWorkspace_sptr
CreateRandomEventWorkspace(size_t numbins, size_t numpixels,
                           double bin_delta = 1.0);

Mantid::API::MatrixWorkspace_sptr
CreateGroupedWorkspace2D(size_t numHist, int numBins, double binDelta);
// grouped workpsace with detectors arranges in rings in centre and into boxes
// outside
Mantid::API::MatrixWorkspace_sptr CreateGroupedWorkspace2DWithRingsAndBoxes(
    size_t RootOfNumHist = 10, int numBins = 10, double binDelta = 1.0);
// not strictly creating a workspace, but really helpful to see what one
// contains
void DisplayDataY(const Mantid::API::MatrixWorkspace_sptr ws);
// not strictly creating a workspace, but really helpful to see what one
// contains
void DisplayData(const Mantid::API::MatrixWorkspace_sptr ws);
// not strictly creating a workspace, but really helpful to see what one
// contains
void DisplayDataX(const Mantid::API::MatrixWorkspace_sptr ws);
// not strictly creating a workspace, but really helpful to see what one
// contains
void DisplayDataE(const Mantid::API::MatrixWorkspace_sptr ws);

void AddTSPEntry(Mantid::API::Run &runInfo, std::string name, double val);
void SetOrientedLattice(Mantid::API::MatrixWorkspace_sptr ws, double a,
                        double b, double c);
void SetGoniometer(Mantid::API::MatrixWorkspace_sptr ws, double phi, double chi,
                   double omega);

// create workspace which should be result of homering (transform to energy in
// inelastic)
Mantid::API::MatrixWorkspace_sptr
createProcessedWorkspaceWithCylComplexInstrument(
    size_t numPixels = 100, size_t numBins = 20,
    bool has_oriented_lattice = true);

// Create a workspace with all components needed for inelastic analysis;
Mantid::API::MatrixWorkspace_sptr createProcessedInelasticWS(
    const std::vector<double> &L2, const std::vector<double> &ploar,
    const std::vector<double> &azimutal, size_t numBins = 4, double Emin = -10,
    double Emax = 10, double Ei = 11);

Mantid::DataObjects::EventWorkspace_sptr
createEventWorkspace3(Mantid::DataObjects::EventWorkspace_const_sptr sourceWS,
                      std::string wsname, Mantid::API::Algorithm *alg);

/// Function to create a fixed RebinnedOutput workspace
Mantid::DataObjects::RebinnedOutput_sptr CreateRebinnedOutputWorkspace();

/// Create a simple peaks workspace containing the given number of peaks
boost::shared_ptr<Mantid::DataObjects::PeaksWorkspace>
createPeaksWorkspace(const int numPeaks = 2);
/**Build table workspace with preprocessed detectors for existign worksapce with
 * instrument */
boost::shared_ptr<Mantid::DataObjects::TableWorkspace>
buildPreprocessedDetectorsWorkspace(Mantid::API::MatrixWorkspace_sptr ws);
// create range of angular detectors positions
void create2DAngles(std::vector<double> &L2, std::vector<double> &polar,
                    std::vector<double> &azim, size_t nPolar = 10,
                    size_t nAzim = 10, double polStart = 0, double polEnd = 90,
                    double azimStart = -30, double azimEnd = 30);

/// Create a 2D workspace with one detector and one monitor based around a
/// virtual reflectometry instrument.
Mantid::API::MatrixWorkspace_sptr
create2DWorkspaceWithReflectometryInstrument(double startX = 0);

void createInstrumentForWorkspaceWithDistances(
    Mantid::API::MatrixWorkspace_sptr workspace,
    const Mantid::Kernel::V3D &samplePosition,
    const Mantid::Kernel::V3D &sourcePosition,
    const std::vector<Mantid::Kernel::V3D> &detectorPositions);
}

#endif /*WORKSPACECREATIONHELPER_H_*/
