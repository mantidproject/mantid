#ifndef WORKSPACECREATIONHELPER_H_
#define WORKSPACECREATIONHELPER_H_
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidTestHelpers/DLLExport.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidAPI/Run.h"

namespace WorkspaceCreationHelper
{
  /// Create a fibonacci series
  template<typename T>
  struct FibSeries
  {
  private:
    T x1;  /// Initial value 1;
    T x2;  /// Initial value 2;
  public:
    inline FibSeries() : x1(1),x2(1) {}
    inline T operator()() { const T out(x1+x2); x1=x2; x2=out;  return out; }
  };

  DLL_TESTHELPERS Mantid::DataObjects::Workspace1D_sptr Create1DWorkspaceRand(int size);
  DLL_TESTHELPERS Mantid::DataObjects::Workspace1D_sptr Create1DWorkspaceConstant(int size, double value, double error);
  DLL_TESTHELPERS Mantid::DataObjects::Workspace1D_sptr Create1DWorkspaceFib(int size);
  DLL_TESTHELPERS Mantid::DataObjects::Workspace2D_sptr Create2DWorkspace(int nHist, int nBins);
  DLL_TESTHELPERS Mantid::DataObjects::Workspace2D_sptr Create2DWorkspaceWhereYIsWorkspaceIndex(int nhist, int numBoundaries);
  DLL_TESTHELPERS Mantid::DataObjects::Workspace2D_sptr Create2DWorkspace123(int64_t nHist, int64_t nBins, bool isHist=false, const std::set<int64_t> &
					       maskedWorkspaceIndices = std::set<int64_t>());
  DLL_TESTHELPERS Mantid::DataObjects::Workspace2D_sptr Create2DWorkspace154(int64_t nHist, int64_t nBins, bool isHist=false,
					       const std::set<int64_t> & maskedWorkspaceIndices = std::set<int64_t>());
  DLL_TESTHELPERS Mantid::DataObjects::Workspace2D_sptr maskSpectra(Mantid::DataObjects::Workspace2D_sptr workspace, 
								    const std::set<int64_t> & maskedWorkspaceIndices);
  /** Create a 2D workspace with this many histograms and bins.
   * Filled with Y = 2.0 and E = sqrt(2.0)w
   */
  DLL_TESTHELPERS Mantid::DataObjects::Workspace2D_sptr Create2DWorkspaceBinned(int nHist, int nBins, double x0=0.0, double deltax = 1.0);

  /** Create a 2D workspace with this many histograms and bins. The bins are assumed to be non-uniform and given by the input array
   * Filled with Y = 2.0 and E = sqrt(2.0)w
   */
  DLL_TESTHELPERS Mantid::DataObjects::Workspace2D_sptr Create2DWorkspaceBinned(int nHist, const int nBins, const double xBoundaries[]);
  /**
   * Create a test workspace with a fully defined instrument
   * Each spectra will have a cylindrical detector defined 2*cylinder_radius away from the centre of the
   * pervious. 
   * Data filled with: Y: 2.0, E: sqrt(2.0), X: nbins of width 1 starting at 0 
   */
  DLL_TESTHELPERS Mantid::DataObjects::Workspace2D_sptr create2DWorkspaceWithFullInstrument(int nHist, int nBins,
                    bool includeMonitors = false);

  /** Create an Eventworkspace with an instrument that contains RectangularDetector's */
  DLL_TESTHELPERS Mantid::DataObjects::EventWorkspace_sptr createEventWorkspaceWithFullInstrument(int numBanks, int numPixels);

  DLL_TESTHELPERS Mantid::DataObjects::WorkspaceSingleValue_sptr CreateWorkspaceSingleValue(double value);
  DLL_TESTHELPERS Mantid::DataObjects::WorkspaceSingleValue_sptr CreateWorkspaceSingleValueWithError(double value, double error);
  /** Perform some finalization on event workspace stuff */
  DLL_TESTHELPERS void EventWorkspace_Finalize(Mantid::DataObjects::EventWorkspace_sptr ew);
  /** Create event workspace with:
   * 500 pixels
   * 1000 histogrammed bins.
   */
  DLL_TESTHELPERS Mantid::DataObjects::EventWorkspace_sptr CreateEventWorkspace();

  /** Create event workspace with:
   * 50 pixels
   * 100 histogrammed bins from 0.0 in steps of 1.0
   * 200 events; two in each bin, at time 0.5, 1.5, etc.
   * PulseTime = 1 second, 2 seconds, etc.
   */
  DLL_TESTHELPERS Mantid::DataObjects::EventWorkspace_sptr CreateEventWorkspace2(int numPixels=50, int numBins=100);

  DLL_TESTHELPERS Mantid::DataObjects::EventWorkspace_sptr 
  CreateEventWorkspace(int numPixels, int numBins, int numEvents = 100, double x0=0.0, double binDelta=1.0,
		       int eventPattern = 1, int start_at_pixelID = 0);

  DLL_TESTHELPERS Mantid::DataObjects::EventWorkspace_sptr CreateGroupedEventWorkspace(std::vector< std::vector<int> > groups,
							 int numBins, double binDelta=1.0);

  DLL_TESTHELPERS Mantid::DataObjects::EventWorkspace_sptr CreateRandomEventWorkspace(size_t numbins, size_t numpixels, double bin_delta=1.0);

  DLL_TESTHELPERS Mantid::API::MatrixWorkspace_sptr CreateGroupedWorkspace2D(size_t numHist, int numBins, double binDelta);
  // grouped workpsace with rings in centre and boxes outside
  DLL_TESTHELPERS Mantid::API::MatrixWorkspace_sptr CreateGroupedWorkspace2DWithRingsAndBoxes(size_t RootOfNumHist=10, int numBins=10, double binDelta=1.0);
  //not strictly creating a workspace, but really helpfull to see what one contains
  DLL_TESTHELPERS void DisplayDataY(const Mantid::API::MatrixWorkspace_sptr ws);
  //not strictly creating a workspace, but really helpfull to see what one contains
  DLL_TESTHELPERS void DisplayData(const Mantid::API::MatrixWorkspace_sptr ws);
  //not strictly creating a workspace, but really helpfull to see what one contains
  DLL_TESTHELPERS void DisplayDataX(const Mantid::API::MatrixWorkspace_sptr ws);
  //not strictly creating a workspace, but really helpfull to see what one contains
  DLL_TESTHELPERS void DisplayDataE(const Mantid::API::MatrixWorkspace_sptr ws);

  DLL_TESTHELPERS void AddTSPEntry(Mantid::API::Run & runInfo, std::string name, double val);
  DLL_TESTHELPERS void SetOrientedLattice(Mantid::API::MatrixWorkspace_sptr ws, double a, double b, double c);
  DLL_TESTHELPERS void SetGoniometer(Mantid::API::MatrixWorkspace_sptr ws, double phi, double chi, double omega);

};

#endif /*WORKSPACECREATIONHELPER_H_*/
