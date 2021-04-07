// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/TypedValidator.h"

namespace Mantid {
namespace API {

/**
  An interface for those validators that require the MatrixWorkspace interface
*/
class MANTID_API_DLL MatrixWorkspaceValidator : public Kernel::TypedValidator<MatrixWorkspace_sptr> {};

} // namespace API
} // namespace Mantid
