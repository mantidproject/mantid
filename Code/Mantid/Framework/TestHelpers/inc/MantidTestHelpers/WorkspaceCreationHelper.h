#ifndef WORKSPACECREATIONHELPER_H_
#define WORKSPACECREATIONHELPER_H_
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidTestHelpers/DLLExport.h"

#include <cmath>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
// Other Helper
#include "ComponentCreationHelper.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

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

  DLL_TESTHELPERS Workspace1D_sptr Create1DWorkspaceRand(int size);
  DLL_TESTHELPERS Workspace1D_sptr Create1DWorkspaceConstant(int size, double value, double error);
  DLL_TESTHELPERS Workspace1D_sptr Create1DWorkspaceFib(int size);
  DLL_TESTHELPERS Workspace2D_sptr Create2DWorkspace(int xlen, int ylen);
  DLL_TESTHELPERS Workspace2D_sptr Create2DWorkspace123(int xlen, int ylen,bool isHist=0, const std::set<int> & 
					       maskedWorkspaceIndices = std::set<int>());
  DLL_TESTHELPERS Workspace2D_sptr Create2DWorkspace154(int xlen, int ylen,bool isHist=0, 
					       const std::set<int> & maskedWorkspaceIndices = std::set<int>());
  DLL_TESTHELPERS Workspace2D_sptr maskSpectra(DLL_TESTHELPERS Workspace2D_sptr workspace, 
				      const std::set<int> & maskedWorkspaceIndices);
  /** Create a 2D workspace with this many histograms and bins.
   * Filled with Y = 2.0 and E = sqrt(2.0)w
   */
  DLL_TESTHELPERS Workspace2D_sptr Create2DWorkspaceBinned(int nhist, int nbins, double x0=0.0, double deltax = 1.0);

  /** Create a 2D workspace with this many histograms and bins. The bins are assumed to be non-uniform and given by the input array
   * Filled with Y = 2.0 and E = sqrt(2.0)w
   */
  DLL_TESTHELPERS Workspace2D_sptr Create2DWorkspaceBinned(int nhist, const int numBoundaries, 
						  const double xBoundaries[]);
  /**
   * Create a test workspace with a fully defined instrument
   * Each spectra will have a cylindrical detector defined 2*cylinder_radius away from the centre of the
   * pervious. 
   * Data filled with: Y: 2.0, E: sqrt(2.0), X: nbins of width 1 starting at 0 
   */
  DLL_TESTHELPERS Workspace2D_sptr create2DWorkspaceWithFullInstrument(int nhist, int nbins, 
							      bool includeMonitors = false);
  DLL_TESTHELPERS WorkspaceSingleValue_sptr CreateWorkspaceSingleValue(double value);
  DLL_TESTHELPERS WorkspaceSingleValue_sptr CreateWorkspaceSingleValueWithError(double value, double error);
  /** Perform some finalization on event workspace stuff */
  DLL_TESTHELPERS void EventWorkspace_Finalize(EventWorkspace_sptr ew);
  /** Create event workspace with:
   * 500 pixels
   * 1000 histogrammed bins.
   */
  DLL_TESTHELPERS EventWorkspace_sptr CreateEventWorkspace();
  /** Create event workspace with:
   * 50 pixels
   * 100 histogrammed bins from 0.0 in steps of 1.0
   * 200 events; two in each bin, at time 0.5, 1.5, etc.
   * PulseTime = 1 second, 2 seconds, etc.
   */
  DLL_TESTHELPERS EventWorkspace_sptr CreateEventWorkspace2();
  /** Create event workspace
   */
  DLL_TESTHELPERS EventWorkspace_sptr 
  CreateEventWorkspace(int numPixels, int numBins, int numEvents = 100, double x0=0.0, double binDelta=1.0,
		       int eventPattern = 1, int start_at_pixelID = 0);
  /** Create event workspace
   */
  DLL_TESTHELPERS EventWorkspace_sptr CreateGroupedEventWorkspace(std::vector< std::vector<int> > groups,
							 int numBins, double binDelta=1.0);
  //not strictly creating a workspace, but really helpfull to see what one contains
  DLL_TESTHELPERS void DisplayDataY(const MatrixWorkspace_sptr ws);
  //not strictly creating a workspace, but really helpfull to see what one contains
  DLL_TESTHELPERS void DisplayData(const MatrixWorkspace_sptr ws);
  //not strictly creating a workspace, but really helpfull to see what one contains
  DLL_TESTHELPERS void DisplayDataX(const MatrixWorkspace_sptr ws);
  //not strictly creating a workspace, but really helpfull to see what one contains
  DLL_TESTHELPERS void DisplayDataE(const MatrixWorkspace_sptr ws);

};

#endif /*WORKSPACECREATIONHELPER_H_*/
