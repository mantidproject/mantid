/******************************************************************************
 * Python module wrapper for the WorkspaceCreationHelper methods
 ******************************************************************************/
#include <boost/python/docstring_options.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/module.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h" // These are still concerned with workspace creation so attach them here
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace WorkspaceCreationHelper;
using namespace Mantid::DataObjects::MDEventsTestHelper;

BOOST_PYTHON_FUNCTION_OVERLOADS(create2DWorkspaceWithFullInstrument_overloads, create2DWorkspaceWithFullInstrument, 2, 4)

BOOST_PYTHON_FUNCTION_OVERLOADS(makeFakeMDHistoWorkspace_overloads, makeFakeMDHistoWorkspace, 2, 7)

BOOST_PYTHON_FUNCTION_OVERLOADS(create2DWorkspaceWithRectangularInstrument_overloads, create2DWorkspaceWithRectangularInstrument, 3, 3)

namespace
{
  /**
   * Proxy to return an API::IEventWorkspace_sptr rather than DataObjects::EventWorkspace_sptr
   */
  Mantid::API::IEventWorkspace_sptr CreateEventWorkspaceProxy()
  {
    return WorkspaceCreationHelper::CreateEventWorkspace();
  }
}


BOOST_PYTHON_MODULE(WorkspaceCreationHelper)
{
  using namespace boost::python;

  // Doc string options - User defined, python arguments, C++ call signatures
  docstring_options docstrings(true, true, false);

  //=================================== 2D workspaces ===================================
  using namespace Mantid::API;

  // Forces it to return a MatrixWorkspace pointer rather than Workspace2D
  typedef MatrixWorkspace_sptr (*Signature1_2D)(int nHist, int nBins,
                                                bool includeMonitors, 
                                                bool startYNegative);
  // Forces it to return a MatrixWorkspace pointer rather than Workspace2D
  typedef MatrixWorkspace_sptr (*Signature2_2D)(int numBanks,
                                                int numPixels,
                                                int numBins);

  def("create2DWorkspaceWithFullInstrument", (Signature1_2D)&create2DWorkspaceWithFullInstrument, 
      create2DWorkspaceWithFullInstrument_overloads());
  def("create2DWorkspaceWithRectangularInstrument", (Signature2_2D)&create2DWorkspaceWithRectangularInstrument,
      create2DWorkspaceWithRectangularInstrument_overloads());


  //=================================== Event Workspaces ===================================
  
  // Forces the returns the be IEventWorkspace_sptr
  typedef IEventWorkspace_sptr (*Signature2_Event)(int,int);

  def("CreateEventWorkspace", &CreateEventWorkspaceProxy);
  def("CreateEventWorkspace2", (Signature2_Event)&CreateEventWorkspace2);

  //=================================== Peak Workspaces ===================================

  // Forces the returns the be IPeaks_sptr
  typedef IPeaksWorkspace_sptr (*Signature1_Peaks)(const int);
  typedef IPeaksWorkspace_sptr (*Signature2_Peaks)(const int, const bool);

  def("createPeaksWorkspace", (Signature1_Peaks)&createPeaksWorkspace);
  def("createPeaksWorkspace", (Signature2_Peaks)&createPeaksWorkspace);

  //=================================== MD Workspaces ===================================

  // Forces the returns the be IMDHistoWorkspaceWorkspace_sptr
  typedef IMDHistoWorkspace_sptr (*Signature1_MDHisto)(double, size_t, size_t, Mantid::coord_t max,
                                                       double, std::string name, double);

  def("makeFakeMDHistoWorkspace", (Signature1_MDHisto)&makeFakeMDHistoWorkspace, makeFakeMDHistoWorkspace_overloads());
}
