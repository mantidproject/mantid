// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"

// PropertyWithValue implementation
#include "MantidKernel/PropertyWithValue.hxx"

namespace Mantid::Kernel {

/// @cond
template class MANTID_API_DLL PropertyWithValue<std::shared_ptr<API::IAlgorithm>>;
template class MANTID_API_DLL PropertyWithValue<std::shared_ptr<API::IEventWorkspace>>;
template class MANTID_API_DLL PropertyWithValue<std::shared_ptr<API::IFunction>>;
template class MANTID_API_DLL PropertyWithValue<std::shared_ptr<API::IMaskWorkspace>>;
template class MANTID_API_DLL PropertyWithValue<std::shared_ptr<API::IMDEventWorkspace>>;
template class MANTID_API_DLL PropertyWithValue<std::shared_ptr<API::IMDHistoWorkspace>>;
template class MANTID_API_DLL PropertyWithValue<std::shared_ptr<API::IMDWorkspace>>;
template class MANTID_API_DLL PropertyWithValue<std::shared_ptr<API::IPeaksWorkspace>>;
template class MANTID_API_DLL PropertyWithValue<std::shared_ptr<API::ISplittersWorkspace>>;
template class MANTID_API_DLL PropertyWithValue<std::shared_ptr<API::ITableWorkspace>>;
template class MANTID_API_DLL PropertyWithValue<std::shared_ptr<API::MatrixWorkspace>>;
template class MANTID_API_DLL PropertyWithValue<std::shared_ptr<API::Workspace>>;
template class MANTID_API_DLL PropertyWithValue<std::shared_ptr<API::WorkspaceGroup>>;
template class MANTID_API_DLL PropertyWithValue<std::shared_ptr<API::ExperimentInfo>>;
/// @endcond

} // namespace Mantid::Kernel
