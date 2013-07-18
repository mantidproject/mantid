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

#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h" // These are still concerned with workspace creation so attach them here
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace WorkspaceCreationHelper;
using namespace Mantid::MDEvents::MDEventsTestHelper;

BOOST_PYTHON_FUNCTION_OVERLOADS(create2DWorkspaceWithFullInstrument_overloads, create2DWorkspaceWithFullInstrument, 2, 4);
BOOST_PYTHON_FUNCTION_OVERLOADS(makeFakeMDHistoWorkspace_overloads, makeFakeMDHistoWorkspace, 2, 7);


BOOST_PYTHON_MODULE(WorkspaceCreationHelper)
{
  using namespace boost::python;

  // Doc string options - User defined, python arguments, C++ call signatures
  docstring_options docstrings(true, true, false);

  //=================================== 2D workspaces ===================================
  using namespace Mantid::DataObjects;
  register_ptr_to_python<boost::shared_ptr<Workspace2D> >();
  class_<Workspace2D, bases<Mantid::API::MatrixWorkspace>,boost::noncopyable>("Workspace2D", no_init);

  def("create2DWorkspaceWithFullInstrument", create2DWorkspaceWithFullInstrument, 
      create2DWorkspaceWithFullInstrument_overloads());

  //=================================== Event Workspaces ===================================
  register_ptr_to_python<boost::shared_ptr<EventWorkspace> >();
  class_<EventWorkspace, bases<Mantid::API::IEventWorkspace>,boost::noncopyable>("EventWorkspace", no_init);

  def("CreateEventWorkspace", (EventWorkspace_sptr (*)())&CreateEventWorkspace);
  def("CreateEventWorkspace2", &CreateEventWorkspace2);

  //--- Peaks workspace ---
  register_ptr_to_python<boost::shared_ptr<PeaksWorkspace> >();
  class_<PeaksWorkspace, bases<Mantid::API::IPeaksWorkspace>,boost::noncopyable>("PeaksWorkspace", no_init);

  def("createPeaksWorkspace", &createPeaksWorkspace);

  //=================================== MD Workspaces ===================================
  using namespace Mantid::MDEvents;
  register_ptr_to_python<boost::shared_ptr<MDHistoWorkspace>>();
  class_<MDHistoWorkspace, bases<Mantid::API::IMDHistoWorkspace>,boost::noncopyable>("MDHistoWorkspace", no_init);

  def("makeFakeMDHistoWorkspace", makeFakeMDHistoWorkspace, makeFakeMDHistoWorkspace_overloads());
}
