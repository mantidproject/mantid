// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

// WorkspaceProperty implementation
#include "MantidAPI/WorkspaceProperty.hxx"

namespace Mantid::API {
///@cond TEMPLATE
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspaceProperty<Mantid::DataObjects::EventWorkspace>;
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspaceProperty<Mantid::DataObjects::SpecialWorkspace2D>;
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspaceProperty<Mantid::DataObjects::SplittersWorkspace>;
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspaceProperty<Mantid::DataObjects::TableWorkspace>;
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspaceProperty<Mantid::DataObjects::Workspace2D>;
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspaceProperty<Mantid::DataObjects::WorkspaceSingleValue>;
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspaceProperty<Mantid::DataObjects::GroupingWorkspace>;
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspaceProperty<Mantid::DataObjects::PeaksWorkspace>;
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspaceProperty<Mantid::DataObjects::MaskWorkspace>;
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspaceProperty<Mantid::DataObjects::OffsetsWorkspace>;
///@endcond TEMPLATE
} // namespace Mantid::API
