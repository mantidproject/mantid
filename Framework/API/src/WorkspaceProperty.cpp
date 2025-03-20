// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"

// WorkspaceProperty implementation
#include "MantidAPI/WorkspaceProperty.hxx"

namespace Mantid::API {
///@cond TEMPLATE
template class MANTID_API_DLL Mantid::API::WorkspaceProperty<Mantid::API::Workspace>;
template class MANTID_API_DLL Mantid::API::WorkspaceProperty<Mantid::API::IEventWorkspace>;
template class MANTID_API_DLL Mantid::API::WorkspaceProperty<Mantid::API::IMDEventWorkspace>;
template class MANTID_API_DLL Mantid::API::WorkspaceProperty<Mantid::API::IMDHistoWorkspace>;
template class MANTID_API_DLL Mantid::API::WorkspaceProperty<Mantid::API::IMDWorkspace>;
template class MANTID_API_DLL Mantid::API::WorkspaceProperty<Mantid::API::MatrixWorkspace>;
template class MANTID_API_DLL Mantid::API::WorkspaceProperty<Mantid::API::IPeaksWorkspace>;
template class MANTID_API_DLL Mantid::API::WorkspaceProperty<Mantid::API::ITableWorkspace>;
template class MANTID_API_DLL Mantid::API::WorkspaceProperty<Mantid::API::WorkspaceGroup>;
///@endcond TEMPLATE
} // namespace Mantid::API
