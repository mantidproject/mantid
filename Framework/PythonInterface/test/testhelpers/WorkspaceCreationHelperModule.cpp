// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/******************************************************************************
 * Python module wrapper for the WorkspaceCreationHelper methods
 ******************************************************************************/
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/docstring_options.hpp>
#include <boost/python/module.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>

#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/kernel/Policies/AsType.h"
#include "MantidTestHelpers/MDEventsTestHelper.h" // These are still concerned with workspace creation so attach them here
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataObjects::MDEventsTestHelper;
using namespace Mantid::PythonInterface::Policies;
using namespace WorkspaceCreationHelper;

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
BOOST_PYTHON_FUNCTION_OVERLOADS(create2DWorkspaceWithFullInstrument_overloads,
                                create2DWorkspaceWithFullInstrument, 2, 4)

BOOST_PYTHON_FUNCTION_OVERLOADS(makeFakeMDHistoWorkspace_overloads,
                                makeFakeMDHistoWorkspace, 2, 7)

BOOST_PYTHON_FUNCTION_OVERLOADS(
    create2DWorkspaceWithRectangularInstrument_overloads,
    create2DWorkspaceWithRectangularInstrument, 3, 3)

GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

BOOST_PYTHON_MODULE(WorkspaceCreationHelper) {
  using namespace boost::python;

  // Doc string options - User defined, python arguments, C++ call signatures
  docstring_options docstrings(true, true, false);

  //=================================== 2D workspaces
  //===================================

  // Function pointers to disambiguate the calls
  using Signature1_2D = Workspace2D_sptr (*)(int, int, bool, bool, bool,
                                             const std::string &, bool);
  using Signature2_2D = Workspace2D_sptr (*)(int, int, int);
  using Signature3_2D = Workspace2D_sptr (*)(int, int, int, int);

  def("create2DWorkspaceWithFullInstrument",
      reinterpret_cast<Signature1_2D>(&create2DWorkspaceWithFullInstrument),
      create2DWorkspaceWithFullInstrument_overloads()
          [return_value_policy<AsType<Workspace_sptr>>()]);
  def("create2DWorkspaceWithRectangularInstrument",
      (Signature2_2D)&create2DWorkspaceWithRectangularInstrument,
      create2DWorkspaceWithRectangularInstrument_overloads());

  def("create2DWorkspace123WithMaskedBin",
      reinterpret_cast<Signature3_2D>(&create2DWorkspace123WithMaskedBin));

  //=================================== Event Workspaces
  //===================================

  def("createEventWorkspace", (EventWorkspace_sptr(*)())createEventWorkspace,
      return_value_policy<AsType<Workspace_sptr>>());
  def("createEventWorkspace2", &createEventWorkspace2,
      return_value_policy<AsType<Workspace_sptr>>());

  //=================================== Peak Workspaces
  //===================================

  def("createPeaksWorkspace",
      (PeaksWorkspace_sptr(*)(const int, const bool))createPeaksWorkspace,
      (arg("numPeaks") = 2, arg("createOrientedLattice") = false),
      return_value_policy<AsType<Workspace_sptr>>());

  //=================================== MD Workspaces
  //===================================

  // Typedef for function pointer to disabiguate references
  using Signature1_MDHisto =
      MDHistoWorkspace_sptr (*)(double, size_t, size_t, Mantid::coord_t, double,
                                const std::string &, double);

  def("makeFakeMDHistoWorkspace", (Signature1_MDHisto)&makeFakeMDHistoWorkspace,
      makeFakeMDHistoWorkspace_overloads()
          [return_value_policy<AsType<Workspace_sptr>>()]);
}
