// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDEvent.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/python/class.hpp>

using Mantid::API::IMDEventWorkspace;
using Mantid::DataObjects::MDEvent;
using Mantid::DataObjects::MDEventWorkspace;
using Mantid::DataObjects::MDLeanEvent;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

//-----------------------------------------------------------------------------
// MSVC workaround - abuse the preprocessor to avoid repeated definitions
//-----------------------------------------------------------------------------

// Aliases to avoid commas in macro expansion
template <unsigned int n> using MDLeanEventEventWorkspace = MDEventWorkspace<MDLeanEvent<n>, n>;
template <unsigned int n> using MDEventEventWorkspace = MDEventWorkspace<MDEvent<n>, n>;

#define MDEVENT_GET_POINTER_N(type, n) GET_POINTER_SPECIALIZATION(BOOST_PP_CAT(type, EventWorkspace<n>))
#define DECL(z, n, text) MDEVENT_GET_POINTER_N(text, n)
BOOST_PP_REPEAT_FROM_TO(1, 10, DECL, MDLeanEvent)
// cppcheck-suppress unknownMacro
BOOST_PP_REPEAT_FROM_TO(1, 10, DECL, MDEvent)
#undef DECL

namespace {

/**
 * @tparam MDE The event type
 * @tparam nd The number of dimensions
 * @param className Name of the class in Python
 */
template <typename MDE, size_t nd> void MDEventWorkspaceExportImpl(const char *className) {
  using ExportType = MDEventWorkspace<MDE, nd>;

  class_<ExportType, bases<IMDEventWorkspace>, boost::noncopyable>(className, no_init);

  // register pointers
  RegisterWorkspacePtrToPython<ExportType>();
}
} // namespace

void export_MDEventWorkspaces() {
// The maximum number of dimensions is defined in the MDWorkspaceFactory
#define STR(x) #x
#define CLS_NAME(type, n) STR(type##Workspace##n##D)
#define DECL(z, n, text) MDEventWorkspaceExportImpl<text<n>, n>(CLS_NAME(text, n));
  BOOST_PP_REPEAT_FROM_TO(1, 10, DECL, MDLeanEvent)
  BOOST_PP_REPEAT_FROM_TO(1, 10, DECL, MDEvent)
#undef DECL
}
